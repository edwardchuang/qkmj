#include "ai_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include "qkmj.h"
#include "protocol.h"
#include "protocol_def.h"

static int ai_enabled = 0;
static int ai_debug_enabled = 0;
static char ai_endpoint[256] = "http://localhost:8000";
static char ai_session_id[40] = "";

#define AI_LOG(...) if (ai_debug_enabled) { printf(__VA_ARGS__); }
#define AI_ERR(...) fprintf(stderr, __VA_ARGS__)

struct memory {
  char *response;
  size_t size;
};

// Helper to log to server (MongoDB)
static void ai_send_log_to_server(const char* type, cJSON* req, cJSON* resp, const char* err) {
    if (!ai_session_id[0]) return;

    cJSON *log = cJSON_CreateObject();
    cJSON_AddStringToObject(log, "level", "ai_trace");
    cJSON_AddNumberToObject(log, "timestamp", (double)time(NULL) * 1000.0);
    cJSON_AddStringToObject(log, "session_id", ai_session_id);
    cJSON_AddStringToObject(log, "user_id", (char*)my_name);
    cJSON_AddStringToObject(log, "type", type);
    
    if (req) cJSON_AddItemToObject(log, "request", cJSON_Duplicate(req, 1));
    if (resp) cJSON_AddItemToObject(log, "response", cJSON_Duplicate(resp, 1));
    if (err) cJSON_AddStringToObject(log, "error", err);

    /* Use send_json to send message ID 901 (AI Log) */
    send_json(gps_sockfd, 901, log); 
    
    /* send_json consumes the object? No, checking protocol.c:
       cJSON_AddItemToObject(root, "data", data);
       send_json TAKES OWNERSHIP of 'data' because it adds it to 'root' and then deletes 'root'.
       Wait, let's double check protocol.c implementation I read earlier.
    */
    /* protocol.c:
       int send_json(int fd, int msg_id, cJSON *data) {
           cJSON *root = cJSON_CreateObject();
           // ...
           if (data) {
               cJSON_AddItemToObject(root, "data", data);
           }
           // ...
           cJSON_Delete(root);
           return 1;
       }
       cJSON_AddItemToObject transfers ownership.
       When root is deleted, data is deleted.
       So we DO NOT need to cJSON_Delete(log) here if send_json is called.
       BUT, if send_json returns 0 (failure) early?
       protocol.c:
       if (!root) { if (data) cJSON_Delete(data); return 0; }
       ...
       cJSON_Delete(root); // Deletes data
       So yes, send_json takes ownership.
    */
}

static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;
 
  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(ptr == NULL) return 0;  /* out of memory */
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
  
  // Debug log: print response chunk
  AI_LOG("[AI DEBUG] Received chunk: %s\n", (char*)data);
 
  return realsize;
}

static size_t registration_write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    AI_LOG("[AI DEBUG] Registration Response: %.*s\n", (int)realsize, (char*)data);
    return realsize;
}

static void generate_session_id() {
    srand((unsigned int)time(NULL) ^ (unsigned int)rand());
    sprintf(ai_session_id, "%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
        rand() & 0xFFFF, rand() & 0xFFFF,
        rand() & 0xFFFF,
        ((rand() & 0x0FFF) | 0x4000), // Version 4
        ((rand() & 0x3FFF) | 0x8000), // Variant 1
        rand() & 0xFFFF, rand() & 0xFFFF, rand() & 0xFFFF);
    AI_LOG("[AI DEBUG] Generated Session ID: %s\n", ai_session_id);
}

static chtype ai_save_buf[3][20];
static int ai_saved = 0;

