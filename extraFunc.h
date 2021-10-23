void printVal();

void printResults(char **mystrlist, int amount);

int loadSymsfrominterface();

int preloadsym(char **names, float *val, int amount);

void run1stpass(int pc);

int runOpcode(char code[4], int pc, int *error);

void printTrace();

void load0intoReg();