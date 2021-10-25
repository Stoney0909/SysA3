#include "vmx20.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "Structs.h"
#include <stdint.h>
#include "Opcodes.h"
#include "extraFunc.h"
// #include "Stack.h"

static symbol *insyms = NULL;
static instruction *inst = NULL;
static Data *data = NULL;
static int inSymSize;
static int opCodeSize;
static int dataSize;
static reg *registers;
static char **preloadNames;
static float *preloadVals;
static int amountofPreload;
static char **resultList;
static int resultAmount;
static int donewithload = 0;

int loadExecutableFile(char *filename, int *errorNumber)
{
    char allfilename[30] = "./";
    FILE *in_file = NULL;
    strcat(allfilename, filename);
    in_file = fopen(filename, "rb");

    if (strlen(filename) < 4)
    {
        *errorNumber = 3;
        return 0;
    }
    else
    {
        char endofFile[5];
        endofFile[0] = filename[strlen(filename) - 4];
        endofFile[1] = filename[strlen(filename) - 3];
        endofFile[2] = filename[strlen(filename) - 2];
        endofFile[3] = filename[strlen(filename) - 1];
        endofFile[4] = '\0';
        if (!(strcmp(endofFile, ".exe") == 0))
        {

            *errorNumber = VMX20_FILE_IS_NOT_VALID;
            return 0;
        }
    }

    //check if file exists
    if (in_file == NULL)
    {
        *errorNumber = VMX20_FILE_NOT_FOUND;
        return 0;
    }

    //get file and put it in buffer
    fseek(in_file, 0, SEEK_END);
    long fsize = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    fread(buffer, 1, fsize, in_file);
    fclose(in_file);
    // if (fsize < 12){
    //     *errorNumber = VMX20_FILE_IS_NOT_VALID;
    //     return 0;
    // }

    //get insyms
    char insymbolSize[4];
    memcpy(insymbolSize, buffer, 4);

    char inSymLines[50];
    sprintf(inSymLines, "%u", *(int *)insymbolSize);
    int insymLinesInt = atoi(inSymLines);
    inSymSize = insymLinesInt;
    insyms = (symbol *)malloc(sizeof(symbol) * (insymLinesInt / 5));

    //check for outsyms
    char outsymbolSize[4];
    memcpy(outsymbolSize, buffer + sizeof(insymbolSize), 4);
    char outSymLines[50];
    sprintf(outSymLines, "%u", *(int *)outsymbolSize);
    int outsymLinesInt = atoi(outSymLines);

    if (outsymLinesInt > 0)
    {
        *errorNumber = VMX20_FILE_CONTAINS_OUTSYMBOLS;
        return 0;
    }

    //get opcodes
    char opcodesizechar[4];
    memcpy(opcodesizechar, buffer + 8, 4);

    char opcodeLines[50];
    sprintf(opcodeLines, "%u", *(int *)opcodesizechar);
    opCodeSize = atoi(opcodeLines);
    inst = (instruction *)malloc(sizeof(instruction) * opCodeSize);

    // printf("%d %d %d\n",inSymSize, outsymLinesInt, opCodeSize );
    // // printf("%ld  %d\n", sizeof(buffer),  ((opCodeSize * 4) + (inSymSize * 4) + 12));

    // if (sizeof(buffer) != ((opCodeSize * 4) + (inSymSize * 4) + 12))
    // {
    //    *errorNumber = VMX20_FILE_IS_NOT_VALID;
    //     return 0;
    // }

    //put insyms into struct
    for (int i = 0; i < insymLinesInt / 5; i++)
    {
        //name
        memcpy(insyms[i].name, buffer + 12 + (20 * i), 16);

        //line
        char lineHolder[4];
        memcpy(lineHolder, buffer + 12 + (20 * i) + 16, 4);

        char inSymLine[50];
        sprintf(inSymLine, "%u", *(int *)lineHolder);
        insyms[i].line = atoi(inSymLine);
    }

    inSymSize = insymLinesInt / 5;

    //put opcodes into struct

    for (int i = 0; i < opCodeSize; i++)
    {
        //code
        memcpy(inst[i].code, buffer + 12 + (20 * inSymSize) + (i * 4), 4);
        if (i < 20)
        {
            // for (int j = 0; j < 4; j++)
            // {
            //     printf("%02hhx", inst[i].code[j]);
            // }
            // printf("\n");
        }

        //line
        inst[i].line = i;
    }

    load0intoReg();
    //1st pass
    int *error1 = 0;
    run1stpass(0, error1);
    if (*error1 == 1)
    {
        *errorNumber = VMX20_FILE_IS_NOT_VALID;
        return 0;
    }
    
    data = (Data *)malloc(sizeof(Data) * opCodeSize);
    //load data
    for (int i = 0; i < opCodeSize; i++)
    {
        if (inst[i].checked != true)
        {
            // if (i < 20)
            // {
            //     for (int j = 0; j < 4; j++)
            //     {
            //         printf("%02hhx", inst[i].code[j]);
            //     }
            // }

            int result = (inst[i].code[3] << 24) | (inst[i].code[2] << 16) | (inst[i].code[1] << 8) | (inst[i].code[0]);
            
            // if (i < 20)
            // {
            //     printf(" %d   %d\n", i, result);
            // }
            
            
            putWord(i, result);
            continue;
        }
    }
    donewithload = 1;
    return 1;
}