static void show_ai_thinking() {
    if (!stdscr) return;
    if (ai_saved) return;

    int x = 30, y = 11;
    int w = 20, h = 3;
    int i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            ai_save_buf[j][i] = mvwinch(stdscr, y + j, x + i);
        }
    }
    ai_saved = 1;

    wmvaddstr(stdscr, y, x,   "+------------------+");
    
    attron(A_REVERSE | A_BOLD);
    wmvaddstr(stdscr, y+1, x, "|  AI Thinking...  |");
    attroff(A_REVERSE | A_BOLD);
    
    wmvaddstr(stdscr, y+2, x, "+------------------+");
    wrefresh(stdscr);
}

static void hide_ai_thinking() {
    if (!stdscr || !ai_saved) return;

    int x = 30, y = 11;
    int w = 20, h = 3;
    int i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            mvwaddch(stdscr, y + j, x + i, ai_save_buf[j][i]);
        }
    }
    ai_saved = 0;
    wrefresh(stdscr);
}

static int ai_register_session() {
    CURL *curl;
    CURLcode res;
    char url[1024];
    char *encoded_name;
    int success = 0;

    if (!ai_session_id[0]) return 0;

    curl = curl_easy_init();
    if (curl) {
        encoded_name = curl_easy_escape(curl, (char*)my_name, 0);
        snprintf(url, sizeof(url), "%s/apps/agent/users/%s/sessions/%s", 
                 ai_endpoint, encoded_name ? encoded_name : "unknown", ai_session_id);
        if (encoded_name) curl_free(encoded_name);

        AI_LOG("[AI DEBUG] Registering Session. URL: %s\n", url);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ""); // Empty body
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, registration_write_callback);
        // SSL verification off for localhost
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        show_ai_thinking();
        res = curl_easy_perform(curl);
        hide_ai_thinking();

        // Log registration attempt
        cJSON *req_json = cJSON_CreateObject();
        cJSON_AddStringToObject(req_json, "url", url);
        cJSON *resp_json = cJSON_CreateObject(); // Empty for now as we don't capture it easily without custom struct
        
        if (res != CURLE_OK) {
            AI_ERR("[AI ERROR] AI Registration failed: %s\n", curl_easy_strerror(res));
            ai_send_log_to_server("registration", req_json, NULL, curl_easy_strerror(res));
        } else {
            AI_LOG("[AI DEBUG] Session Registration Successful\n");
            success = 1;
            ai_send_log_to_server("registration", req_json, NULL, NULL);
        }
        
        cJSON_Delete(req_json);
        cJSON_Delete(resp_json);
        curl_easy_cleanup(curl);
    }
    return success;
}

void ai_init() {
    char *env_mode = getenv("AI_MODE");
    char *env_endpoint = getenv("AI_ENDPOINT");
    char *env_debug = getenv("AI_DEBUG");
    
    if (env_endpoint) {
        strncpy(ai_endpoint, env_endpoint, sizeof(ai_endpoint) - 1);
        ai_endpoint[sizeof(ai_endpoint) - 1] = '\0';
    }
    
    if (env_debug && (strcmp(env_debug, "1") == 0 || strcasecmp(env_debug, "true") == 0)) {
        ai_debug_enabled = 1;
    }

    AI_LOG("[AI DEBUG] AI Init. Endpoint: %s\n", ai_endpoint);

    curl_global_init(CURL_GLOBAL_ALL);

    if (env_mode && strcmp(env_mode, "auto") == 0) {
        AI_LOG("[AI DEBUG] Auto mode detected. Enabling AI...\n");
        ai_set_enabled(1); // Use setter to trigger session ID generation and registration
    }
}

void ai_cleanup() {
    curl_global_cleanup();
}

int ai_is_enabled() {
    return ai_enabled;
}

void ai_set_enabled(int enabled) {
    AI_LOG("[AI DEBUG] Setting AI enabled: %d\n", enabled);
    if (enabled && !ai_enabled) {
        generate_session_id();
        if (ai_register_session()) {
            char msg[256];
            snprintf(msg, sizeof(msg), "AI Session Created: %s", ai_session_id);
            display_comment(msg);
        } else {
            display_comment("AI Session Registration Failed!");
        }
    }
    ai_enabled = enabled;
}

