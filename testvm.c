#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vmx20.h"
#include "Structs.h"
#include "extraFunc.h"

void printLoadError(int *error);

int main(int argc, char **argv)
{
    int *loadError = malloc(sizeof(int));

    int loadReturn = loadExecutableFile(argv[1], loadError); //only works for 1 file currently

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
                strncpy(name[preloadCount], mystr, j);
                name[preloadCount][j] = '\0';
                char *temp = malloc(sizeof(char*));
                strncpy(temp, mystr + j + 1, strlen(mystr)- j);
                val[preloadCount++] = atof(temp);

            }
        }
        if (hasVal == false) //add to print results
        {
            results[count++] = argv[i];
        }
    }


    if (loadReturn == 0)
    {
        printLoadError(loadError);
    }
    else
    {    
        preloadsym(name, val, preloadCount);
        printResults(results, count);
        int term[1];
        unsigned int sp[1];
        if(execute(1, sp, term, 1) == 1){
            // printf("vmx halts with status 0\n");
            exit(0);
        }
    }
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

//todo
//Implement:
//execute without trace
//exe supports jmp load store halt add and sub instr with mainx20 entry point