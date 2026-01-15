#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include <mongoc/mongoc.h>
#include "mongo.h"
#include "qkmj.h"

// Colors
#define CLR_RESET  "\x1b[0m"
#define CLR_HEADER "\x1b[1;34m"
#define CLR_PROMPT "\x1b[1;32m"
#define CLR_INFO   "\x1b[1;36m"
#define CLR_SUCCESS "\x1b[1;32m"
#define CLR_WARN   "\x1b[1;33m"
#define CLR_ERROR  "\x1b[1;31m"
#define CLR_JSON   "\x1b[0;37m"

static int debug_enabled = 0;

// Global Match ID for display
extern char current_match_id[64];

// Mahjong Card Mapping
static const char* get_card_name(int card) {
    static char buf[32];
    static const char* zh_num[] = {"零", "一", "二", "三", "四", "五", "六", "七", "八", "九"};
    static const char* flower_names[] = {"春１", "夏２", "秋３", "冬４", "梅１", "蘭２", "菊３", "竹４"};
    
    switch(card) {
        case 0: return "??";
        case 10: return "摸牌";
        case 20: return "＊＊";
        case 30: return "東"; case 40: return "南"; case 50: return "西"; case 60: return "北";
        case 31: return "東風"; case 32: return "南風"; case 33: return "西風"; case 34: return "北風";
        case 41: return "紅中"; case 42: return "白板"; case 43: return "青發";
    }
    if (card >= 1 && card <= 9) { sprintf(buf, "%s萬", zh_num[card]); return buf; }
    if (card >= 11 && card <= 19) { sprintf(buf, "%s索", zh_num[card-10]); return buf; }
    if (card >= 21 && card <= 29) { sprintf(buf, "%s筒", zh_num[card-20]); return buf; }
    if (card >= 51 && card <= 58) { return flower_names[card-51]; }
    sprintf(buf, "%d", card);
    return buf;
}

mongoc_collection_t* get_logs_collection(mongoc_client_t **client_out) {
    const char *uri_str = getenv("MONGO_URI");
    if (!uri_str) uri_str = "mongodb://localhost:27017";
    
    mongoc_client_t *client = mongoc_client_new(uri_str);
    if (!client) return NULL;
    
    *client_out = client;
    return mongoc_client_get_collection(client, MONGO_DB_NAME, MONGO_COLLECTION_LOGS);
}

bson_t* cjson_to_bson(cJSON *json) {
    char *str = cJSON_PrintUnformatted(json);
    bson_error_t error;
    bson_t *bson = bson_new_from_json((const uint8_t *)str, -1, &error);
    if (!bson) {
        fprintf(stderr, "%sBSON Error: %s%s\n", CLR_ERROR, error.message, CLR_RESET);
    }
    free(str);
    return bson;
}

void sanitize_mongo_json(cJSON *item) {
    cJSON *curr = item;
    while (curr) {
        if (curr->type == cJSON_Object) {
            cJSON *numInt = cJSON_GetObjectItem(curr, "$numberInt");
            if (numInt && cJSON_IsString(numInt)) {
                int val = atoi(numInt->valuestring);
                cJSON *tmp = cJSON_DetachItemFromObject(curr, "$numberInt");
                cJSON_Delete(tmp);
                
                curr->type = cJSON_Number;
                curr->valueint = val;
                curr->valuedouble = (double)val;
            } else {
                cJSON *numLong = cJSON_GetObjectItem(curr, "$numberLong");
                if (numLong && cJSON_IsString(numLong)) {
                    long long val = atoll(numLong->valuestring);
                    cJSON *tmp = cJSON_DetachItemFromObject(curr, "$numberLong");
                    cJSON_Delete(tmp);
                    
                    curr->type = cJSON_Number;
                    curr->valueint = (int)val;
                    curr->valuedouble = (double)val;
                } else {
                    sanitize_mongo_json(curr->child);
                }
            }
        } else if (curr->type == cJSON_Array) {
            sanitize_mongo_json(curr->child);
        }
        curr = curr->next;
    }
}

