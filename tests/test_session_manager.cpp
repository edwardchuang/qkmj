#include <gtest/gtest.h>
#include <thread>

// Pre-include C standard headers to avoid C++ linkage issues inside extern "C"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdbool> 
#include <algorithm> // Fix for bson-macros.h pulling in algorithm inside extern "C"

extern "C" {
#include "session_manager.h"
#include "mongo.h"
#include "logger.h"
}

// Test Fixture
class SessionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Connect to Mongo
        const char* env_uri = std::getenv("MONGO_URI");
        const char* uri = env_uri ? env_uri : "mongodb://localhost:27017";
        
        bool connected = mongo_connect(uri);
        if (!connected) {
             std::cerr << "Failed to connect to MongoDB at " << uri << std::endl;
        }
        ASSERT_TRUE(connected) << "Failed to connect to MongoDB. Ensure it is running.";
        
        // Clean up the collection before starting to ensure clean state
        mongo_drop_collection(MONGO_DB_NAME, "active_sessions");
        
        // Init session manager
        session_mgmt_init();
    }

    void TearDown() override {
        // Clean up after tests
        mongo_drop_collection(MONGO_DB_NAME, "active_sessions");
        mongo_disconnect();
    }
};

TEST_F(SessionManagerTest, CreateAndCheckOnline) {
    const char* user = "testuser_1";
    const char* server = "server_1";
    const char* ip = "127.0.0.1";
    long money = 1000;

    // Should not be online initially
    EXPECT_FALSE(session_is_online(user));

    // Create session
    bool created = session_create(user, server, ip, money);
    ASSERT_TRUE(created);

    // Should be online now
    EXPECT_TRUE(session_is_online(user));
}

TEST_F(SessionManagerTest, DestroySession) {
    const char* user = "testuser_2";
    session_create(user, "srv1", "1.1.1.1", 500);
    EXPECT_TRUE(session_is_online(user));

    bool destroyed = session_destroy(user);
    EXPECT_TRUE(destroyed);
    
    EXPECT_FALSE(session_is_online(user));
}

TEST_F(SessionManagerTest, UpdateStatus) {
    const char* user = "testuser_3";
    session_create(user, "srv1", "1.1.1.1", 500);

    // Update status to PLAYING in a table
    bool updated = session_update_status(user, "PLAYING", "table_leader_1");
    EXPECT_TRUE(updated);
    
    // User should still be online
    EXPECT_TRUE(session_is_online(user));
}

TEST_F(SessionManagerTest, Heartbeat) {
    const char* user = "testuser_4";
    session_create(user, "srv1", "1.1.1.1", 500);
    
    bool hb = session_heartbeat(user);
    EXPECT_TRUE(hb);
}

TEST_F(SessionManagerTest, DuplicateSessionUpsert) {
    // Test that creating a session for an existing user updates it (no error)
    const char* user = "testuser_5";
    session_create(user, "srv1", "1.1.1.1", 500);
    
    // Login again (different server or IP maybe)
    bool created_again = session_create(user, "srv2", "2.2.2.2", 600);
    EXPECT_TRUE(created_again);
    
    EXPECT_TRUE(session_is_online(user));
    // Ideally we would check if the fields updated, but session_is_online is a sufficient liveness check for now.
}
