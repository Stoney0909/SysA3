#include <stdio.h>
#include <stdlib.h>
#include "Stack.h"
struct  stack* newStack(int capacity)
{
    struct stack *pt = (struct stack*)malloc(sizeof(stack));
    pt->maxsize = capacity;
    pt->top = -1;
    pt->items = (int*)malloc(sizeof(int)* capacity);
    return pt;
    
};

int size(stack *pt){
    return pt->top + 1;
}

int isEmpty(stack *pt){
    return pt->top == -1;
}

int isFull(stack *pt){
    return pt->top == pt->maxsize -1;
}

void stackpush(stack *pt, int x){
    if (isFull(pt))
    {
        printf("Stack Overflow\n Program Terminated\n");
        exit(0);
    }
    pt->items[++pt->top] = x;    
}

int stackpeek(stack *pt){
    if (!isEmpty(pt))
    {
        return pt->items[pt->top];
    }
    else{
        printf("No Stack Items Program Terminated\n");
        exit(0);
    }
    
}

int stackpop(stack *pt){
    if (isEmpty(pt))
    {
        printf("Underflow\nProgram Termninated\n");
        exit(0);
    }
    return pt->items[pt->top--];
    
}