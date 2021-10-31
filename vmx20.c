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
static int currReg;

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
    if (fsize < 12)
    {
        *errorNumber = VMX20_FILE_IS_NOT_VALID;
        return 0;
    }

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

    if (fsize != ((opCodeSize * 4) + (inSymSize * 4) + 12))
    {
        *errorNumber = VMX20_FILE_IS_NOT_VALID;
        return 0;
    }

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
    int *error1 = malloc(sizeof(int));
    *error1 = 0;
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

    // if(runProgram(numProcessors, 0) == 0)
    // {
    //     //doerror
    // }

    free(registers);
    free(inst);
    free(preloadNames);
    free(preloadVals);
    return 1;
}

int runProgram(unsigned int numProcs, int currProc, int initialSP[], int *terminationStatus, int trace)
{
    load0intoReg(0);
    //todo:
    //runop
    //change so regs get changed with call
    //make stack aka push pc push fp and push 0
    //ldind to load the stack and the items at -1 which is stind values 0 is old fp
    stack *mem = newStack(10000);

    for (int i = 0; i < initialSP[0]; i++) //build stack start
    {
        stackpush(mem, 0);
    }
    currReg = 0;

    while (true)
    {
        char code[4] = inst[registers[currReg].reg[15]].code;
        int pc = registers[currReg].reg[15];

        if (tracing)
        {
            printTrace();
            char *buffer = malloc(100 * sizeof(char));
            int *errorNumber = malloc(sizeof(int));
            if (disassemble(pc, buffer, errorNumber) == 0)
            {
                *terminationStatus = *errorNumber;
                return 0;
            }
            else
            {
                printf("%s\n", buffer);
            }
        }

        if (code[0] == jmp)
        {
            int move = returnAddr(code, 5);
            registers[currReg].reg[15] += move;
            continue;
        }

        else if (code[0] == halt)
        {
            return 1;
        }

        else if (code[0] == call)
        {
            stackpush(mem, registers[currReg].reg[15]); //push pc
            registers[currReg].reg[13]++;
            stackpush(mem, registers[currReg].reg[14]); //push fp
            registers[currReg].reg[13]++;

            int temp = currReg + 1;
            load0intoReg(temp);
            registers[temp].reg[13] = registers[currReg].reg[13];
            registers[temp].reg[14] = registers[currReg].reg[13];
            int move = returnAddr(code, 5);
            registers[temp].reg[15] += move;
            currReg++;

            stackpush(mem, 0); //push 0
            registers[currReg].reg[13]++;

            continue;
        }

        else if (code[0] == ret)
        {
            int ret = mem->items[registers[currReg].reg[13]]; //pop return val
            if (stackpop(mem) == 0)
            {
                *terminationStatus = VMX20_ADDRESS_OUT_OF_RANGE;
                return 0;
            }
            registers[currReg].reg[13]--;

            int fp = mem->items[registers[currReg].reg[13]]; //pop fp
            if (stackpop(mem) == 0)
            {
                *terminationStatus = VMX20_ADDRESS_OUT_OF_RANGE;
                return 0;
            }
            registers[currReg].reg[13]--;

            int pc = mem->items[registers[currReg].reg[13]]; //pop pc
            if (stackpop(mem) == 0)
            {
                *terminationStatus = VMX20_ADDRESS_OUT_OF_RANGE;
                return 0;
            }
            registers[currReg].reg[13]--;
            int sp = registers[currReg].reg[13];
            currReg--;

            registers[currReg].reg[15] = pc;
            registers[currReg].reg[14] = fp;
            registers[currReg].reg[13] = sp;
            int m1offset = registers[currReg].reg[14] - 1;
            mem->items[m1offset] = ret;
        }

        else if (code[0] == ldind) //todo
        {
            int result = returnAddr(code, 4);
            if (result >= 0)
            {
                result--;
            }
            else
            {
                result++;
            }

            result += registers[currReg].reg[code[1] >> 4];

            int *outword = malloc(sizeof(int));
            if (getWord(result, outword) == 0)
            {
                // printf("%d    ", result);
                *error = VMX20_ADDRESS_OUT_OF_RANGE;
                return 0;
            };

            registers[currReg].reg[code[1] & 0xf] = *outword;

            return 1;
        }

        else if (code[0] == stind) //todo
        {
            int result = returnAddr(code, 4);
            if (result >= 0)
            {
                result--;
            }
            else
            {
                result++;
            }

            result += registers[currReg].reg[code[1] >> 4];

            if (putWord(result, registers[currReg].reg[code[1] & 0xf]) == 0)
            {
                *error = VMX20_ADDRESS_OUT_OF_RANGE;
                return 0;
            }
            return 1;
        }

        else if (code[0] == pop) //todo
        {
        }

        else if (code[0] == push) //todo
        {
        }

        else if (code[0] == getpid) //todo
        {
        }

        else if (code[0] == getpn) //todo
        {
        }

        else
        {
            if (runOpcode(code, pc, terminationStatus, trace) == 0)
            {
                return 0;
            }
        }

        registers[currReg].reg[15]++;
    }
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
        else
        {
            return;
        }
    }
}

