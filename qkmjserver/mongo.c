/**
 * @file mongo.c
 * @brief A simplified C wrapper for common MongoDB operations.
 *
 * This file provides a basic API to connect to, disconnect from, and perform
 * standard CRUD (Create, Read, Update, Delete) operations on a MongoDB
 * database. It uses the mongoc (MongoDB C Driver) library.
 */

#include "mongo.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// Global MongoDB client and URI instances
static mongoc_client_t* client;
static mongoc_uri_t* uri;

/**
 * @brief Helper to detect system CA certificate bundle location.
 * 
 * Checks common locations on macOS (Homebrew) and Linux.
 * 
 * @return Path to the CA file if found, otherwise NULL.
 */
static const char* detect_system_ca_file() {
    static const char* search_paths[] = {
        "/opt/homebrew/etc/openssl@3/cert.pem",   // macOS Homebrew (Apple Silicon)
        "/usr/local/etc/openssl@1.1/cert.pem",    // macOS Homebrew (Intel)
        "/etc/ssl/cert.pem",                      // macOS System / Some Linux
        "/etc/pki/tls/certs/ca-bundle.crt",       // Fedora/RHEL/CentOS
        "/etc/ssl/certs/ca-certificates.crt",     // Debian/Ubuntu/Gentoo
        NULL
    };

    for (int i = 0; search_paths[i] != NULL; i++) {
        if (access(search_paths[i], F_OK) == 0) {
            return search_paths[i];
        }
    }
    return NULL;
}

/**
 * @brief Initializes the MongoDB driver and establishes a connection.
 *
 * This function sets up the MongoDB C driver, parses the connection URI string,
 * and creates a new client instance. It should be called once before any other
 * database operations.
 *
 * @param uri_string The MongoDB connection string (e.g.,
 * "mongodb://localhost:27017").
 */
bool mongo_connect(const char* uri_string) {
  mongoc_init();

  uri = mongoc_uri_new_with_error(uri_string, NULL);
  if (!uri) {
    fprintf(stderr, "failed to parse URI: %s\n", uri_string);
    return false;
  }

  // Auto-detect CA file if not specified in URI.
  // Priority:
  // 1. SSL_CERT_FILE environment variable (if set and file exists)
  // 2. System default paths (detect_system_ca_file)
  // This fixes connection issues when running outside of debuggers/IDEs that don't inject cert paths.
  if (!mongoc_uri_get_option_as_utf8(uri, "tlsCAFile", NULL)) {
      const char* env_ca_file = getenv("SSL_CERT_FILE");
      const char* ca_file = NULL;

      if (env_ca_file && access(env_ca_file, F_OK) == 0) {
          ca_file = env_ca_file;
      } else {
          ca_file = detect_system_ca_file();
      }

      if (ca_file) {
          mongoc_uri_set_option_as_utf8(uri, "tlsCAFile", ca_file);
          // Optional: Verify it was set (for debugging, though we lack a logger here)
          // fprintf(stderr, "mongo_connect: Auto-detected tlsCAFile=%s\n", ca_file);
      }
  }

  client = mongoc_client_new_from_uri(uri);
  if (!client) {
    fprintf(stderr, "failed to create client\n");
    mongoc_uri_destroy(uri);
    mongoc_cleanup();
    return false;
  }

  mongoc_client_set_appname(client, MONGO_APP_NAME);

  // Check connection
  bson_t* command;
  bson_t reply;
  bson_error_t error;
  command = BCON_NEW("ping", BCON_INT32(1));
  if (!mongoc_client_command_simple(client, "admin", command, NULL, &reply,
                                    &error)) {
    fprintf(stderr, "Ping failed: %s\n", error.message);
    bson_destroy(command);
    bson_destroy(&reply);
    mongoc_client_destroy(client);
    mongoc_uri_destroy(uri);
    mongoc_cleanup();
    return false;
  }
  bson_destroy(command);
  bson_destroy(&reply);
  return true;
}

/**
 * @brief Disconnects from the MongoDB server and cleans up resources.
 *
 * This function destroys the client and URI objects and cleans up all resources
 * allocated by the MongoDB C driver. It should be called when the application
 * is finished with the database.
 */