// get the address of a symbol in the current executable file
//   the label must be a symbol in the insymbol section of the executable file
//   the address is returned through the second parameter
//   the function returns 1 if successful and 0 otherwise
int getAddress(char *label, unsigned int *outAddr)
{
    for (int i = 0; i < inSymSize; i++)
    {
        if (strcmp(label, insyms[i].name) == 0)
        {
            *outAddr = insyms[i].line;
            return 1;
        }
    }
    return 0;
}

// read a word from memory
//   the word is returned through the second parameter
//   the function returns 1 if successful and 0 otherwise
int getWord(unsigned int addr, int *outWord)
{
    for (int i = 0; i < dataSize; i++)
    {
        // printf("%d   %d\n", data[i].line, addr);
        if (data[i].line == addr)
        {

            *outWord = data[i].data;
            return 1;
        }
    }
    return 0;
}

// write a word to memory
//   the function returns 1 if successful and 0 otherwise
int putWord(unsigned int addr, int word)
{

    if (dataSize > 0)
    {
        // printf("%d   %d\n", addr, word);
        for (int i = 0; i < dataSize; i++)
        {
            if (data[i].line == addr)
            {
                data[i].data = word;
                return 1;
            }
        }
    }

    if (donewithload)
    {
        return 1;
    }

    // int *tempdata =  (int *)malloc(dataSize * sizeof(int));
    // memcpy(tempdata, data, dataSize* sizeof(int));
    // data = (int *)malloc((dataSize + 1) * sizeof(int));
    // memcpy(data, tempdata, dataSize* sizeof(int));
    // free(tempdata);
    data[dataSize].data = word;

    //  int *tempdata1 =  (int *)malloc(dataSize * sizeof(int));
    //     memcpy(tempdata1, dataline, dataSize* sizeof(int));
    //     dataline = (int *)malloc((dataSize + 1) * sizeof(int));
    //     memcpy(dataline, tempdata1, dataSize* sizeof(int));
    //     free(tempdata1);
    data[dataSize].line = addr;

    // printf("%d\n",dataSize);
    dataSize++;
    //  printf("New %d", dataSize);
    return 1;
}

// execute the current loaded executable file
//   the function returns 1 if all processors are successfully started and
//     0 otherwise
//   the first parameter specifies the number of processors to use
//   the second parameter provides an initial SP value for each processor
//   the third parameter is used to return the termination status for
//     each processor
//   the following termination statuses are supported:
//     VMX20_NORMAL_TERMINATION
//     VMX20_DIVIDE_BY_ZERO
//     VMX20_ADDRESS_OUT_OF_RANGE
//     VMX20_ILLEGAL_INSTRUCTION
//   the fourth parameter is a Boolean indicating whether an instruction
//     trace should be be printed to stderr
//   Note: that all other registers will be initialized to 0, including
//     the PC and the FP.
//

