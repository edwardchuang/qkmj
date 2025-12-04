/**
 * @file mongo_test.c
 * @brief Unit tests for the MongoDB C wrapper (mongo.c).
 *
 * This file contains a series of unit tests to verify the functionality of
 * the MongoDB C wrapper functions defined in mongo.c. It covers connection,
 * disconnection, and all CRUD operations, including document insertion,
 * retrieval, updating, deletion, counting, and sequence generation.
 *
 * NOTE: These tests require a local MongoDB instance running at MONGODB_URI.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <bson/bson.h> // Added for bson_as_json and other BSON types
#include "mongo.h"

// Define constants for MongoDB URI, test database, and collection names.
#define MONGODB_URI "mongodb://localhost:27017"
#define TEST_DB_NAME "qkmj_test_db"
#define TEST_COLLECTION_NAME "test_collection"
#define TEST_SEQUENCE_NAME "user_id_sequence"

/**
 * @brief Helper function to print a BSON document to stdout.
 *
 * Converts a BSON document to its canonical extended JSON string representation
 * and prints it, then frees the allocated string.
 *
 * @param doc A pointer to the BSON document to print.
 */
void print_bson(const bson_t *doc) {
    char *str = bson_as_canonical_extended_json(doc, NULL);
    printf("%s\n", str);
    bson_free(str);
}

/**
 * @brief Tests the mongo_insert_document function.
 *
 * Inserts a sample document and asserts that the operation is successful.
 */
void test_insert() {
    printf("--- Testing mongo_insert_document ---\n");
    bson_t *doc = BCON_NEW("name", BCON_UTF8("test_user_1"),
                           "value", BCON_INT32(100));

    // Assert that the insertion operation returns true (success)
    assert(mongo_insert_document(TEST_DB_NAME, TEST_COLLECTION_NAME, doc) ==
           true);
    printf("Insert successful.\n");

    bson_destroy(doc);
}

/**
 * @brief Tests the mongo_count_documents function.
 *
 * Counts documents with an empty filter and with a specific filter,
 * asserting the correct counts.
 */
void test_count() {
    printf("--- Testing mongo_count_documents ---\n");
    bson_t *empty_filter = bson_new();

    // Count all documents
    int64_t count =
        mongo_count_documents(TEST_DB_NAME, TEST_COLLECTION_NAME, empty_filter);
    printf("Found %" PRId64 " documents.\n", count);
    assert(count == 1); // Should be 1 from the previous insert

    // Count documents matching a specific filter
    bson_t *filter = BCON_NEW("name", BCON_UTF8("test_user_1"));
    count =
        mongo_count_documents(TEST_DB_NAME, TEST_COLLECTION_NAME, filter);
    printf("Found %" PRId64 " documents with filter.\n", count);
    assert(count == 1); // Should still be 1

    bson_destroy(empty_filter);
    bson_destroy(filter);
}

/**
 * @brief Tests the mongo_find_document function.
 *
 * Finds an existing document, verifies its content, and attempts to find
 * a non-existent document, asserting the expected outcomes.
 */
void test_find_one() {
    printf("--- Testing mongo_find_document ---\n");
    bson_t *filter = BCON_NEW("name", BCON_UTF8("test_user_1"));
    bson_t *found_doc =
        mongo_find_document(TEST_DB_NAME, TEST_COLLECTION_NAME, filter);

    // Assert that a document was found
    assert(found_doc != NULL);
    printf("Found document:\n");
    print_bson(found_doc);

    // Verify the content of the found document
    bson_iter_t iter;
    assert(bson_iter_init_find(&iter, found_doc, "value"));
    assert(BSON_ITER_HOLDS_INT32(&iter));
    assert(bson_iter_int32(&iter) == 100);

    bson_destroy(filter);
    bson_destroy(found_doc);

    // Test case for a document that does not exist
    bson_t *bad_filter = BCON_NEW("name", BCON_UTF8("non_existent_user"));
    bson_t *not_found_doc =
        mongo_find_document(TEST_DB_NAME, TEST_COLLECTION_NAME, bad_filter);
    assert(not_found_doc == NULL); // Assert that no document was found
    printf("Correctly did not find non-existent document.\n");
    bson_destroy(bad_filter);
}

/**
 * @brief Tests the mongo_update_document function.
 *
 * Updates an existing document and then retrieves it to verify the update.
 */
void test_update() {
    printf("--- Testing mongo_update_document ---\n");
    bson_t *filter = BCON_NEW("name", BCON_UTF8("test_user_1"));
    bson_t *update = BCON_NEW("$set", "{", "value", BCON_INT32(200), "}");

    // Assert that the update operation returns true (success)
    assert(mongo_update_document(TEST_DB_NAME, TEST_COLLECTION_NAME, filter,
                                 update) == true);
    printf("Update successful.\n");

    // Retrieve the updated document to verify the change
    bson_t *updated_doc =
        mongo_find_document(TEST_DB_NAME, TEST_COLLECTION_NAME, filter);
    assert(updated_doc != NULL);

    bson_iter_t iter;
    assert(bson_iter_init_find(&iter, updated_doc, "value"));
    assert(BSON_ITER_HOLDS_INT32(&iter));
    int32_t new_value = bson_iter_int32(&iter);
    printf("Verified updated value: %d\n", new_value);
    assert(new_value == 200);

    bson_destroy(filter);
    bson_destroy(update);
    bson_destroy(updated_doc);
}

