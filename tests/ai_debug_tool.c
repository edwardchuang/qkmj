#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "ai_client.h"
#include "qkmj.h"

// Colors
#define CLR_RESET  "\x1b[0m"
#define CLR_HEADER "\x1b[1;34m"
#define CLR_INFO   "\x1b[1;36m"
#define CLR_SUCCESS "\x1b[1;32m"
#define CLR_WARN   "\x1b[1;33m"
#define CLR_ERROR  "\x1b[1;31m"
#define CLR_JSON   "\x1b[0;37m"

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

void print_decision(ai_decision_t d) {
    const char* actions[] = {"NONE", "DISCARD", "EAT", "PONG", "KANG", "WIN", "PASS"};
    const char* color = (d.action == AI_ACTION_NONE || d.action == AI_ACTION_PASS) ? CLR_WARN : CLR_SUCCESS;

    printf("\n%s╔══════════════════════════════════════╗%s\n", color, CLR_RESET);
    printf("%s║             AI DECISION              ║%s\n", color, CLR_RESET);
    printf("%s╠══════════════════════════════════════╣%s\n", color, CLR_RESET);
    printf("%s║  %-10s : %-20s ║%s\n", color, "ACTION", actions[d.action], CLR_RESET);
    printf("%s║  %-10s : %-20d ║%s\n", color, "CARD", d.card, CLR_RESET);
    if (d.action == AI_ACTION_EAT) {
        char meld[20];
        snprintf(meld, sizeof(meld), "[%d, %d]", d.meld_cards[0], d.meld_cards[1]);
        printf("%s║  %-10s : %-20s ║%s\n", color, "MELD", meld, CLR_RESET);
    }
    printf("%s╚══════════════════════════════════════╝%s\n\n", color, CLR_RESET);
}

int main(int argc, char** argv) {
    printf("%s==========================================%s\n", CLR_HEADER, CLR_RESET);
    printf("%s           QKMJ AI LIVE DEBUGGER          %s\n", CLR_HEADER, CLR_RESET);
    printf("%s==========================================%s\n\n", CLR_HEADER, CLR_RESET);

    // Initialize mock state
    strcpy((char*)my_name, "DebugUser");
    my_sit = 1;
    table[1] = 1; // Mark seat 1 as active
    turn = 1;
    card_point = 50;
    info.wind = 0;   // East
    info.dealer = 1; // Me

    // Simple hand
    pool[1].num = 13;
    for(int i=0; i<13; i++) pool[1].card[i] = 11 + i;

    // Initialize AI
    ai_init();

    ai_phase_t phase = AI_PHASE_DISCARD;
    int test_card = 25; // 5 bamboo
    int from_seat = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "claim") == 0) {
            phase = AI_PHASE_CLAIM;
            from_seat = 2;
            printf("%sMode:%s CLAIM (from seat 2, card %d)\n", CLR_INFO, CLR_RESET, test_card);
        } else {
            test_card = atoi(argv[1]);
            printf("%sMode:%s DISCARD (drew card %d)\n", CLR_INFO, CLR_RESET, test_card);
        }
    } else {
        printf("%sMode:%s DISCARD (default, drew card %d)\n", CLR_INFO, CLR_RESET, test_card);
    }

    // Capture state before request for display
    char *state_json = ai_serialize_state(phase, test_card, from_seat);
    print_json("Serialized State", state_json);
    free(state_json);

    printf("\n%sEnabling AI and Registering Session...%s\n", CLR_INFO, CLR_RESET);
    ai_set_enabled(1);

    if (!ai_is_enabled()) {
        printf("%s[ERROR] Failed to enable AI session.%s\n", CLR_ERROR, CLR_RESET);
        return 1;
    }

    printf("\n%sSending Decision Request...%s\n", CLR_INFO, CLR_RESET);
    ai_decision_t decision = ai_get_decision(phase, test_card, from_seat);
    
    print_decision(decision);

    ai_cleanup();
    return 0;
}