int execute(unsigned int numProcessors, unsigned int initialSP[],
            int terminationStatus[], int trace)
{
    if (loadSymsfrominterface() == 0)
    {
        printf("Failed to Load Insymbols from input\n");
        return 0;
    }

    //Initialize stack and put in values
    stack *sp = newStack(50);

    // for (int i = 0; i < sizeof(*initialSP) / 4; i++)
    // {
    //     printf("%d\n", initialSP[i]);
    //     stackpush(sp, initialSP[i]);
    // }
    stackpush(sp, 0);

    int *error;
    while (!isEmpty(sp))
    {
        int i = stackpeek(sp);
        if (inst[i].checked == true && inst[i].data == false)
        {
            if (trace)
            {
                printTrace();
            }
            
            if (runOpcode(inst[i].code, i, error, trace) == 0)
            {
                terminationStatus[0] = *error;
            }
            if (inst[i].code[0] == halt)
            {
                terminationStatus[0] = 0;
                return 1;
            }
        }

        stackpop(sp);
        i++;
        stackpush(sp, i);
    }

    free(registers);
    free(inst);
    free(preloadNames);
    free(preloadVals);
    return 1;
}

void printVal()
{
    for (int i = 0; i < resultAmount; i++)
    {

        int *outword = malloc(sizeof(int));
        for (int j = 0; j < inSymSize; j++)
        {

            if (strcmp(resultList[i], insyms[j].name) == 0)
            {
                printf("%s = ", resultList[i]);
                getWord(insyms[j].line, outword);
                printf("%.6g\n", *(float *)&*outword);
            }
        }
    }
    free(resultList);
}

void printResults(char **mystrlist, int amount)
{
    resultAmount = amount;
    resultList = malloc(sizeof(char *) * amount);
    memcpy(resultList, mystrlist, sizeof(mystrlist));
}

int loadSymsfrominterface()
{

    for (int i = 0; i < amountofPreload; i++)
    {
        for (int j = 0; j < inSymSize; j++)
        {
            if (strcmp(preloadNames[i], insyms[j].name) == 0)
            {
                putWord(insyms[j].line, *(int *)&preloadVals[i]);
            }
        }
    }
    return 1;
}

int preloadsym(char **names, float *val, int amount)
{
    amountofPreload = amount;
    preloadVals = malloc(sizeof(float) * amount);
    preloadNames = malloc(sizeof(char *) * amount);
    memcpy(preloadVals, val, sizeof(val));
    memcpy(preloadNames, names, sizeof(names));
    return 1;
}

void run1stpass(int pc, int *error)
{
    int i = pc;
    while (true)
    {
        if (inst[i].checked == false && inst[i].data != true)
        {
            // printf("Line %d ", i);
            // for (int j = 0; j < 4; j++)
            // {
            //     printf("%02hhx", inst[i].code[j]);
            // }
            // printf("\n");
            

            inst[i].data = false;
            inst[i].checked = true;

            if (inst[i].code[0] == jmp)
            {
                inst[i].data = false;
                int result = returnAddr(inst[i].code, 5);
                if (result > 0)
                {
                    // printf("%d\n", i + result + 1);
                    for (int j = pc + 1; j < i + result; j++)
                    {
                        inst[j].data = true;
                    }

                    run1stpass(i + result, error);
                    return;
                }
                else
                {
                    return;
                }
            }
            if (inst[i].code[0] == call)
            {
                int result = returnAddr(inst[i].code, 5);
                run1stpass(i + result, error);
            }
            if (inst[i].code[0] == ret)
            {
                return;
            }
            if (inst[i].code[0] == halt)
            {
                return;
            }
            if (inst[i].code[0] == beq)
            {
                int result = returnAddr(inst[i].code, 4);
                run1stpass(i + result, error);
            }
            if (inst[i].code[0] == bgt)
            {

                int result = returnAddr(inst[i].code, 4);
                run1stpass(i + result, error);
            }
            if (inst[i].code[0] == blt)
            {

                int result = returnAddr(inst[i].code, 4);
                run1stpass(i + result, error);
            }
            if (inst[i].code[0] > pop)
            {
               *error = 1;
            }
            
            i++;
        }
    }
}

