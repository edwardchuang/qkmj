/**
 * @file mongo.h
 * @brief Header for the MongoDB C wrapper functions.
 *
 * This header declares the functions provided by mongo.c for interacting
 * with a MongoDB database, including connection management and CRUD operations.
 */

#ifndef MONGO_H
#define MONGO_H

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#define MONGO_DB_NAME "qkmj"
#define MONGO_APP_NAME "qkmj"
#define MONGO_COLLECTION_USERS "users"
#define MONGO_COLLECTION_LOGS "logs"
#define MONGO_COLLECTION_COUNTERS "counters"
#define MONGO_SEQUENCE_USERID "userid"

/**
 * @brief Initializes the MongoDB driver and establishes a connection.
 *
 * @param uri_string The MongoDB connection string (e.g., "mongodb://localhost:27017").
 */
void mongo_connect(const char *uri_string);

/**
 * @brief Disconnects from the MongoDB server and cleans up resources.
 */
void mongo_disconnect(void);

/**
 * @brief Inserts a single document into a specified collection.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param document A BSON document to insert.
 * @return true if the insertion was successful, false otherwise.
 */
bool mongo_insert_document(const char *db_name, const char *collection_name,
                           const bson_t *document);

/**
 * @brief Finds a single document in a collection that matches the filter.
 *
 * The caller is responsible for destroying the returned bson_t document
 * with `bson_destroy()` when it is no longer needed.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @return A new BSON document, or NULL if no document was found or an error.
 */
bson_t *mongo_find_document(const char *db_name, const char *collection_name,
                            const bson_t *filter);

/**
 * @brief Finds all documents matching a filter and returns a cursor.
 *
 * The caller is responsible for destroying the returned cursor
 * with `mongoc_cursor_destroy()` when iteration is complete.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @return A new mongoc_cursor_t to iterate over results, or NULL on error.
 */
mongoc_cursor_t *mongo_find_documents(const char *db_name,
                                      const char *collection_name,
                                      const bson_t *filter);

/**
 * @brief Updates a single document that matches the filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @param update A BSON document specifying the update to apply.
 * @return true if the update was successful, false otherwise.
 */
bool mongo_update_document(const char *db_name, const char *collection_name,
                           const bson_t *filter, const bson_t *update);

/**
 * @brief Deletes a single document that matches the filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @return true if the deletion was successful, false otherwise.
 */
bool mongo_delete_document(const char *db_name, const char *collection_name,
                           const bson_t *filter);

/**
 * @brief Counts the number of documents in a collection matching a filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @return The number of matching documents, or -1 on error.
 */
int64_t mongo_count_documents(const char *db_name, const char *collection_name,
                              const bson_t *filter);

/**
 * @brief Drops (deletes) an entire collection from the database.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection to drop.
 */
void mongo_drop_collection(const char *db_name, const char *collection_name);

/**
 * @brief Atomically finds and increments a named sequence counter.
 *
 * @param db_name The name of the database where the 'counters' collection lives.
 * @param sequence_name The name of the sequence to increment.
 * @return The next value in the sequence, or -1 on error.
 */
int64_t mongo_get_next_sequence(const char *db_name,
                                const char *sequence_name);

#endif /* MONGO_H */