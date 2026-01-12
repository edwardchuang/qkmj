#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include "ai_client.h"
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

// Global context for 'run' shortcut
char last_phase[16] = "";
int last_card = -1;
int last_from = 0;

// Global mock state
void init_mock_state() {
    memset(&player, 0, sizeof(player));
    memset(&pool, 0, sizeof(pool));
    memset(&info, 0, sizeof(info));
    memset(table, 0, sizeof(table));
    memset(check_flag, 0, sizeof(check_flag));
    
    my_sit = 1;
    turn = 1;
    info.dealer = 1;
    info.wind = 0; // East
    
    strcpy((char*)my_name, "Sensei_Tester");
    strcpy(player[1].name, "Sensei_Tester");
    table[1] = 1; // Mark seat 1 as active in table mapping
}

void print_help() {
    printf("%sAvailable Commands:%s\n", CLR_HEADER, CLR_RESET);
    printf("  %shand <c1> <c2>...%s : Set your hand (0-33). Max 16.\n", CLR_INFO, CLR_RESET);
    printf("  %sname <new_name>%s   : Set player name.\n", CLR_INFO, CLR_RESET);
    printf("  %sflag <type> <0|1>%s  : Set flags (eat, pong, kang, win).\n", CLR_INFO, CLR_RESET);
    printf("  %srun [phase card from]%s: Run AI decision. Args optional if JSON loaded.\n", CLR_INFO, CLR_RESET);
    printf("  %s{...}%s               : Paste JSON state to update mock environment.\n", CLR_INFO, CLR_RESET);
    printf("  %sstatus%s            : Show current mock state and config.\n", CLR_INFO, CLR_RESET);
    printf("  %ssession%s           : Reset/Create AI session.\n", CLR_INFO, CLR_RESET);
    printf("  %sdebug <0|1>%s       : Toggle AI verbose debugging.\n", CLR_INFO, CLR_RESET);
    printf("  %sclear%s             : Reset mock state.\n", CLR_INFO, CLR_RESET);
    printf("  %shelp%s              : Show this message.\n", CLR_INFO, CLR_RESET);
    printf("  %sexit%s              : Quit.\n", CLR_INFO, CLR_RESET);
    printf("\n%sConfiguration Notes:%s\n", CLR_WARN, CLR_RESET);
    printf("  - %sAI_ENDPOINT%s: Set URL (default: http://localhost:8000).\n", CLR_INFO, CLR_RESET);
    printf("  - %sAI_TOKEN%s   : Set Bearer token for auth.\n", CLR_INFO, CLR_RESET);
    printf("  - Sensei uses the same logic as the main game to assign endpoints.\n");
    printf("  - If endpoint ends in ':query', Reasoning Engine mode is enabled.\n");
}

void print_status() {
    printf("%s--- Current Mock State ---%s\n", CLR_HEADER, CLR_RESET);
    printf("%sEndpoint:%s %s\n", CLR_INFO, CLR_RESET, ai_get_endpoint());
    printf("%sSeat:%s %d  %sDealer:%s %d  %sWind:%s %d\n", 
           CLR_INFO, CLR_RESET, my_sit, 
           CLR_INFO, CLR_RESET, info.dealer,
           CLR_INFO, CLR_RESET, info.wind);
    
    printf("%sHand (%d cards):%s ", CLR_INFO, pool[my_sit].num, CLR_RESET);
    for(int i=0; i<pool[my_sit].num; i++) printf("%d ", pool[my_sit].card[i]);
    printf("\n");
    
    printf("%sFlags:%s Eat:%d Pong:%d Kang:%d Win:%d\n", CLR_INFO, CLR_RESET,
           check_flag[my_sit][1], check_flag[my_sit][2], 
           check_flag[my_sit][3], check_flag[my_sit][4]);
    
    if (ai_is_enabled()) {
        const char* sid = ai_get_session_id();
        printf("%sAI Session:%s ACTIVE (ID: %s)\n", CLR_SUCCESS, CLR_RESET, sid[0] ? sid : "none");
    } else {
        printf("%sAI Session:%s INACTIVE (run 'session' to start)\n", CLR_WARN, CLR_RESET);
    }
    
    if (last_card != -1) {
        printf("%sLast Context:%s phase=%s card=%d from=%d\n", CLR_INFO, CLR_RESET, last_phase, last_card, last_from);
    }
    printf("---------------------------\n");
}

