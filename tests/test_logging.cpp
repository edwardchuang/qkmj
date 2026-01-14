#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "qkmj.h"
#include "protocol.h"
#include "misc.h"
#include <cjson/cJSON.h>
}

// Global for capturing sent logs
static std::vector<std::pair<int, std::string>> captured_json;

// Mock send_json
extern "C" int send_json(int fd, int msg_id, cJSON* data) {
    char* s = cJSON_PrintUnformatted(data);
    captured_json.push_back({msg_id, std::string(s)});
    free(s);
    cJSON_Delete(data);
    return 1;
}

class LoggingTest : public ::testing::Test {
protected:
    void SetUp() override {
        captured_json.clear();
        memset(&player, 0, sizeof(player));
        memset(&pool, 0, sizeof(pool));
        memset(&info, 0, sizeof(info));
        memset(table, 0, sizeof(table));
        
        strncpy((char*)my_name, "TestUser", sizeof(my_name));
        my_sit = 1;
        table[1] = 1;
        gps_sockfd = 123;
        
        strncpy(current_match_id, "TEST-MATCH-123", sizeof(current_match_id));
        move_serial = 0;
    }
};

TEST_F(LoggingTest, SendGameLogBasic) {
    cJSON* extra = cJSON_CreateObject();
    cJSON_AddStringToObject(extra, "test", "extra_info");
    
    send_game_log("TestMove", 23, extra);
    cJSON_Delete(extra);
    
    ASSERT_EQ(captured_json.size(), 1);
    EXPECT_EQ(captured_json[0].first, 901);
    
    cJSON* log = cJSON_Parse(captured_json[0].second.c_str());
    ASSERT_NE(log, nullptr);
    
    EXPECT_STREQ(cJSON_GetObjectItem(log, "match_id")->valuestring, "TEST-MATCH-123");
    EXPECT_EQ(cJSON_GetObjectItem(log, "move_serial")->valueint, 1);
    EXPECT_STREQ(cJSON_GetObjectItem(log, "action")->valuestring, "TestMove");
    EXPECT_EQ(cJSON_GetObjectItem(log, "card")->valueint, 23);
    
    cJSON* state = cJSON_GetObjectItem(log, "state");
    ASSERT_NE(state, nullptr);
    EXPECT_STREQ(cJSON_GetObjectItem(state, "cmd")->valuestring, "decision");
    
    cJSON* ext = cJSON_GetObjectItem(log, "extra");
    ASSERT_NE(ext, nullptr);
    EXPECT_STREQ(cJSON_GetObjectItem(ext, "test")->valuestring, "extra_info");
    
    cJSON_Delete(log);
}

TEST_F(LoggingTest, MoveSerialIncrements) {
    send_game_log("Move1", 1, NULL);
    send_game_log("Move2", 2, NULL);
    
    ASSERT_EQ(captured_json.size(), 2);
    
    cJSON* log1 = cJSON_Parse(captured_json[0].second.c_str());
    cJSON* log2 = cJSON_Parse(captured_json[1].second.c_str());
    
    EXPECT_EQ(cJSON_GetObjectItem(log1, "move_serial")->valueint, 1);
    EXPECT_EQ(cJSON_GetObjectItem(log2, "move_serial")->valueint, 2);
    
    cJSON_Delete(log1);
    cJSON_Delete(log2);
}

TEST_F(LoggingTest, PassActionOptions) {
    // Setup check_flag to show options were available
    memset(check_flag, 0, sizeof(check_flag));
    check_flag[my_sit][1] = 1; // can_eat
    check_flag[my_sit][4] = 1; // can_win
    
    write_check(0); // Pass
    
    ASSERT_EQ(captured_json.size(), 1);
    cJSON* log = cJSON_Parse(captured_json[0].second.c_str());
    
    EXPECT_STREQ(cJSON_GetObjectItem(log, "action")->valuestring, "Pass");
    
    cJSON* extra = cJSON_GetObjectItem(log, "extra");
    ASSERT_NE(extra, nullptr);
    EXPECT_TRUE(cJSON_IsTrue(cJSON_GetObjectItem(extra, "can_eat")));
    EXPECT_TRUE(cJSON_IsTrue(cJSON_GetObjectItem(extra, "can_win")));
    EXPECT_FALSE(cJSON_HasObjectItem(extra, "can_pong"));
    
    cJSON_Delete(log);
}

TEST_F(LoggingTest, MatchIdFormat) {

    // Simulating new Hex-based match_id generation with byte-swap

    char match_id[64];

    unsigned int ts = (unsigned int)time(NULL);

    unsigned int rev_ts = ((ts & 0x000000FF) << 24) |

                          ((ts & 0x0000FF00) << 8)  |

                          ((ts & 0x00FF0000) >> 8)  |

                          ((ts & 0xFF000000) >> 24);

    

    int player_id = 5;

    unsigned int salt = 0xABCD;

    snprintf(match_id, sizeof(match_id), "%08X%04X", rev_ts, (unsigned int)((player_id ^ salt) & 0xFFFF));

    

    EXPECT_EQ(strlen(match_id), 12);

    // Should be all hex (0-9, A-F)

    for(int i=0; i<12; i++) {

        EXPECT_TRUE(isxdigit(match_id[i]));

    }

}