void kifu_list() {
    mongoc_client_t *client;
    bson_error_t error;
    mongoc_collection_t *collection = get_logs_collection(&client);
    if (!collection) {
        printf("%sFailed to connect to MongoDB.%s\n", CLR_ERROR, CLR_RESET);
        return;
    }

    cJSON *pipe = cJSON_CreateArray();
    cJSON *match = cJSON_CreateObject();
    cJSON *match_q = cJSON_CreateObject();
    cJSON_AddStringToObject(match_q, "level", "game_trace");
    
    // Filter out DIAG actions so they don't count as moves
    cJSON *action_filter = cJSON_CreateObject();
    cJSON *nin_list = cJSON_CreateArray();
    cJSON_AddItemToArray(nin_list, cJSON_CreateString("DEADLOCK_DIAG"));
    cJSON_AddItemToArray(nin_list, cJSON_CreateString("CLIENT_STALL_DIAG"));
    cJSON_AddItemToObject(action_filter, "$nin", nin_list);
    cJSON_AddItemToObject(match_q, "action", action_filter);

    cJSON_AddItemToObject(match, "$match", match_q);
    cJSON_AddItemToArray(pipe, match);

    cJSON *group = cJSON_CreateObject();
    cJSON *group_q = cJSON_CreateObject();
    cJSON_AddStringToObject(group_q, "_id", "$match_id");
    cJSON *st = cJSON_CreateObject(); cJSON_AddStringToObject(st, "$min", "$timestamp"); cJSON_AddItemToObject(group_q, "start_time", st);
    cJSON *mv = cJSON_CreateObject(); cJSON_AddNumberToObject(mv, "$sum", 1); cJSON_AddItemToObject(group_q, "moves", mv);
    cJSON *pl = cJSON_CreateObject(); cJSON_AddStringToObject(pl, "$addToSet", "$user_id"); cJSON_AddItemToObject(group_q, "players", pl);
    cJSON_AddItemToObject(group, "$group", group_q);
    cJSON_AddItemToArray(pipe, group);

    cJSON *sort = cJSON_CreateObject();
    cJSON *sort_q = cJSON_CreateObject();
    cJSON_AddNumberToObject(sort_q, "start_time", -1);
    cJSON_AddItemToObject(sort, "$sort", sort_q);
    cJSON_AddItemToArray(pipe, sort);

    cJSON *limit = cJSON_CreateObject();
    cJSON_AddNumberToObject(limit, "$limit", 20);
    cJSON_AddItemToArray(pipe, limit);

    bson_t *bson_pipe = cjson_to_bson(pipe);
    cJSON_Delete(pipe);

    mongoc_cursor_t *cursor = mongoc_collection_aggregate(collection, MONGOC_QUERY_NONE, bson_pipe, NULL, NULL);
    const bson_t *doc;

    printf("\n%s%-20s | %-20s | %-6s | %s%s\n", CLR_HEADER, "Match ID", "Start Time", "Moves", "Players", CLR_RESET);
    printf("--------------------------------------------------------------------------------\n");

    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        const char *mid = "unknown";
        int64_t ts = 0;
        int64_t moves = 0;

        if (bson_iter_init_find(&iter, doc, "_id") && BSON_ITER_HOLDS_UTF8(&iter)) mid = bson_iter_utf8(&iter, NULL);
        if (bson_iter_init_find(&iter, doc, "start_time")) ts = bson_iter_as_int64(&iter);
        if (bson_iter_init_find(&iter, doc, "moves")) moves = bson_iter_as_int64(&iter);

        time_t t = (time_t)(ts / 1000);
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&t));

        printf("%s | %s | %-6lld | ", mid, time_buf, (long long)moves); 
        
        if (bson_iter_init_find(&iter, doc, "players") && BSON_ITER_HOLDS_ARRAY(&iter)) {
            const uint8_t *data;
            uint32_t len;
            bson_iter_array(&iter, &len, &data);
            bson_t *players_bson = bson_new_from_data(data, len);
            bson_iter_t p_iter;
            int first = 1;
            if (bson_iter_init(&p_iter, players_bson)) {
                while (bson_iter_next(&p_iter)) {
                    if (!first) printf(", ");
                    printf("%s", bson_iter_utf8(&p_iter, NULL));
                    first = 0;
                }
            }
            bson_destroy(players_bson);
        }
        printf("\n");
    }

    if (mongoc_cursor_error(cursor, &error)) {
        printf("%sCursor Error: %s%s\n", CLR_ERROR, error.message, CLR_RESET);
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(bson_pipe);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
}