/**
 * @brief Tests the mongo_find_documents function using a cursor.
 *
 * Inserts an additional document, then retrieves all documents using a cursor
 * and asserts the correct number of documents found.
 */
void test_find_many() {
    printf("--- Testing mongo_find_documents ---\n");
    // Insert another document to have multiple documents to find
    bson_t *doc2 = BCON_NEW("name", BCON_UTF8("test_user_2"),
                           "value", BCON_INT32(300));
    assert(mongo_insert_document(TEST_DB_NAME, TEST_COLLECTION_NAME, doc2) ==
           true);
    bson_destroy(doc2);

    bson_t *filter = bson_new(); // Empty filter to find all documents
    mongoc_cursor_t *cursor =
        mongo_find_documents(TEST_DB_NAME, TEST_COLLECTION_NAME, filter);
    assert(cursor != NULL);

    const bson_t *doc;
    int count = 0;
    // Iterate through the cursor to count found documents
    while (mongoc_cursor_next(cursor, &doc)) {
        count++;
    }

    printf("Found %d documents with cursor.\n", count);
    assert(count == 2); // Should be 2 documents now

    mongoc_cursor_destroy(cursor);
    bson_destroy(filter);
}

/**
 * @brief Tests the mongo_delete_document function.
 *
 * Deletes a specific document and then verifies its removal by counting
 * remaining documents.
 */
void test_delete() {
    printf("--- Testing mongo_delete_document ---\n");
    bson_t *filter = BCON_NEW("name", BCON_UTF8("test_user_1"));

    // Assert that the deletion operation returns true (success)
    assert(mongo_delete_document(TEST_DB_NAME, TEST_COLLECTION_NAME, filter) ==
           true);
    printf("Delete successful.\n");

    // Verify deletion by counting with the same filter (should be 0)
    int64_t count =
        mongo_count_documents(TEST_DB_NAME, TEST_COLLECTION_NAME, filter);
    assert(count == 0);

    // Count all remaining documents (should be 1, for "test_user_2")
    bson_t *empty_filter = bson_new();
    count =
        mongo_count_documents(TEST_DB_NAME, TEST_COLLECTION_NAME, empty_filter);
    printf("Remaining documents: %" PRId64 "\n", count);
    assert(count == 1);

    bson_destroy(filter);
    bson_destroy(empty_filter);
}

/**
 * @brief Tests the mongo_get_next_sequence function.
 *
 * Calls the sequence function multiple times to assert its atomic increment
 * behavior and proper return values.
 */
void test_get_next_sequence() {
    printf("--- Testing mongo_get_next_sequence ---\n");

    int64_t seq1 =
        mongo_get_next_sequence(TEST_DB_NAME, TEST_SEQUENCE_NAME);
    printf("First sequence value: %" PRId64 "\n", seq1);
    assert(seq1 == 1); // First call, should initialize to 1

    int64_t seq2 =
        mongo_get_next_sequence(TEST_DB_NAME, TEST_SEQUENCE_NAME);
    printf("Second sequence value: %" PRId64 "\n", seq2);
    assert(seq2 == 2); // Should increment to 2

    int64_t seq3 =
        mongo_get_next_sequence(TEST_DB_NAME, TEST_SEQUENCE_NAME);
    printf("Third sequence value: %" PRId64 "\n", seq3);
    assert(seq3 == 3); // Should increment to 3

    printf("Sequence test successful.\n");
}


/**
 * @brief Main function to run all MongoDB tests.
 *
 * Connects to MongoDB, performs initial cleanup, runs all test functions,
 * performs final cleanup, and then disconnects.
 * Asserts success for each test step.
 *
 * @return EXIT_SUCCESS if all tests pass, EXIT_FAILURE otherwise.
 */
int main() {
    printf("### Starting MongoDB CRUD Test Suite ###\n");
    printf("NOTE: This test requires a local MongoDB instance running at %s\n\n",
           MONGODB_URI);

    // Connect to MongoDB
    mongo_connect(MONGODB_URI);

    // Initial cleanup: Drop the test collection and any existing sequence
    printf("--- Initial cleanup of test collection and sequence ---\n");
    mongo_drop_collection(TEST_DB_NAME, TEST_COLLECTION_NAME);
    mongo_drop_collection(TEST_DB_NAME, "counters"); // For sequence testing

    // Run tests in sequence
    test_insert();
    test_count();
    test_find_one();
    test_update();
    test_find_many();
    test_delete();
    test_get_next_sequence();

    // Final cleanup: Drop the test collection and sequence counter
    printf("\n--- Final cleanup of test collection and sequence ---\n");
    mongo_drop_collection(TEST_DB_NAME, TEST_COLLECTION_NAME);
    mongo_drop_collection(TEST_DB_NAME, "counters");

    // Disconnect from MongoDB
    mongo_disconnect();

    printf("\n### All tests passed successfully! ###\n");

    return EXIT_SUCCESS;
}