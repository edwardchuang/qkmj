#ifndef MJGPS_MONGO_HELPERS_H
#define MJGPS_MONGO_HELPERS_H

#include <bson/bson.h>

#include "mjgps.h"

// Helper to convert MongoDB BSON to player_record struct
void bson_to_record(const bson_t* doc, struct player_record* rec);

// Helper to convert player_record struct to MongoDB BSON
bson_t* record_to_bson(const struct player_record* rec);

#endif  // MJGPS_MONGO_HELPERS_H