void handle_debug(char* line) {
    int val = 0;
    if (sscanf(line, "debug %d", &val) == 1) {
        if (val) {
            setenv("AI_DEBUG", "1", 1);
            printf("%sAI Debugging ENABLED.%s\n", CLR_SUCCESS, CLR_RESET);
        } else {
            setenv("AI_DEBUG", "0", 1);
            printf("%sAI Debugging DISABLED.%s\n", CLR_WARN, CLR_RESET);
        }
        ai_init(); // Re-init to pick up env var
    } else {
        printf("%sUsage: debug <0|1>%s\n", CLR_WARN, CLR_RESET);
    }
}

void print_json(const char* label, const char* json_str) {
    if (!json_str) return;
    cJSON *root = cJSON_Parse(json_str);
    if (root) {
        char *rendered = cJSON_Print(root);
        printf("%s%s:%s\n%s%s%s\n", CLR_INFO, label, CLR_RESET, CLR_JSON, rendered, CLR_RESET);
        free(rendered);
        cJSON_Delete(root);
    } else {
        printf("%s%s (Raw):%s %s\n", CLR_INFO, label, CLR_RESET, json_str);
    }
}

void handle_run(char* line) {
    char phase_str[16] = "";
    int card = -1;
    int from_seat = 0;
    int n = sscanf(line, "run %s %d %d", phase_str, &card, &from_seat);
    
    if (n < 2) {
        // Try to use last context
        if (last_card != -1) {
            strcpy(phase_str, last_phase);
            card = last_card;
            from_seat = last_from;
            printf("%sUsing last context: %s %d %d%s\n", CLR_INFO, phase_str, card, from_seat, CLR_RESET);
        } else {
            printf("%sUsage: run <discard|claim> <card> [from]%s\n", CLR_WARN, CLR_RESET);
            return;
        }
    }

    ai_phase_t phase;
    if (strcmp(phase_str, "discard") == 0) phase = AI_PHASE_DISCARD;
    else if (strcmp(phase_str, "claim") == 0) phase = AI_PHASE_CLAIM;
    else {
        printf("%sInvalid phase '%s'. Use 'discard' or 'claim'.%s\n", CLR_ERROR, phase_str, CLR_RESET);
        return;
    }

    if (!ai_is_enabled()) {
        printf("%sAI is not enabled. Run 'session' first.%s\n", CLR_WARN, CLR_RESET);
        return;
    }

    // Show what we are sending
    char *state = ai_serialize_state(phase, card, from_seat);
    print_json("Request Payload", state);
    free(state);

    printf("%sWaiting for AI...%s\n", CLR_INFO, CLR_RESET);
    ai_decision_t d = ai_get_decision(phase, card, from_seat);

    const char* actions[] = {"NONE", "DISCARD", "EAT", "PONG", "KANG", "WIN", "PASS"};
    printf("\n%s>>> AI DECISION: %s (card %d)%s\n", CLR_SUCCESS, actions[d.action], d.card, CLR_RESET);
    if (d.action == AI_ACTION_EAT) {
        printf("%s>>> Meld cards: %d, %d%s\n", CLR_SUCCESS, d.meld_cards[0], d.meld_cards[1], CLR_RESET);
    }
}

void handle_json(const char* json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        printf("%sFailed to parse JSON state.%s\n", CLR_ERROR, CLR_RESET);
        return;
    }

    // Update Seat
    cJSON *my_seat = cJSON_GetObjectItem(root, "my_seat");
    if (cJSON_IsNumber(my_seat)) {
        my_sit = my_seat->valueint;
        printf("%sMock Seat -> %d%s\n", CLR_INFO, my_sit, CLR_RESET);
    }

    // Update Hand
    cJSON *hand = cJSON_GetObjectItem(root, "hand");
    if (cJSON_IsArray(hand)) {
        pool[my_sit].num = 0;
        int sz = cJSON_GetArraySize(hand);
        for (int i = 0; i < sz && i < 16; i++) {
            cJSON *c = cJSON_GetArrayItem(hand, i);
            if (cJSON_IsNumber(c)) pool[my_sit].card[pool[my_sit].num++] = c->valueint;
        }
        printf("%sMock Hand -> Updated (%d cards)%s\n", CLR_INFO, pool[my_sit].num, CLR_RESET);
    }

    // Update Legal Actions
    cJSON *actions = cJSON_GetObjectItem(root, "legal_actions");
    if (cJSON_IsObject(actions)) {
        const char* keys[] = {"can_eat", "can_pong", "can_kang", "can_win"};
        for (int i = 0; i < 4; i++) {
            cJSON *a = cJSON_GetObjectItem(actions, keys[i]);
            if (cJSON_IsBool(a)) check_flag[my_sit][i+1] = cJSON_IsTrue(a) ? 1 : 0;
        }
        printf("%sMock Flags -> Updated%s\n", CLR_INFO, CLR_RESET);
    }

    // Capture context for 'run'
    cJSON *phase = cJSON_GetObjectItem(root, "phase");
    cJSON *new_card = cJSON_GetObjectItem(root, "new_card");
    if (cJSON_IsString(phase) && cJSON_IsNumber(new_card)) {
        strncpy(last_phase, phase->valuestring, sizeof(last_phase)-1);
        last_card = new_card->valueint;
        last_from = 0;
        cJSON *fs = cJSON_GetObjectItem(root, "from_seat");
        if (cJSON_IsNumber(fs)) last_from = fs->valueint;
        
        printf("%sContext Captured: %s card %d (from %d). Type 'run' to test this state.%s\n", 
               CLR_WARN, last_phase, last_card, last_from, CLR_RESET);
    }

    cJSON_Delete(root);
}

