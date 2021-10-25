#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "vmx20.h"
#include "extraFunc.h"

void printLoadError(int *error);

void printExecuteTerm(int term[], int cpu);

int main(int argc, char **argv)
{
    int *loadError = malloc(sizeof(int));
    char **results = malloc(argc * sizeof(char *));
    int count = 0;

    char **name = malloc(argc * sizeof(char *));
    float *val = malloc(argc * sizeof(float));
    int preloadCount = 0;

    for (int i = 2; i < argc; i++)
    {
        bool hasVal = false;
        char *mystr = malloc(strlen(argv[i]));
        sprintf(mystr, "%s", argv[i]);

        for (int j = 0; j < strlen(argv[i]); j++)
        {
            if (mystr[j] == '=')
            {
                hasVal = true;

                char *temp1 = malloc(sizeof(j * sizeof(char)));
                strncpy(temp1, mystr, j * sizeof(char));
                name[preloadCount] = temp1;
                name[preloadCount][j] = '\0';

                char *temp = malloc((strlen(argv[i]) - j) * sizeof(char));
                strncpy(temp, mystr + j + 1, strlen(mystr) - j);
                val[preloadCount++] = atof(temp);
            }
        }

        if (hasVal == false) //add to print results
        {
            results[count++] = argv[i];
        }
    }

    preloadsym(name, val, preloadCount);

    int loadReturn = loadExecutableFile(argv[1], loadError); //only works for 1 file currently

    if (loadReturn == 0)
    {
        printLoadError(loadError);
    }
    else
    {
        printResults(results, count);
        int term[1];
        unsigned int sp[1];
        sp[0] = 0;
        if (execute(1, sp, term, 0) == 1)
        {
            printExecuteTerm(term, 1);
            printVal();
            exit(0);
        }
        
    }

    free(loadError);
    free(results);
    free(name);
    free(val);
}

void printLoadError(int *error)
{
    int errorint = *(int *)error;
    switch (errorint)
    {
    case VMX20_FILE_NOT_FOUND:
        printf("VMX20_FILE_NOT_FOUND\n");
        exit(0);
        break;
    case VMX20_FILE_CONTAINS_OUTSYMBOLS:
        printf("VMX20_FILE_CONTAINS_OUTSYMBOLS\n");
        exit(0);
        break;
    case VMX20_FILE_IS_NOT_VALID:
        printf("VMX20_FILE_IS_NOT_VALID\n");
        exit(0);
        break;
    default:
        printf("NULL_ERROR\n");
        exit(0);
        break;
    }
}

void printExecuteTerm(int term[], int cpu)
{

    for (int i = 0; i < cpu; i++)
    {
        switch (term[i])
        {
        case VMX20_NORMAL_TERMINATION:
            printf("Processor %d VMX20_NORMAL_TERMINATION\n", i);
            break;
        case VMX20_DIVIDE_BY_ZERO:
            printf("Processor %d VMX20_DIVIDE_BY_ZERO\n", i);
            break;
        case VMX20_ADDRESS_OUT_OF_RANGE:
            printf("Processor %d VMX20_ADDRESS_OUT_OF_RANGE\n", i);
            break;
        case VMX20_ILLEGAL_INSTRUCTION:
            printf("Processor %d VMX20_ILLEGAL_INSTRUCTION\n", i);
            break;

        default:
            printf("Processor %d VMX20_NULL_TERMINATION\n", i);
            break;
        }
    }
}

//todo
//Implement:
//execute without trace
//exe supports jmp load store halt add and sub instr with mainx20 entry point