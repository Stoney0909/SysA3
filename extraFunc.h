void printVal();

void printResults(char **mystrlist, int amount);

int loadSymsfrominterface();

int preloadsym(char **names, float *val, int amount);

void run1stpass(int pc);

int runOpcode(char code[4], int pc, int *error, bool tracing);

void printTrace();

void load0intoReg();

int returnAddr(char code[4], int amount);