void kifu_dump(const char* match_id, int target_serial) {
    mongoc_client_t *client;
    mongoc_collection_t *collection = get_logs_collection(&client);
    if (!collection) return;

    cJSON *filter = cJSON_CreateObject();
    cJSON_AddStringToObject(filter, "match_id", match_id);
    cJSON_AddStringToObject(filter, "level", "game_trace");
    
    // We cannot filter by "move_serial" directly because diagnostic logs 
    // consume serial numbers but are hidden in the UI, causing a skew.
    // We must fetch all logs and count valid moves manually.
    
    cJSON *opts = cJSON_CreateObject();
    cJSON *sort = cJSON_CreateObject();
    cJSON_AddNumberToObject(sort, "move_serial", 1);
    cJSON_AddItemToObject(opts, "sort", sort);

    bson_t *b_filter = cjson_to_bson(filter);
    bson_t *b_opts = cjson_to_bson(opts);
    
    cJSON_Delete(filter);
    cJSON_Delete(opts);

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, b_filter, b_opts, NULL);
    const bson_t *doc;
    int found = 0;
    int logical_serial = 0;

    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        const char *action = "";

        // Check action type to filter diagnostics
        if (bson_iter_init_find(&iter, doc, "action") && BSON_ITER_HOLDS_UTF8(&iter)) {
            action = bson_iter_utf8(&iter, NULL);
        }

        if (strcmp(action, "DEADLOCK_DIAG") == 0 || strcmp(action, "CLIENT_STALL_DIAG") == 0) {
            continue; // Skip diagnostic logs to align with 'list'/'show' counts
        }

        logical_serial++;

        // If searching for specific move, skip until we reach it
        if (target_serial > 0 && logical_serial != target_serial) {
            continue;
        }
        
        if (bson_iter_init_find(&iter, doc, "state") && BSON_ITER_HOLDS_DOCUMENT(&iter)) {
            const uint8_t *data;
            uint32_t len;
            bson_iter_document(&iter, &len, &data);
            bson_t *state_bson = bson_new_from_data(data, len);
            
            char *json_extended = bson_as_canonical_extended_json(state_bson, NULL);
            if (json_extended) {
                // Convert Extended JSON to Standard JSON
                cJSON *root = cJSON_Parse(json_extended);
                if (root) {
                    sanitize_mongo_json(root); // Fix $numberInt
                    char *json_standard = cJSON_PrintUnformatted(root);
                    if (json_standard) {
                        printf("%s\n", json_standard);
                        free(json_standard);
                        found++;
                    }
                    cJSON_Delete(root);
                } else {
                    // Fallback (shouldn't happen if BSON is valid)
                    printf("%s\n", json_extended);
                    found++;
                }
                bson_free(json_extended);
            }
            bson_destroy(state_bson);
        }

        // If we found the specific move we wanted, stop
        if (target_serial > 0) break;
    }

    if (target_serial > 0 && found == 0) {
         fprintf(stderr, "%sMove #%d not found in match %s%s\n", CLR_ERROR, target_serial, match_id, CLR_RESET);
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(b_filter);
    bson_destroy(b_opts);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
}