int runOpcode(char code[4], int pc, int *error, bool tracing)
{

    if (code[0] == load)
    {
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

        registers[currReg].reg[code[1] & 0xf] = *outword;
        return 1;
    }

    else if (code[0] == store)
    {

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

        if (putWord(pc + result, registers[currReg].reg[code[1] & 0xf]) == 0)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }

        return 1;
    }

    else if (code[0] == addf)
    {
        float firstReg = *(float *)&registers[currReg].reg[code[1] & 0xf];
        float secReg = *(float *)&registers[currReg].reg[code[1] >> 4];
        float total = firstReg + secReg;
        registers[currReg].reg[code[1] & 0xf] = *(int *)&total;
    }

    else if (code[0] == addi)
    {
        int temp = registers[code[1] & 0xf].regNum;

        registers[currReg].reg[code[1] & 0xf] = registers[currReg].reg[code[1] >> 4] + temp;
    }

    else if (code[0] == subf)
    {
        float firstReg = *(float *)&registers[currReg].reg[code[1] & 0xf];
        float secReg = *(float *)&registers[currReg].reg[code[1] >> 4];
        float total = firstReg - secReg;
        registers[currReg].reg[code[1] & 0xf] = *(int *)&total;
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == subi)
    {
        registers[currReg].reg[code[1] & 0xf] -= registers[currReg].reg[code[1] >> 4];
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == ldaddr)
    {
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

        registers[currReg].reg[code[1] & 0xf] = pc + result;
        return 1;
    }

    else if (code[0] == ldimm)
    {
        int result = (code[3] << 12) | (code[2] << 4) | (code[1] >> 4);

        registers[currReg].reg[code[1] & 0xf] = result;

        return 1;
    }

    else if (code[0] == mulf)
    {

        float firstReg = *(float *)&registers[currReg].reg[code[1] & 0xf];
        float secReg = *(float *)&registers[currReg].reg[code[1] >> 4];
        float total = firstReg * secReg;
        registers[currReg].reg[code[1] & 0xf] = *(int *)&total;
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == muli)
    {
        registers[currReg].reg[code[1] & 0xf] = registers[currReg].reg[code[1] & 0xf] * registers[currReg].reg[code[1] >> 4];
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == divf)
    {
        float firstReg = *(float *)&registers[currReg].reg[code[1] & 0xf];
        float secReg = *(float *)&registers[currReg].reg[code[1] >> 4];
        if (secReg == 0)
        {
            *error = VMX20_DIVIDE_BY_ZERO;
            return 0;
        }

        float total = firstReg / secReg;
        registers[currReg].reg[code[1] & 0xf] = *(int *)&total;
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == divi)
    {
        if (registers[currReg].reg[code[1] >> 4] == 0)
        {
            *error = VMX20_DIVIDE_BY_ZERO;
            return 0;
        }
        registers[currReg].reg[code[1] & 0xf] = registers[currReg].reg[code[1] & 0xf] / registers[currReg].reg[code[1] >> 4];
        // registers[code[1] >> 4].regNum = 0;
    }

    else if (code[0] == ldind)
    {
        int result = returnAddr(code, 4);
        if (result >= 0)
        {
            result--;
        }
        else
        {
            result++;
        }

        result += registers[currReg].reg[code[1] >> 4];

        int *outword = malloc(sizeof(int));
        if (getWord(result, outword) == 0)
        {
            // printf("%d    ", result);
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        };

        registers[currReg].reg[code[1] & 0xf] = *outword;

        return 1;
    }

    else if (code[0] == stind) //todo
    {
        int result = returnAddr(code, 4);
        if (result >= 0)
        {
            result--;
        }
        else
        {
            result++;
        }

        result += registers[currReg].reg[code[1] >> 4];

        if (putWord(result, registers[currReg].reg[code[1] & 0xf]) == 0)
        {
            *error = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
    }

    else if (code[0] == cmpxchg) //todo
    {
    }

    else
    {
        *error = VMX20_ILLEGAL_INSTRUCTION;
        return 0;
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

void load0intoReg(int FP)
{
    registers = (reg *)malloc(1000 * (16 * sizeof(reg)));
    for (int i = 0; i < 16; i++)
    {
        registers[FP].reg[i] = 0;
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
    buffer[0] = *"\0";
    switch (inst[address].code[0])
    {
    case halt:
        strcat(buffer, "halt");
        return 1;
        break;
    case load:
        strcat(buffer, "load");
        if (opRegAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case store:
        strcat(buffer, "store");
        if (opRegAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case ldimm:
        strcat(buffer, "ldimm");
        opRegConst(buffer, inst[address].code);
        return 1;
        break;
    case ldaddr:
        strcat(buffer, "ldaddr");
        if (opRegAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case ldind:
        strcat(buffer, "ldind");
        opRegOffset(buffer, inst[address].code);
        return 1;
        break;
    case stind:
        strcat(buffer, "stind");
        opRegOffset(buffer, inst[address].code);
        return 1;
        break;
    case addf:
        strcat(buffer, "addf");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case subf:
        strcat(buffer, "subf");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case divf:
        strcat(buffer, "divf");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case mulf:
        strcat(buffer, "mulf");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case addi:
        strcat(buffer, "addi");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case subi:
        strcat(buffer, "subi");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case divi:
        strcat(buffer, "divi");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case muli:
        strcat(buffer, "muli");
        opRegReg(buffer, inst[address].code);
        return 1;
        break;
    case call:
        strcat(buffer, "call");
        if (opAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }

        return 1;
        break;
    case ret:
        strcat(buffer, "ret");
        return 1;
        break;
    case blt:
        strcat(buffer, "blt");
        if (opRegRegAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case bgt:
        strcat(buffer, "bgt");
        if (opRegRegAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case beq:
        strcat(buffer, "beq");
        if (opRegRegAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case jmp:
        strcat(buffer, "jmp");
        if (opAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case cmpxchg:
        strcat(buffer, "cmpxchg");
        if (opRegRegAddr(buffer, inst[address].code, address) == 0)
        {
            *errorNumber = VMX20_ADDRESS_OUT_OF_RANGE;
            return 0;
        }
        return 1;
        break;
    case getpid:
        strcat(buffer, "getpid");
        opReg(buffer, inst[address].code);
        return 1;
        break;
    case getpn:
        strcat(buffer, "getpn");
        opReg(buffer, inst[address].code);
        return 1;
        break;
    case push:
        strcat(buffer, "push");
        opReg(buffer, inst[address].code);
        return 1;
        break;
    case pop:
        strcat(buffer, "pop");
        opReg(buffer, inst[address].code);
        return 1;
        break;

    default:
        *errorNumber = VMX20_ILLEGAL_INSTRUCTION;
        return 0;
    }

    return 0;
}

void reg1(char *buffer, char code[4])
{
    strcat(buffer, " ");
    char holder[20];
    sprintf(holder, "%d", code[1] & 0xf);
    strcat(buffer, holder);
}

void reg2(char *buffer, char code[4])
{
    strcat(buffer, ", ");
    char holder[20];
    sprintf(holder, "%d", (code[1] >> 4) & 0xf);
    strcat(buffer, holder);
}

int opAddr(char *buffer, char code[4], int line)
{
    //Addr
    int linesmoved = returnAddr(code, 5);
    line = line + linesmoved;
    if (line < 0 || line > opCodeSize)
    {
        return 0;
    }

    strcat(buffer, " ");
    char holder[20];
    sprintf(holder, "%d", line);
    strcat(buffer, holder);
    return 1;
}

void opReg(char *buffer, char code[4])
{
    reg1(buffer, code);
}

void opRegConst(char *buffer, char code[4])
{
    reg1(buffer, code);
    int myconst = returnAddr(code, 5);
    strcat(buffer, " ");
    char holder[20];
    sprintf(holder, "%d", myconst);
    strcat(buffer, holder);
}

int opRegAddr(char *buffer, char code[4], int line)
{
    reg1(buffer, code);
    //Addr
    int linesmoved = returnAddr(code, 5);
    line = line + linesmoved;
    if (line < 0 || line > opCodeSize)
    {
        return 0;
    }
    strcat(buffer, " ");
    char holder[20];
    sprintf(holder, "%d", line);
    strcat(buffer, holder);
    return 1;
}

void opRegReg(char *buffer, char code[4])
{
    reg1(buffer, code);
    reg2(buffer, code);
}

void opRegOffset(char *buffer, char code[4])
{
    reg1(buffer, code);
    reg2(buffer, code);

    int myconst = returnAddr(code, 4);
    strcat(buffer, " ");
    char holder[20];
    sprintf(holder, "%d", myconst);
    strcat(buffer, holder);
}

int opRegRegAddr(char *buffer, char code[4], int line)
{
    reg1(buffer, code);
    reg2(buffer, code);
    //addr
    int linesmoved = returnAddr(code, 4);
    line = line + linesmoved;
    if (line < 0 || line > opCodeSize)
    {
        return 0;
    }
    strcat(buffer, " ");
    char holder[20];
    sprintf(holder, "%d", line);
    strcat(buffer, holder);
    return 1;
}

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

//need to change stack

struct stack *newStack(int capacity)
{
    struct stack *pt = (struct stack *)malloc(sizeof(stack));
    pt->maxsize = capacity;
    pt->top = -1;
    pt->items = (int *)malloc(sizeof(int) * capacity);
    return pt;
}

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