void mongo_disconnect(void) {
  if (client) {
    mongoc_client_destroy(client);
  }
  if (uri) {
    mongoc_uri_destroy(uri);
  }
  mongoc_cleanup();
}

/**
 * @brief Inserts a single document into a specified collection.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param document A BSON document to insert.
 * @return true if the insertion was successful, false otherwise.
 */
bool mongo_insert_document(const char* db_name, const char* collection_name,
                           const bson_t* document) {
  mongoc_collection_t* collection;
  bson_error_t error;
  bool retval;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return false;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  retval =
      mongoc_collection_insert_one(collection, document, NULL, NULL, &error);

  if (!retval) {
    fprintf(stderr, "Insert failed: %s\n", error.message);
  }

  mongoc_collection_destroy(collection);
  return retval;
}

/**
 * @brief Finds a single document in a collection that matches the filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @return A new BSON document that must be freed with bson_destroy(), or NULL
 *         if no document was found or an error occurred.
 */
bson_t* mongo_find_document(const char* db_name, const char* collection_name,
                            const bson_t* filter) {
  mongoc_collection_t* collection;
  mongoc_cursor_t* cursor;
  const bson_t* found_doc;
  bson_t* doc = NULL;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return NULL;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  cursor = mongoc_collection_find_with_opts(collection, filter, NULL, NULL);

  // Fetch the first result
  if (mongoc_cursor_next(cursor, &found_doc)) {
    doc = bson_copy(found_doc);
  }

  mongoc_cursor_destroy(cursor);
  mongoc_collection_destroy(collection);
  return doc;
}

/**
 * @brief Finds all documents matching a filter and returns a cursor.
 *
 * The caller is responsible for iterating through the cursor and destroying it
 * with mongoc_cursor_destroy() when finished.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @return A new mongoc_cursor_t to iterate over the results, or NULL on error.
 */
mongoc_cursor_t* mongo_find_documents(const char* db_name,
                                      const char* collection_name,
                                      const bson_t* filter) {
  mongoc_collection_t* collection;
  mongoc_cursor_t* cursor;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return NULL;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  cursor = mongoc_collection_find_with_opts(collection, filter, NULL, NULL);

  // The collection object is destroyed here. This is generally safe as long
  // as the cursor does not need to access the collection after its creation.
  // For more complex scenarios, the collection's lifecycle might need to be
  // managed by the caller alongside the cursor.
  mongoc_collection_destroy(collection);

  return cursor;
}

/**
 * @brief Updates a single document that matches the filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @param update A BSON document specifying the update to apply.
 * @return true if the update was successful, false otherwise.
 */
bool mongo_update_document(const char* db_name, const char* collection_name,
                           const bson_t* filter, const bson_t* update) {
  mongoc_collection_t* collection;
  bson_error_t error;
  bool retval;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return false;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  retval = mongoc_collection_update_one(collection, filter, update, NULL, NULL,
                                        &error);

  if (!retval) {
    fprintf(stderr, "Update failed: %s\n", error.message);
  }

  mongoc_collection_destroy(collection);
  return retval;
}

/**
 * @brief Replaces a single document that matches the filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @param replacement A BSON document specifying the replacement document.
 * @return true if the replacement was successful, false otherwise.
 */
bool mongo_replace_document(const char* db_name, const char* collection_name,
                            const bson_t* filter, const bson_t* replacement) {
  mongoc_collection_t* collection;
  bson_error_t error;
  bool retval;
  bson_t* opts;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return false;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  opts = BCON_NEW("upsert", BCON_BOOL(true));
  retval = mongoc_collection_replace_one(collection, filter, replacement, opts,
                                         NULL, &error);
  bson_destroy(opts);

  if (!retval) {
    fprintf(stderr, "Replace failed: %s\n", error.message);
  }

  mongoc_collection_destroy(collection);
  return retval;
}

/**
 * @brief Deletes a single document that matches the filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter.
 * @return true if the deletion was successful, false otherwise.
 */
bool mongo_delete_document(const char* db_name, const char* collection_name,
                           const bson_t* filter) {
  mongoc_collection_t* collection;
  bson_error_t error;
  bool retval;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return false;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  retval = mongoc_collection_delete_one(collection, filter, NULL, NULL, &error);

  if (!retval) {
    fprintf(stderr, "Delete failed: %s\n", error.message);
  }

  mongoc_collection_destroy(collection);
  return retval;
}