void kifu_show(const char* match_id) {
    mongoc_client_t *client;
    mongoc_collection_t *collection = get_logs_collection(&client);
    if (!collection) return;

    cJSON *filter = cJSON_CreateObject();
    cJSON_AddStringToObject(filter, "match_id", match_id);
    
    cJSON *opts = cJSON_CreateObject();
    cJSON *sort = cJSON_CreateObject();
    cJSON_AddNumberToObject(sort, "_id", 1);
    cJSON_AddItemToObject(opts, "sort", sort);

    bson_t *b_filter = cjson_to_bson(filter);
    bson_t *b_opts = cjson_to_bson(opts);
    
    cJSON_Delete(filter);
    cJSON_Delete(opts);

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, b_filter, b_opts, NULL);
    const bson_t *doc;

    printf("\n%s--- Match Record: %s ---%s\n", CLR_HEADER, match_id, CLR_RESET);

    int count = 0;
    int last_wind = -1;
    int last_dealer = -1;

    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        const char *level = "";
        
        if (bson_iter_init_find(&iter, doc, "level") && BSON_ITER_HOLDS_UTF8(&iter)) {
            level = bson_iter_utf8(&iter, NULL);
        }

        if (strcmp(level, "game_record") == 0) {
            printf("\n%s--- Round Result ---%s\n", CLR_SUCCESS, CLR_RESET);
            char *json = bson_as_canonical_extended_json(doc, NULL);
            cJSON *res = cJSON_Parse(json);
            if (res) {
                cJSON *data_obj = cJSON_GetObjectItem(res, "data");
                if (!data_obj) data_obj = res; 

                const char *winner = cJSON_GetStringValue(cJSON_GetObjectItem(data_obj, "winer"));
                const char *loser = cJSON_GetStringValue(cJSON_GetObjectItem(data_obj, "card_owner"));
                
                if (winner) {
                    if (loser && strcmp(winner, loser) == 0) {
                        printf("%sWinner: %s (Self-Drawn 自摸)%s\n", CLR_SUCCESS, winner, CLR_RESET);
                    } else {
                        printf("%sWinner: %s%s | %sLoser: %s (Discarder 放槍)%s\n", 
                               CLR_SUCCESS, winner, CLR_RESET, CLR_ERROR, loser ? loser : "Unknown", CLR_RESET);
                    }
                }

                const char *tais = cJSON_GetStringValue(cJSON_GetObjectItem(data_obj, "tais"));
                if (tais) printf("Tai: %s\n", tais);

                int base = 0, tai_val = 0;
                if (cJSON_HasObjectItem(data_obj, "base_value")) base = cJSON_GetNumberValue(cJSON_GetObjectItem(data_obj, "base_value"));
                if (cJSON_HasObjectItem(data_obj, "tai_value")) tai_val = cJSON_GetNumberValue(cJSON_GetObjectItem(data_obj, "tai_value"));
                printf("Config: Base=%d, Tai=%d\n", base, tai_val);

                cJSON *moneys = cJSON_GetObjectItem(data_obj, "moneys");
                if (cJSON_IsArray(moneys)) {
                    printf("Money Changes:\n");
                    int sz = cJSON_GetArraySize(moneys);
                    for (int i=0; i<sz; i++) {
                        cJSON *m = cJSON_GetArrayItem(moneys, i);
                        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(m, "name"));
                        int change = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(m, "change_money"));
                        printf("  %-10s : %s%d%s\n", name, (change >= 0 ? CLR_SUCCESS : CLR_ERROR), change, CLR_RESET);
                    }
                }
                cJSON_Delete(res);
            }
            bson_free(json);
            continue; 
        }

        if (strcmp(level, "game_trace") != 0) continue;

        int serial = 0;
        const char *user = "Unknown";
        const char *action = "None";
        int card = 0;
        bool is_ai = false;
        int wind = -1;
        int dealer = -1;

        serial = count + 1;
        
        bson_iter_t sub_iter;
        if (bson_iter_init_find(&iter, doc, "user_id") && BSON_ITER_HOLDS_UTF8(&iter)) user = bson_iter_utf8(&iter, NULL);
        if (bson_iter_init_find(&iter, doc, "action") && BSON_ITER_HOLDS_UTF8(&iter)) action = bson_iter_utf8(&iter, NULL);
        if (bson_iter_init_find(&iter, doc, "card") && BSON_ITER_HOLDS_INT32(&iter)) card = bson_iter_int32(&iter);
        if (bson_iter_init_find(&iter, doc, "is_ai") && BSON_ITER_HOLDS_BOOL(&iter)) is_ai = bson_iter_bool(&iter);
        
        bson_iter_init(&iter, doc);
        if (bson_iter_find_descendant(&iter, "state.context.round_wind", &sub_iter) && BSON_ITER_HOLDS_INT32(&sub_iter)) {
            wind = bson_iter_int32(&sub_iter);
        }
        
        bson_iter_init(&iter, doc);
        if (bson_iter_find_descendant(&iter, "state.context.dealer", &sub_iter) && BSON_ITER_HOLDS_INT32(&sub_iter)) {
            dealer = bson_iter_int32(&sub_iter);
        }

        if (wind != last_wind || dealer != last_dealer) {
            const char* wind_names[] = {"?", "東", "南", "西", "北"};
            printf("\n%s----------------------------------------%s\n", CLR_INFO, CLR_RESET);
            printf("%s>>> Round: %s風 %s家開莊 <<<%s\n", CLR_INFO, 
                   (wind >= 1 && wind <= 4) ? wind_names[wind] : "?",
                   (dealer >= 1 && dealer <= 4) ? wind_names[dealer] : "?",
                   CLR_RESET);
            last_wind = wind;
            last_dealer = dealer;
        }

        bool is_diag = (strcmp(action, "DEADLOCK_DIAG") == 0 || strcmp(action, "CLIENT_STALL_DIAG") == 0);
        if (is_diag && !debug_enabled) continue;

        const char *act_color = "";
        const char *act_reset = "";
        if (strcmp(action, "Win") == 0) {
            act_color = CLR_SUCCESS;
            act_reset = CLR_RESET;
        }

        printf("#%-3d %-10s %-4s | %s%-8s%s | %-4s", 
               serial, user, is_ai ? "[AI]" : "", 
               act_color, action, act_reset,
               get_card_name(card));

        if ((strcmp(action, "Pass") == 0 || is_diag) && bson_iter_init_find(&iter, doc, "extra") && BSON_ITER_HOLDS_DOCUMENT(&iter)) {
            const uint8_t *data;
            uint32_t len;
            bson_iter_document(&iter, &len, &data);
            bson_t *extra = bson_new_from_data(data, len);
            bson_iter_t e_iter;
            
            if (is_diag) {
                printf("\n      %s[DIAG] %s", CLR_WARN, CLR_RESET);
                char *json = bson_as_canonical_extended_json(extra, NULL);
                printf("%s%s", json, CLR_RESET);
                bson_free(json);
            } else {
                printf(" (Passed: ");
                int first = 1;
                if (bson_iter_init(&e_iter, extra)) {
                    while (bson_iter_next(&e_iter)) {
                        if (BSON_ITER_HOLDS_BOOL(&e_iter) && bson_iter_bool(&e_iter)) {
                            if (!first) printf(", ");
                            printf("%s", bson_iter_key(&e_iter));
                            first = 0;
                        }
                    }
                }
                printf(")");
            }
            bson_destroy(extra);
        }
        printf("\n");
        count++;
    }
    mongoc_cursor_destroy(cursor);
    bson_destroy(b_filter);
    bson_destroy(b_opts);

    // Removed the secondary query block that was here
    if (count == 0) {
        printf("%sNo logs found for this Match ID.%s\n", CLR_WARN, CLR_RESET);
    }

    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
}

