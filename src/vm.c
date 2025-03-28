#include "vm.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "stdio.h"
#include "value.h"
// singleton VM instance
VM vm;

static void resetStack() { vm.stackTop = vm.stack; }

void initVM() {
    resetStack();
    vm.objects = NULL;
    initMap(&vm.strings);
    initMap(&vm.globals);
}

void freeVM() {
    freeMap(&vm.strings);
    freeMap(&vm.globals);
    freeObjects(vm.objects);
    vm.objects = NULL;
    initVM(&vm);
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    resetStack();
}

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }
static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());
    int length = a->length + b->length;
    char *str = ALLOCATE(char, length + 1);
    memcpy(str, a->chars, a->length);
    memcpy(str + a->length, b->chars, b->length);
    str[length] = '\0';
    ObjString *r = takeString(str, length);
    push(OBJ_VAL(r));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
    for (;;) {
#define BINARY_OP(valueType, op)     \
    do {                             \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b));     \
    } while (false)
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN:
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_ADD:
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    BINARY_OP(NUMBER_VAL, +);
                }
                break;
            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_CONSTANT:
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            case OP_POP:
                pop();
                break;
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                mapSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!mapGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                // returns true is the key is new
                if (mapSet(&vm.globals, name, peek(0))) {
                    mapDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_STRING
}

// InterpretResult interpret(Chunk *chunk) {
//     vm.chunk = chunk;
//     vm.ip = vm.chunk->code;
//     return run();
// }

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    printf("\nrunning...\n");
    InterpretResult result = run();
    freeChunk(&chunk);

    return INTERPRET_OK;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}
