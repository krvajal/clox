#include "object.h"

#include <string.h>

#include "memory.h"
#include "vm.h"

extern VM vm;

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *obj = (Obj *)reallocate(NULL, 0, size);
    obj->type = type;

    // keep a linked list of objects
    obj->next = vm.objects;
    vm.objects = obj;

    return obj;
}

#define ALLOCATE_OBJ(type, objectType) \
    (type *)allocateObject(sizeof(type), objectType)

ObjString *allocateString(char *chars, int length, uint32_t hash) {
    ObjString *objString = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    objString->length = length;
    objString->chars = chars;
    objString->hash = hash;
    mapSet(&vm.strings, objString, NIL_VAL);
    return objString;
}

static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = mapFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(heapChars, length, hash);
}

ObjString *takeString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    return allocateString(chars, length, hash);
}