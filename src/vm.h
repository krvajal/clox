#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "map.h"

#define STACK_MAX 256

typedef struct {
    Chunk *chunk;
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stackTop;
    Obj *objects;
    Map globals;
    Map strings;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM();
void freeVM();
void push(Value value);
Value pop();

// InterpretResult interpret(Chunk *chunk);
InterpretResult interpret(const char *line);

#endif