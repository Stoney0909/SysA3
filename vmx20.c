#include "vmx20.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "Structs.h"
#include <stdint.h>

symbol *insyms = NULL;
instruction *inst = NULL;
Data *data = NULL;
int inSymSize = 0;
int opCodeSize = 0;
int dataSize = 0;

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

    //get insyms
    char insymbolSize[4];
    memcpy(insymbolSize, buffer, 4);

    char inSymLines[50];
    sprintf(inSymLines, "%u", *(int *)insymbolSize);
    int insymLinesInt = atoi(inSymLines);
    inSymSize = insymLinesInt;
    insyms = (symbol *)malloc(sizeof(symbol) * (insymLinesInt / 5));

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
    int opCodeSize = atoi(opcodeLines);
    printf("%d\n", opCodeSize);
    inst = (instruction*)malloc(sizeof(inst) * opCodeSize);

    //put opcodes into struct

    for (int i = 0; i < opCodeSize; i++)
    {
        //code
        memcpy(inst[i].code, buffer + 12 + (20 * inSymSize) + (i * 4) , 4);
        
        //line
        inst[i].line = i;
    }

    data = malloc(opCodeSize * sizeof(Data));
    
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
        if (data[i].line == addr)
        {
            *outWord = data[i].dataI;
            return 1;
        }        
    }
    return 0;
    
}

// write a word to memory
//   the function returns 1 if successful and 0 otherwise
int putWord(unsigned int addr, int word)
{

        data[dataSize].dataI = word;
        data[dataSize].line = addr;
        dataSize++;
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
    return 0;
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