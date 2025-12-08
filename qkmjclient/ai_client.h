#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <cjson/cJSON.h>

// AI Action Types
typedef enum {
    AI_ACTION_NONE,
    AI_ACTION_DISCARD,
    AI_ACTION_EAT,
    AI_ACTION_PONG,
    AI_ACTION_KANG,
    AI_ACTION_WIN,
    AI_ACTION_PASS
} ai_action_t;

// AI Decision Structure
typedef struct {
    ai_action_t action;
    int card;           // Card to discard or primary card of meld
    int meld_cards[2];  // For eat: the other two cards (e.g., if eating 3, cards might be [1, 2])
} ai_decision_t;

// Phase types
typedef enum {
    AI_PHASE_DISCARD,
    AI_PHASE_CLAIM
} ai_phase_t;

/**
 * @brief Initialize the AI client (CURL).
 */
void ai_init();

/**
 * @brief Cleanup the AI client.
 */
void ai_cleanup();

/**
 * @brief Check if AI mode is enabled.
 * @return 1 if enabled, 0 otherwise.
 */
int ai_is_enabled();

/**
 * @brief Enable or disable AI mode.
 * @param enabled 1 to enable, 0 to disable.
 */
void ai_set_enabled(int enabled);

/**
 * @brief Get a decision from the remote AI agent.
 * 
 * @param phase The current game phase (Discard or Claim).
 * @param card The relevant card (drawn card for discard phase, thrown card for claim phase).
 * @param from_seat Who threw the card (for claim phase).
 * @return ai_decision_t The decision structure.
 */
ai_decision_t ai_get_decision(ai_phase_t phase, int card, int from_seat);

/**
 * @brief Serialize the current game state to JSON string.
 * @param phase The current game phase.
 * @param card The relevant card.
 * @param from_seat The seat index of the player who discarded the card.
 * @return Serialized JSON string (must be freed).
 */
char* ai_serialize_state(ai_phase_t phase, int card, int from_seat);

/**
 * @brief Parse the AI decision from JSON response.
 * @param json_response The JSON string from AI agent.
 * @return The parsed decision.
 */
ai_decision_t ai_parse_decision(const char* json_response);

#endif // AI_CLIENT_H