// Helper to add player state
void add_player_state(cJSON *players_array, int seat_idx) {
    cJSON *player_obj = cJSON_CreateObject();
    int i;
    cJSON *hand_array, *meld_array, *discard_array, *flower_array;

    cJSON_AddNumberToObject(player_obj, "seat", seat_idx);
    cJSON_AddNumberToObject(player_obj, "score", player[table[seat_idx]].money); // Approximate score using money
    cJSON_AddBoolToObject(player_obj, "is_me", (seat_idx == my_sit));

    if (seat_idx == my_sit) {
        hand_array = cJSON_CreateArray();
        for(i=0; i<pool[seat_idx].num; i++) {
            cJSON_AddItemToArray(hand_array, cJSON_CreateNumber(pool[seat_idx].card[i]));
        }
        cJSON_AddItemToObject(player_obj, "hand", hand_array);
    } else {
        cJSON_AddNumberToObject(player_obj, "hand_count", pool[seat_idx].num);
    }

    // Exposed melds
    meld_array = cJSON_CreateArray();
    for (i = 0; i < pool[seat_idx].out_card_index; i++) {
        cJSON *meld = cJSON_CreateObject();
        int type = pool[seat_idx].out_card[i][0];
        if (type == 1 || type == 2 || type == 3 || type == 11 || type == 12) {
             cJSON_AddNumberToObject(meld, "type", type);
             cJSON_AddNumberToObject(meld, "card", pool[seat_idx].out_card[i][1]); 
             cJSON_AddItemToArray(meld_array, meld);
        }
    }
    cJSON_AddItemToObject(player_obj, "melds", meld_array);

    // Discards (placeholder)
    discard_array = cJSON_CreateArray();
    cJSON_AddItemToObject(player_obj, "discards", discard_array); 

    // Flowers
    flower_array = cJSON_CreateArray();
    for (i=0; i<8; i++) {
        if (pool[seat_idx].flower[i]) cJSON_AddItemToArray(flower_array, cJSON_CreateNumber(i));
    }
    cJSON_AddItemToObject(player_obj, "flowers", flower_array);

    cJSON_AddItemToArray(players_array, player_obj);
}

char* ai_serialize_state(ai_phase_t phase, int card, int from_seat) {
    cJSON *root, *context, *event, *players, *legal;
    char *json_payload;
    int i;

    // Build JSON
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "cmd", "decision");
    
    // Note: session_id is now sent in the wrapper, but we can keep it here too if useful for debugging
    if (ai_session_id[0] != '\0') {
        cJSON_AddStringToObject(root, "session_id", ai_session_id);
    }

    // Context
    context = cJSON_CreateObject();
    cJSON_AddNumberToObject(context, "round_wind", info.wind);
    cJSON_AddNumberToObject(context, "dealer", info.dealer);
    cJSON_AddNumberToObject(context, "my_seat", my_sit);
    cJSON_AddNumberToObject(context, "turn_seat", turn);
    cJSON_AddNumberToObject(context, "remain_cards", 144 - card_point);
    cJSON_AddStringToObject(context, "phase", phase == AI_PHASE_DISCARD ? "discard" : "claim");
    cJSON_AddItemToObject(root, "context", context);

    // Event
    event = cJSON_CreateObject();
    cJSON_AddNumberToObject(event, "new_card", card);
    if (phase == AI_PHASE_CLAIM) {
        cJSON_AddNumberToObject(event, "from_seat", from_seat);
    }
    cJSON_AddItemToObject(root, "event", event);

    // Players
    players = cJSON_CreateArray();
    for (i = 1; i <= 4; i++) {
        if (table[i]) add_player_state(players, i);
    }
    cJSON_AddItemToObject(root, "players", players);

    // Legal Actions
    legal = cJSON_CreateObject();
    if (phase == AI_PHASE_DISCARD) {
        cJSON_AddBoolToObject(legal, "can_discard", 1);
        cJSON_AddBoolToObject(legal, "can_win", check_flag[my_sit][4]); 
        cJSON_AddBoolToObject(legal, "can_kang", check_flag[my_sit][3]);
    } else {
        cJSON_AddBoolToObject(legal, "can_eat", check_flag[my_sit][1]);
        cJSON_AddBoolToObject(legal, "can_pong", check_flag[my_sit][2]);
        cJSON_AddBoolToObject(legal, "can_kang", check_flag[my_sit][3]);
        cJSON_AddBoolToObject(legal, "can_win", check_flag[my_sit][4]);
    }
    cJSON_AddItemToObject(root, "legal_actions", legal);

    json_payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_payload;
}

