#include <stdbool.h>
typedef struct instruction
{
    int line;
    char code[4];
    bool data;
    bool checked;
}instruction;

typedef struct symbol
{
    int line;
    char name[16];
    float data;
}symbol;
typedef struct Data
{
    int line;
    int data;
}Data;
typedef struct reg
{
    int regNum;
}reg;


