#include "map.h"

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75

void initMap(Map* map) {
    map->count = 0;
    map->capacity = 0;
    map->entries = NULL;
}

void freeMap(Map* map) {
    FREE_ARRAY(Entry, map->entries, map->capacity);
    initMap(map);
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;
    // linear probing
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == key) {
            return entry;
        }
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // return the tombstone to place the value in the
                // prev tombstone slot for reuse
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        }
        index = (index + 1) % capacity;
    }
}

static void adjustCapacity(Map* map, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }
    map->count = 0;
    for (int i = 0; i < map->capacity; i++) {
        Entry* entry = &(map->entries[i]);
        if (entry->key == NULL) {
            // here we discard empty keys and tombstones
            continue;
        }
        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        map->count++;
    }
    FREE_ARRAY(Entry, map->entries, map->capacity);
    map->entries = entries;
    map->capacity = capacity;
}

bool mapSet(Map* map, ObjString* key, Value value) {
    if (map->count + 1 > map->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(map->capacity);
        adjustCapacity(map, capacity);
    }

    Entry* entry = findEntry(map->entries, map->capacity, key);
    bool isNewKey = entry->key == NULL;
    // only increment the count if we placed
    // the entry in a new slot
    if (isNewKey && IS_NIL(entry->value)) {
        map->count++;
    }
    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void mapAddAll(Map* from, Map* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries;
        if (entry->key == NULL) {
            continue;
        }
        mapSet(to, entry->key, entry->value);
    }
}

bool mapGet(Map* map, ObjString* key, Value* outValue) {
    if (map->count == 0) {
        return false;
    }
    Entry* entry = findEntry(map->entries, map->capacity, key);
    if (entry->key == NULL) {
        return false;
    }
    *outValue = entry->value;
    return true;
}

bool mapDelete(Map* map, ObjString* key) {
    if (map->count == 0) {
        return false;
    }
    Entry* entry = findEntry(map->entries, map->capacity, key);
    if (entry->key == NULL) {
        return false;
    }
    // place a tombstone in the entry
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    // note that count is not decreased here
    // as tombstones are part of the size of the map
    return true;
}

ObjString* mapFindString(Map* map, const char* chars, int length,
                         uint32_t hash) {
    if (map->count == 0) {
        return NULL;
    }
    uint32_t index = hash % map->capacity;
    for (;;) {
        Entry* entry = &map->entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // empty slopt without tombstone
                return NULL;
            }
        } else if (entry->key->length == length && entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }
        index = (index + 1) % map->capacity;
    }
}