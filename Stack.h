typedef struct stack
{
    int maxsize;
    int top;
    int *items;
}stack;

struct  stack* newStack(int capacity);
int size(stack *pt);
int isEmpty(stack *pt);
int isFull(stack *pt);
void stackpush(stack *pt, int x);
int stackpeek(stack *pt);
int stackpop(stack *pt);