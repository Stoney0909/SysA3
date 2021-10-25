# Dissasembler
IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR= ./
LDIR =../lib

LIBS=-lm


_DEPS = vmx20.h Structs.h Opcodes.h extraFunc.h Stack.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = testvm.o  vmx20.o Stack.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all : testvm

testvm: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.INTERMEDIATE: $(_OBJ)

.PHONY: clean

# # Linker
# IDIR =../include
# CC=gcc
# CFLAGS=-I$(IDIR)

# ODIR= ./
# LDIR =../lib

# LIBS=-lm

# _DEPS = Opcodes.h
# DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))




# $(ODIR)/%.o: %.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS)



# .PHONY: clean