int runOpcode(char code[4], int pc, int *error, bool tracing)
{
    if (tracing)
    {
        printf("%08d  ", pc);
        for (int j = 3; j > -1; j--)
        {
            printf("%02hhx", code[j]);
        }
    }

    if (code[0] == jmp)
    {
        if (tracing)
        {
            printf(" Jump\n");
        }

        return 1;
    }

    else if (code[0] == load)
    {
        if (tracing)
        {
            printf(" Load\n");
        }

        int result = returnAddr(code, 5);

        if (pc + result > opCodeSize)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        if (pc + result < 0)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        int *outword = malloc(sizeof(int));

        // printf("%d\n", result + pc);

        if (getWord(pc + result, outword) == 0)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }

        registers[code[1] & 0xf].regNum = *outword;
        return 1;
    }

    else if (code[0] == halt)
    {
        if (tracing)
        {
            printf("halt\n");
        }

        return 1;
    }

    else if (code[0] == store)
    {
        if (tracing)
        {
            printf(" Store\n");
        }
        int result = returnAddr(code, 5);
        // printf("%d\n", result + pc);

        if (pc + result > opCodeSize)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        if (pc + result < 0)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }

        if (putWord(pc + result, registers[code[1] & 0xf].regNum) == 0)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }

        return 1;
    }

    else if (code[0] == addf)
    {
        if (tracing)
        {
            printf(" addf\n");
        }

        float firstReg = *(float *)&registers[code[1] & 0xf].regNum;
        float secReg = *(float *)&registers[code[1] >> 4].regNum;
        float total = firstReg + secReg;
        registers[code[1] & 0xf].regNum = *(int *)&total;
    }

    else if (code[0] == addi)
    {
        if (tracing)
        {
            printf(" addi\n");
        }

        int temp = registers[code[1] & 0xf].regNum;

        registers[code[1] & 0xf].regNum = registers[code[1] >> 4].regNum + temp;

       
    }

    else if (code[0] == subf)
    {
        if (tracing)
        {
            printf(" subf\n");
        }

        float firstReg = *(float *)&registers[code[1] & 0xf].regNum;
        float secReg = *(float *)&registers[code[1] >> 4].regNum;
        float total = firstReg - secReg;
        registers[code[1] & 0xf].regNum = *(int *)&total;
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == subi)
    {
        if (tracing)
        {
            printf(" subi\n");
        }

        registers[code[1] & 0xf].regNum -= registers[code[1] >> 4].regNum;
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == ldaddr)
    {
        if (tracing)
        {
            printf(" ldaddr\n");
        }

        int result = returnAddr(code, 5);

        if (pc + result > opCodeSize)
        {
            *error = 2;
            return 0;
        }
        if (pc + result < 0)
        {
            *error = 2;
            return 0;
        }

        registers[code[1] & 0xf].regNum = pc + result;
        return 1;
    }

    else if (code[0] == ldimm)
    {
        if (tracing)
        {
            printf(" ldimm\n");
        }

        int result = (code[3] << 12) | (code[2] << 4) | (code[1] >> 4);

        registers[code[1] & 0xf].regNum = result;

        return 1;
    }

    else if (code[0] == ldind)
    {
        if (tracing)
        {
            printf(" ldind\n");
        }

        int result = returnAddr(code, 4);
        if (result >= 0)
        {
            result--;
        }
        else
        {
            result++;
        }

        // printf("%d \n", result);
        // for (int i = 3; i > 0; i++)
        // {
        //     printf("%02hhx", code[i]);
        // }
        
        result += registers[code[1] >> 4].regNum;

        int *outword = malloc(sizeof(int));
        if (getWord(result, outword) == 0)
        {
            // printf("%d    ", result);
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        };
        
        registers[code[1] & 0xf].regNum = *outword;

        return 1;
    }

    else
    {
        if (tracing)
        {
            printf("\n");
        }

        return 1;
    }
}

void printTrace()
{
    for (int i = 0; i < 16; i++)
    {
        printf("%d", registers[i].regNum);
        printf(" ");
        if (i == 7)
        {
            printf("\n");
        }
    }
    printf("\n");
}

