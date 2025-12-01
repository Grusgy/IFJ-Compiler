//
// Created by Pavel Bobek on 01.12.2025.
//

#include <stdlib.h>
#include "generator.h"

int Stack_Init( Stack *stack ) {
    if(stack == NULL)
        return 1;

    stack->InstructionArray = malloc(sizeof(char) * MAX_STACK);
    if(stack->InstructionArray == NULL)
        return 2;

    stack->topIndex = -1;

    return 0;
}

bool Stack_IsEmpty( const Stack *stack ) {
    if(stack->topIndex == -1)
        return true;
    return false;
}

bool Stack_IsFull( const Stack *stack ) {
    if(stack->topIndex >= MAX_STACK - 1)
        return true;
    return false;
}

void Stack_Top( const Stack *stack, instruction_t** dataPtr ) {
    if(Stack_IsEmpty(stack))
    {
        dataPtr = NULL;
        return;
    }

    *dataPtr = stack->InstructionArray[stack->topIndex];
}

void Stack_Pop( Stack *stack ) {
    if(Stack_IsEmpty(stack))
        return;

    stack->topIndex--;
}

void Stack_Push( Stack *stack, instruction_t* data ) {

    if(Stack_IsFull(stack))
        return;

    stack->topIndex++;
    stack->InstructionArray[stack->topIndex] = data;
}

void Stack_Dispose( Stack *stack ) {
    stack->topIndex = -1;
    free(stack->InstructionArray);
    stack->InstructionArray = NULL;
}