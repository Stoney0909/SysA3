int runProgram(unsigned int numProcs, int currProc, int initialSP[], int *terminationStatus, int trace);

void printVal();

void printResults(char **mystrlist, int amount);

int loadSymsfrominterface();

int preloadsym(char **names, float *val, int amount);

void run1stpass(int pc, int *error);

int runOpcode(char code[4], int pc, int *error, bool tracing);

void printTrace();

void load0intoReg();

int returnAddr(char code[4], int amount);



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



int opAddr(char *buffer, char code[4], int line);
void opReg(char *buffer, char code[4]);
void opRegConst(char *buffer, char code[4]);
int opRegAddr(char *buffer, char code[4], int line);
void opRegReg(char *buffer, char code[4]);
void opRegOffset(char *buffer, char code[4]);
int opRegRegAddr(char *buffer, char code[4], int line);