void print_help() {
    printf("%sKifu Tool Commands:%s\n", CLR_HEADER, CLR_RESET);
    printf("  %slist%s            : List 20 most recent matches\n", CLR_INFO, CLR_RESET);
    printf("  %sshow <id>%s       : Show move history and final result\n", CLR_INFO, CLR_RESET);
    printf("  %sdump <id> [mv]%s  : Dump AI-ready JSON for all moves or specific move #\n", CLR_INFO, CLR_RESET);
    printf("  %sdebug <on|off>%s  : Toggle diagnostic logs (DEADLOCK/STALL)\n", CLR_INFO, CLR_RESET);
    printf("  %shelp%s            : Show this message\n", CLR_INFO, CLR_RESET);
    printf("  %sexit%s            : Exit tool\n", CLR_INFO, CLR_RESET);
}

int main() {
    char line[1024];
    
    printf("%s==========================================%s\n", CLR_HEADER, CLR_RESET);
    printf("%s           QKMJ KIFU BROWSER              %s\n", CLR_HEADER, CLR_RESET);
    printf("%s==========================================%s\n", CLR_HEADER, CLR_RESET);
    
    mongoc_init();

    while (1) {
        printf("%skifu> %s", CLR_PROMPT, CLR_RESET);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        if (strncmp(line, "list", 4) == 0) {
            kifu_list();
        } else if (strncmp(line, "show ", 5) == 0) {
            kifu_show(line + 5);
        } else if (strncmp(line, "dump ", 5) == 0) {
            char *args = line + 5;
            char *id = strtok(args, " ");
            char *mv = strtok(NULL, " ");
            if (id) {
                kifu_dump(id, mv ? atoi(mv) : 0);
            } else {
                printf("%sUsage: dump <match_id> [move_serial]%s\n", CLR_WARN, CLR_RESET);
            }
        } else if (strncmp(line, "debug ", 6) == 0) {
            if (strstr(line, "on")) {
                debug_enabled = 1;
                printf("%sDebug mode ENABLED (showing diagnostic logs).%s\n", CLR_SUCCESS, CLR_RESET);
            } else {
                debug_enabled = 0;
                printf("%sDebug mode DISABLED (hiding diagnostic logs).%s\n", CLR_WARN, CLR_RESET);
            }
        } else if (strncmp(line, "help", 4) == 0) {
            print_help();
        } else if (strncmp(line, "exit", 4) == 0 || strncmp(line, "quit", 4) == 0) {
            break;
        } else {
            printf("%sUnknown command. Type 'help'.%s\n", CLR_WARN, CLR_RESET);
        }
    }

    mongoc_cleanup();
    return 0;
}