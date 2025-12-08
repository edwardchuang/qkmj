#include "ai_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "qkmj.h"

static int ai_enabled = 0;
static char ai_endpoint[256] = "https://localhost:8888/ask";

struct memory {
  char *response;
  size_t size;
};

static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;
 
  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(ptr == NULL) return 0;  /* out of memory */
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
 
  return realsize;
}

void ai_init() {
    char *env_mode = getenv("AI_MODE");
    char *env_endpoint = getenv("AI_ENDPOINT");

    if (env_mode && strcmp(env_mode, "auto") == 0) {
        ai_enabled = 1;
    }
    
    if (env_endpoint) {
        strncpy(ai_endpoint, env_endpoint, sizeof(ai_endpoint) - 1);
        ai_endpoint[sizeof(ai_endpoint) - 1] = '\0';
    }

    curl_global_init(CURL_GLOBAL_ALL);
}

void ai_cleanup() {
    curl_global_cleanup();
}

int ai_is_enabled() {
    return ai_enabled;
}

void ai_set_enabled(int enabled) {
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

    // Exposed melds (from out_card but need parsing logic? pool[].out_card structure)
    // qkmj stores melds in out_card? Yes. 
    // Structure: [type, c1, c2, c3, c4]
    // type: 1 (chi), 2 (pong), 3 (ming-kang), 11 (an-kang), 12 (jia-kang)
    meld_array = cJSON_CreateArray();
    for (i = 0; i < pool[seat_idx].out_card_index; i++) {
        cJSON *meld = cJSON_CreateObject();
        int type = pool[seat_idx].out_card[i][0];
        // Filter legitimate melds
        if (type == 1 || type == 2 || type == 3 || type == 11 || type == 12) {
             cJSON_AddNumberToObject(meld, "type", type);
             cJSON_AddNumberToObject(meld, "card", pool[seat_idx].out_card[i][1]); // Representative card
             cJSON_AddItemToArray(meld_array, meld);
        }
    }
    cJSON_AddItemToObject(player_obj, "melds", meld_array);

    // Discards
    discard_array = cJSON_CreateArray();
    for (i = 0; i < pool[seat_idx].out_card_index; i++) {
        int type = pool[seat_idx].out_card[i][0];
        // 7,8,9 are discards? 
        // In qkmj:
        // Normal discard is just put in table_card? No, check `throw_card`.
        // It sets `table_card`.
        // `pool` seems to track exposed cards (melds/flowers/discards?) 
        // Let's assume `out_card` stores melds.
        // The river is tracked in `pool` too? 
        // `pool[sit].card` is HAND.
        // Discards are tricky in this codebase. They might be transient or stored in `table_card` visual buffer.
        // For AI, knowing river is important.
        // Re-reading `qkmj.c` `throw_card`: it updates `table_card`.
        // Recovering the exact sequence of discards from `table_card` is hard because it's a 2D grid.
        // We might skip full discard history for this iteration or try to parse `table_card`.
    }
    cJSON_AddItemToObject(player_obj, "discards", discard_array); // Empty for now

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
        cJSON_AddBoolToObject(legal, "can_win", 0); // TODO: Check valid win
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
    if (resp_root) {
        cJSON *action_item = cJSON_GetObjectItem(resp_root, "action");
        if (action_item && cJSON_IsString(action_item)) {
            const char *act = action_item->valuestring;
            if (strcmp(act, "discard") == 0) decision.action = AI_ACTION_DISCARD;
            else if (strcmp(act, "eat") == 0) decision.action = AI_ACTION_EAT;
            else if (strcmp(act, "pong") == 0) decision.action = AI_ACTION_PONG;
            else if (strcmp(act, "kang") == 0) decision.action = AI_ACTION_KANG;
            else if (strcmp(act, "win") == 0) decision.action = AI_ACTION_WIN;
            else decision.action = AI_ACTION_PASS;
        }
        
        cJSON *card_item = cJSON_GetObjectItem(resp_root, "card");
        if (card_item && cJSON_IsNumber(card_item)) {
            decision.card = card_item->valueint;
        }

        // Handle meld_cards for eat if needed
        cJSON *meld_cards = cJSON_GetObjectItem(resp_root, "meld_cards");
        if (meld_cards && cJSON_IsArray(meld_cards)) {
            cJSON *mc1 = cJSON_GetArrayItem(meld_cards, 0);
            cJSON *mc2 = cJSON_GetArrayItem(meld_cards, 1);
            if (mc1) decision.meld_cards[0] = mc1->valueint;
            if (mc2) decision.meld_cards[1] = mc2->valueint;
        }

        cJSON_Delete(resp_root);
    }
    return decision;
}

ai_decision_t ai_get_decision(ai_phase_t phase, int card, int from_seat) {
    ai_decision_t decision = {AI_ACTION_NONE, 0, {0, 0}};
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    struct memory chunk = {0};
    char *json_payload;

    if (!ai_enabled) return decision;

    json_payload = ai_serialize_state(phase, card, from_seat);

    // Send Request
    curl = curl_easy_init();
    if (curl) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, ai_endpoint);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        // SSL verification off for localhost
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            decision = ai_parse_decision(chunk.response);
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    
    free(json_payload);
    free(chunk.response);

    return decision;
}
