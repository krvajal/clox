#include "object.h"

#include <string.h>

#include "memory.h"

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *obj = (Obj *)reallocate(NULL, 0, size);
    obj->type = type;
    return obj;
}

#define ALLOCATE_OBJ(type, objectType) \
    (type *)allocateObject(sizeof(type), objectType)

ObjString *allocateString(char *chars, int length) {
    ObjString *objString = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    objString->length = length;
    objString->chars = chars;
}

ObjString *copyString(const char *chars, int length) {
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}
