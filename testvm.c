#include <stdio.h>
#include <stdlib.h>
#include "vmx20.h"

void printLoadError(int *error);

int main(int argc, char **argv)
{
    int *loadError = malloc(sizeof(int));

    int loadReturn = loadExecutableFile(argv[1], loadError);//only works for 1 file currently

    if (loadReturn == 0)
    {
        printLoadError(loadError);
    }
    else{
        printf("Success\n");
    }




}

void printLoadError(int *error)
{
    int errorint = *(int*) error;
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
//loadexe
//getaddr
//getword
//putword
//execute without trace
//exe supports jmp load store halt add and sub instr with mainx20 entry point