void handle_hand(char* line) {
    char *p = line + 4; // skip "hand"
    pool[my_sit].num = 0;
    while (*p) {
        while (*p && !isdigit(*p)) p++;
        if (!*p) break;
        if (pool[my_sit].num >= 16) break;
        pool[my_sit].card[pool[my_sit].num++] = atoi(p);
        while (*p && isdigit(*p)) p++;
    }
    printf("%sHand updated.%s\n", CLR_SUCCESS, CLR_RESET);
}

void handle_flag(char* line) {
    char type[16];
    int val = 0;
    if (sscanf(line, "flag %s %d", type, &val) < 2) {
        printf("%sUsage: flag <eat|pong|kang|win> <0|1>%s\n", CLR_WARN, CLR_RESET);
        return;
    }
    
    if (strcmp(type, "eat") == 0) check_flag[my_sit][1] = val;
    else if (strcmp(type, "pong") == 0) check_flag[my_sit][2] = val;
    else if (strcmp(type, "kang") == 0) check_flag[my_sit][3] = val;
    else if (strcmp(type, "win") == 0) check_flag[my_sit][4] = val;
    else printf("%sUnknown flag type.%s\n", CLR_ERROR, CLR_RESET);
}

void handle_name(char* line) {
    char new_name[50];
    if (sscanf(line, "name %s", new_name) == 1) {
        strncpy((char*)my_name, new_name, sizeof(my_name)-1);
        strncpy(player[my_sit].name, new_name, sizeof(player[my_sit].name)-1);
        printf("%sPlayer name updated to: %s%s\n", CLR_SUCCESS, new_name, CLR_RESET);
    } else {
        printf("%sUsage: name <new_name>%s\n", CLR_WARN, CLR_RESET);
    }
}

int main() {
    char line[4096]; // Larger buffer for JSON pastes
    
    printf("%s==========================================%s\n", CLR_HEADER, CLR_RESET);
    printf("%s           QKMJ AI SENSEI (Shell)         %s\n", CLR_HEADER, CLR_RESET);
    printf("%s==========================================%s\n", CLR_HEADER, CLR_RESET);
    printf("Type 'help' for commands.\n\n");

    init_mock_state();
    ai_init();

    while (1) {
        printf("%ssensei> %s", CLR_PROMPT, CLR_RESET);
        if (!fgets(line, sizeof(line), stdin)) break;
        
        // Strip newline
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) continue;
        
        if (line[0] == '{') handle_json(line);
        else if (strncmp(line, "help", 4) == 0) print_help();
        else if (strncmp(line, "status", 6) == 0) print_status();
        else if (strncmp(line, "exit", 4) == 0 || strncmp(line, "quit", 4) == 0) break;
        else if (strncmp(line, "hand", 4) == 0) handle_hand(line);
        else if (strncmp(line, "name", 4) == 0) handle_name(line);
        else if (strncmp(line, "flag", 4) == 0) handle_flag(line);
        else if (strncmp(line, "run", 3) == 0) handle_run(line);
        else if (strncmp(line, "debug", 5) == 0) handle_debug(line);
        else if (strncmp(line, "session", 7) == 0) {
            ai_set_enabled(1);
            if (ai_is_enabled()) {
                printf("%sAI session initialized successfully.%s\n", CLR_SUCCESS, CLR_RESET);
            } else {
                printf("%sAI session initialization FAILED.%s\n", CLR_ERROR, CLR_RESET);
            }
        }
        else if (strncmp(line, "clear", 5) == 0) {
            init_mock_state();
            last_card = -1;
            printf("%sState reset.%s\n", CLR_SUCCESS, CLR_RESET);
        }
        else {
            printf("%sUnknown command: %s. Type 'help' for list.%s\n", CLR_WARN, line, CLR_RESET);
        }
    }

    ai_cleanup();
    printf("Goodbye!\n");
    return 0;
}
