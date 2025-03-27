#include "memory.h"

#include <stdlib.h>

#include "object.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    void* result = realloc(pointer, newSize);
    if (result == NULL) {
        exit(1);
    }

    return result;
}

void freeObject(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            ObjString* objStr = (ObjString*)obj;
            FREE_ARRAY(char, objStr->chars, objStr->length + 1);
            FREE(OBJ_STRING, obj);
            break;
        }
    }
}

void freeObjects(Obj* root) {
    Obj* obj = root;
    while (obj != NULL) {
        Obj* next = obj->next;
        freeObject(obj);
        obj = next;
    }
}