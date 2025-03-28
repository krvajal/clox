#ifndef clox_map_h
#define clox_map_h

#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Map;

void initMap(Map* map);
void freeMap(Map* map);
bool mapSet(Map* map, ObjString* key, Value value);
bool mapGet(Map* map, ObjString* key, Value* outValue);
bool mapDelete(Map* map, ObjString* key);
ObjString* mapFindString(Map* map, const char* chars, int length,
                         uint32_t hash);
#endif