/**
 * @brief Counts the number of documents in a collection matching a filter.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection.
 * @param filter A BSON document specifying the query filter. Use an empty
 *               BSON document to count all documents.
 * @return The number of matching documents, or -1 on error.
 */
int64_t mongo_count_documents(const char* db_name, const char* collection_name,
                              const bson_t* filter) {
  mongoc_collection_t* collection;
  bson_error_t error;
  int64_t count;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return -1;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  count = mongoc_collection_count_documents(collection, filter, NULL, NULL,
                                            NULL, &error);

  if (count < 0) {
    fprintf(stderr, "Count failed: %s\n", error.message);
  }

  mongoc_collection_destroy(collection);
  return count;
}

/**
 * @brief Drops (deletes) an entire collection from the database.
 *
 * @param db_name The name of the database.
 * @param collection_name The name of the collection to drop.
 */
void mongo_drop_collection(const char* db_name, const char* collection_name) {
  mongoc_collection_t* collection;
  bson_error_t error;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return;
  }

  collection = mongoc_client_get_collection(client, db_name, collection_name);
  if (!mongoc_collection_drop(collection, &error)) {
    // It's not a fatal error if the collection didn't exist. The driver
    // reports "ns not found" in this case, which we can safely ignore.
    if (strcmp(error.message, "ns not found") != 0) {
      fprintf(stderr, "Failed to drop collection: %s\n", error.message);
    }
  }

  mongoc_collection_destroy(collection);
}

/**
 * @brief Atomically finds and increments a named sequence counter.
 *
 * This function implements a counter pattern, often used for creating
 * auto-incrementing IDs. It uses a 'counters' collection to store sequence
 * values. The operation is atomic.
 *
 * @param db_name The name of the database where the 'counters' collection
 * lives.
 * @param sequence_name The name of the sequence to increment (e.g., "userid").
 * @return The next value in the sequence, or -1 on error.
 */
int64_t mongo_get_next_sequence(const char* db_name,
                                const char* sequence_name) {
  mongoc_collection_t* collection;
  bson_error_t error;
  bson_t* query;
  bson_t* update;
  bson_t reply;
  int64_t seq_value = -1;
  mongoc_find_and_modify_opts_t* opts;
  bson_iter_t iter;
  bson_iter_t child;

  if (!client) {
    fprintf(stderr, "MongoDB client not connected\n");
    return -1;
  }

  collection =
      mongoc_client_get_collection(client, db_name, MONGO_COLLECTION_COUNTERS);

  query = BCON_NEW("_id", BCON_UTF8(sequence_name));
  update = BCON_NEW("$inc", "{", "seq", BCON_INT64(1), "}");

  // Set options for the findAndModify command.
  // - Upsert: If the sequence name doesn't exist, create it.
  // - Return New: Return the document *after* it has been modified.
  opts = mongoc_find_and_modify_opts_new();
  mongoc_find_and_modify_opts_set_update(opts, update);
  mongoc_find_and_modify_opts_set_flags(
      opts, MONGOC_FIND_AND_MODIFY_UPSERT | MONGOC_FIND_AND_MODIFY_RETURN_NEW);

  if (mongoc_collection_find_and_modify_with_opts(collection, query, opts,
                                                  &reply, &error)) {
    // Extract the 'seq' value from the returned document
    if (bson_iter_init_find(&iter, &reply, "value") &&
        BSON_ITER_HOLDS_DOCUMENT(&iter) && bson_iter_recurse(&iter, &child)) {
      if (bson_iter_find(&child, "seq") && BSON_ITER_HOLDS_INT64(&child)) {
        seq_value = bson_iter_int64(&child);
      }
    }
  } else {
    fprintf(stderr, "get_next_sequence failed: %s\n", error.message);
  }

  bson_destroy(query);
  bson_destroy(update);
  bson_destroy(&reply);
  mongoc_find_and_modify_opts_destroy(opts);
  mongoc_collection_destroy(collection);

  return seq_value;
}