void load0intoReg()
{
    registers = (reg *)malloc(16 * sizeof(reg));
    for (int i = 0; i < 16; i++)
    {
        registers[i].regNum = 0;
    }
}

// disassemble the word at the given address
//   return 1 if successful and 0 otherwise
//   the first parameter contains the address of the word to disassemble
//   the second parameter is a pointer to a buffer where the output should be
//     placed
//   the output will be the opcode followed by a space, followed by the
//     comma separated operands (if any). each comma will be followed by
//     a space. PC-relative addresses are converted to absolute addresses
//     and displayed in decimal. offsets and immediate constants are displayed
//     in decimal.
//   the caller can rely that the output will certainly not consume more than
//     100 characters
//   the third parameter is used to return an error code if an error is
//     encountered
//   the following error codes are supported:
//     VMX20_ADDRESS_OUT_OF_RANGE
//     VMX20_ILLEGAL_INSTRUCTION
int disassemble(unsigned int address, char *buffer, int *errorNumber)
{
    return 0;
}

//Notes
//Used to get addr
// int result = inst[i].code[3] | inst[i].code[2] |(inst[i].code[1] >> 4);

int returnAddr(char code[4], int amount)
{
    if (amount == 5)
    {
        unsigned int negChecker = (code[3] >> 4) & 0xf;
        if (negChecker == 15) //do twos comp
        {
            int linesmoved = 1048575 - ((((code[3] >> 4) & 0xf) * pow(16, 4)) + ((code[3] & 0xf) * pow(16, 3)) + (((code[2] >> 4) & 0xf) * pow(16, 2)) + ((code[2] & 0xf) * 16) + ((code[1] >> 4) & 0xf));
            return -linesmoved;
        }
        else
        {
            int linesmoved = (((code[3] >> 4) & 0xf) * pow(16, 4)) + ((code[3] & 0xf) * pow(16, 3)) + (((code[2] >> 4) & 0xf) * pow(16, 2)) + ((code[2] & 0xf) * 16) + ((code[1] >> 4) & 0xf);
            return linesmoved + 1;
        }
    }
    else
    {
        unsigned int negChecker = (code[3] >> 4) & 0xf;
        // printf("%d   ",  negChecker);
        if (negChecker == 15) //do twos comp
        {
            printf("%02hhx%02hhx\n", code[3], code[2]);
            int linesmoved = 65535 - ((((code[3] >> 4) & 0xf) * pow(16, 3)) + ((code[3] & 0xf) * pow(16, 2)) + (((code[2] >> 4) & 0xf) * 16) + (code[2] & 0xf));
            return -linesmoved;
        }
        else
        {
            int linesmoved = ((((code[3] >> 4) & 0xf) * pow(16, 3)) + ((code[3] & 0xf) * pow(16, 2)) + (((code[2] >> 4) & 0xf) * 16) + (code[2] & 0xf));
            return linesmoved + 1;
        }
    }
}
struct stack *newStack(int capacity)
{
    struct stack *pt = (struct stack *)malloc(sizeof(stack));
    pt->maxsize = capacity;
    pt->top = -1;
    pt->items = (int *)malloc(sizeof(int) * capacity);
    return pt;
};

int size(stack *pt)
{
    return pt->top + 1;
}

int isEmpty(stack *pt)
{
    return pt->top == -1;
}

int isFull(stack *pt)
{
    return pt->top == pt->maxsize - 1;
}

void stackpush(stack *pt, int x)
{
    if (isFull(pt))
    {
        printf("Stack Overflow\n Program Terminated\n");
        exit(0);
    }
    pt->items[++pt->top] = x;
}

int stackpeek(stack *pt)
{
    if (!isEmpty(pt))
    {
        return pt->items[pt->top];
    }
    else
    {
        printf("No Stack Items Program Terminated\n");
        exit(0);
    }
}

int stackpop(stack *pt)
{
    if (isEmpty(pt))
    {
        printf("Underflow\nProgram Termninated\n");
        exit(0);
    }
    return pt->items[pt->top--];
}