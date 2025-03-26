#include "vm.h"

#include "compiler.h"
#include "debug.h"
#include "stdio.h"
#include "value.h"
// singleton VM instance
VM vm;

static void resetStack() { vm.stackTop = vm.stack; }

void initVM() { resetStack(); }
void freeVM() {}

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
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
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_ADD:
                BINARY_OP(NUMBER_VAL, +);
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
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
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