ai_decision_t ai_parse_decision(const char* json_response) {
    ai_decision_t decision = {AI_ACTION_NONE, 0, {0, 0}};
    cJSON *resp_root = cJSON_Parse(json_response);
    cJSON *target_root = NULL;
    cJSON *temp_root = NULL;
    
    if (!resp_root) {
        AI_LOG("[AI ERROR] Failed to parse JSON response\n");
        return decision;
    }

    // Check if it's the specific array format from the agent
    if (cJSON_IsArray(resp_root)) {
        cJSON *item0 = cJSON_GetArrayItem(resp_root, 0);
        if (item0) {
            cJSON *content = cJSON_GetObjectItem(item0, "content");
            if (content) {
                cJSON *parts = cJSON_GetObjectItem(content, "parts");
                if (parts && cJSON_IsArray(parts)) {
                    cJSON *part0 = cJSON_GetArrayItem(parts, 0);
                    if (part0) {
                        cJSON *text = cJSON_GetObjectItem(part0, "text");
                        if (text && cJSON_IsString(text)) {
                            AI_LOG("[AI DEBUG] Found wrapped JSON string: %s\n", text->valuestring);
                            
                            const char *clean_json = text->valuestring;
                            // Skip leading markdown code block markers
                            while (*clean_json == ' ' || *clean_json == '\n' || *clean_json == '\r' || *clean_json == '\t') clean_json++;
                            if (strncmp(clean_json, "```json", 7) == 0) clean_json += 7;
                            else if (strncmp(clean_json, "```", 3) == 0) clean_json += 3;
                            
                            temp_root = cJSON_Parse(clean_json);
                            if (temp_root) {
                                target_root = temp_root;
                            } else {
                                AI_LOG("[AI ERROR] Failed to parse wrapped JSON string\n");
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Fallback: maybe it's the direct object?
        target_root = resp_root;
    }

    if (target_root) {
        cJSON *action_item = cJSON_GetObjectItem(target_root, "action");
        if (action_item && cJSON_IsString(action_item)) {
            const char *act = action_item->valuestring;
            AI_LOG("[AI DEBUG] Parsed Action: %s\n", act);
            if (strcmp(act, "discard") == 0) decision.action = AI_ACTION_DISCARD;
            else if (strcmp(act, "eat") == 0) decision.action = AI_ACTION_EAT;
            else if (strcmp(act, "pong") == 0) decision.action = AI_ACTION_PONG;
            else if (strcmp(act, "kang") == 0) decision.action = AI_ACTION_KANG;
            else if (strcmp(act, "win") == 0) decision.action = AI_ACTION_WIN;
            else decision.action = AI_ACTION_PASS;
        }
        
        cJSON *card_item = cJSON_GetObjectItem(target_root, "card");
        if (card_item && cJSON_IsNumber(card_item)) {
            decision.card = card_item->valueint;
        }

        cJSON *meld_cards = cJSON_GetObjectItem(target_root, "meld_cards");
        if (meld_cards && cJSON_IsArray(meld_cards)) {
            cJSON *mc1 = cJSON_GetArrayItem(meld_cards, 0);
            cJSON *mc2 = cJSON_GetArrayItem(meld_cards, 1);
            if (mc1) decision.meld_cards[0] = mc1->valueint;
            if (mc2) decision.meld_cards[1] = mc2->valueint;
        }
    }

    if (temp_root) cJSON_Delete(temp_root);
    cJSON_Delete(resp_root);
    return decision;
}

ai_decision_t ai_get_decision(ai_phase_t phase, int card, int from_seat) {
    ai_decision_t decision = {AI_ACTION_NONE, 0, {0, 0}};
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    struct memory chunk = {0};
    char *inner_json;
    char *final_payload = NULL;

    if (!ai_enabled) return decision;

    inner_json = ai_serialize_state(phase, card, from_seat);

    // Wrap the payload
    cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddStringToObject(wrapper, "appName", "agent");
    cJSON_AddStringToObject(wrapper, "userId", (char*)my_name);
    cJSON_AddStringToObject(wrapper, "sessionId", ai_session_id);
    
    cJSON *newMessage = cJSON_CreateObject();
    cJSON_AddStringToObject(newMessage, "role", "user");
    
    cJSON *parts = cJSON_CreateArray();
    cJSON *part = cJSON_CreateObject();
    cJSON_AddStringToObject(part, "text", inner_json);
    cJSON_AddItemToArray(parts, part);
    
    cJSON_AddItemToObject(newMessage, "parts", parts);
    cJSON_AddItemToObject(wrapper, "newMessage", newMessage);
    
    final_payload = cJSON_PrintUnformatted(wrapper);
    AI_LOG("[AI DEBUG] Request Payload: %s\n", final_payload);

    // Send Request
    curl = curl_easy_init();
    if (curl) {
        char run_url[300];
        snprintf(run_url, sizeof(run_url), "%s/run", ai_endpoint);
        AI_LOG("[AI DEBUG] Sending Decision Request to: %s\n", run_url);

        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, run_url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, final_payload);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        // SSL verification off for localhost
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        show_ai_thinking();
        res = curl_easy_perform(curl);
        hide_ai_thinking();

        cJSON *resp_json = NULL;
        const char *err = NULL;

        if (res == CURLE_OK) {
            AI_LOG("[AI DEBUG] Request Successful. Response size: %zu\n", chunk.size);
            if (chunk.response) {
                resp_json = cJSON_Parse(chunk.response);
                if (!resp_json) {
                    resp_json = cJSON_CreateString(chunk.response);
                }
            }
            decision = ai_parse_decision(chunk.response);
        } else {
            AI_ERR("[AI ERROR] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            err = curl_easy_strerror(res);
        }

        // Log decision trace
        // Create a copy of wrapper for logging, but replace the "text" string with the actual object
        cJSON *log_wrapper = cJSON_Duplicate(wrapper, 1);
        cJSON *l_nm = cJSON_GetObjectItem(log_wrapper, "newMessage");
        if (l_nm) {
            cJSON *l_parts = cJSON_GetObjectItem(l_nm, "parts");
            if (l_parts && cJSON_GetArraySize(l_parts) > 0) {
                cJSON *l_part0 = cJSON_GetArrayItem(l_parts, 0);
                if (l_part0) {
                    cJSON *parsed_inner = cJSON_Parse(inner_json);
                    if (parsed_inner) {
                        cJSON_ReplaceItemInObject(l_part0, "text", parsed_inner);
                    }
                }
            }
        }

        ai_send_log_to_server("decision", log_wrapper, resp_json, err);
        cJSON_Delete(log_wrapper);

        if (resp_json) cJSON_Delete(resp_json);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    
    cJSON_Delete(wrapper);
    free(inner_json); // Free the inner string as it's now in final_payload
    if (final_payload) free(final_payload);
    if (chunk.response) free(chunk.response);

    return decision;
}
