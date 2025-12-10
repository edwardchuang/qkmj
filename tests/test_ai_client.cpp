#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ai_client.h"
#include "qkmj.h"
}

// Reset globals before each test
class AIClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        memset(&player, 0, sizeof(player));
        memset(&pool, 0, sizeof(pool));
        memset(&info, 0, sizeof(info));
        memset(table, 0, sizeof(table));
        memset(check_flag, 0, sizeof(check_flag));
        my_sit = 1;
        turn = 1;
        info.dealer = 1;
        info.wind = 0; // East
        
        // Setup table
        table[1] = 1; // Me
        table[2] = 2;
        table[3] = 3;
        table[4] = 4;
        
        // Setup dummy player names
        strcpy(player[1].name, "Hero");
        strcpy(player[2].name, "Villain1");
        strcpy(player[3].name, "Villain2");
        strcpy(player[4].name, "Villain3");
    }
};

TEST_F(AIClientTest, ParseDecisionDiscard) {
    const char* json = "{\"action\": \"discard\", \"card\": 23}";
    ai_decision_t decision = ai_parse_decision(json);
    
    EXPECT_EQ(decision.action, AI_ACTION_DISCARD);
    EXPECT_EQ(decision.card, 23);
}

TEST_F(AIClientTest, ParseDecisionEat) {
    const char* json = "{\"action\": \"eat\", \"card\": 23, \"meld_cards\": [24, 25]}";
    ai_decision_t decision = ai_parse_decision(json);
    
    EXPECT_EQ(decision.action, AI_ACTION_EAT);
    EXPECT_EQ(decision.card, 23);
    EXPECT_EQ(decision.meld_cards[0], 24);
    EXPECT_EQ(decision.meld_cards[1], 25);
}

TEST_F(AIClientTest, ParseDecisionWrappedArray) {
    const char* json = "[{\"content\": {\"parts\": [{\"text\": \"{\\\"action\\\": \\\"discard\\\", \\\"card\\\": 33}\"}]}}]";
    ai_decision_t decision = ai_parse_decision(json);
    
    EXPECT_EQ(decision.action, AI_ACTION_DISCARD);
    EXPECT_EQ(decision.card, 33);
}

TEST_F(AIClientTest, ParseDecisionPass) {
    const char* json = "{\"action\": \"pass\"}";
    ai_decision_t decision = ai_parse_decision(json);
    
    EXPECT_EQ(decision.action, AI_ACTION_PASS);
}

TEST_F(AIClientTest, SerializeStateDiscardPhase) {
    // Setup hand
    pool[1].num = 2;
    pool[1].card[0] = 11;
    pool[1].card[1] = 12;
    
    char* json_str = ai_serialize_state(AI_PHASE_DISCARD, 13, 0);
    ASSERT_NE(json_str, nullptr);
    
    // Simple string check (using cJSON to verify would be better but complex)
    std::string s(json_str);
    EXPECT_NE(s.find("\"cmd\":\"decision\""), std::string::npos);
    EXPECT_NE(s.find("\"phase\":\"discard\""), std::string::npos);
    EXPECT_NE(s.find("\"new_card\":13"), std::string::npos);
    
    // Check legal actions
    EXPECT_NE(s.find("\"can_discard\":true"), std::string::npos);
    
    free(json_str);
}

TEST_F(AIClientTest, SerializeStateClaimPhase) {
    // Setup legal action
    check_flag[1][1] = 1; // Can Eat
    
    char* json_str = ai_serialize_state(AI_PHASE_CLAIM, 23, 2);
    ASSERT_NE(json_str, nullptr);
    
    std::string s(json_str);
    EXPECT_NE(s.find("\"phase\":\"claim\""), std::string::npos);
    EXPECT_NE(s.find("\"new_card\":23"), std::string::npos);
    EXPECT_NE(s.find("\"from_seat\":2"), std::string::npos);
    EXPECT_NE(s.find("\"can_eat\":true"), std::string::npos);
    EXPECT_NE(s.find("\"can_pong\":false"), std::string::npos);
    
    free(json_str);
}
