IDIR=inc
CC=g++
CFLAGS=-lpthread -I$(IDIR) 
DFLAGS=-g -Wall -lpthread -I$(IDIR) -DDEBUG
BDIR=bin
ODIR=$(BDIR)/obj

SDIR=src
RDIR=$(BDIR)/release
DDIR=$(BDIR)/debug

SCRDIR=scripts

TARGET=main

_DEPS = $(shell ls $(IDIR))
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
SRC := $(shell ls $(SDIR))
_OBJ = $(patsubst %.cpp, %.o, $(SRC))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
OBJDEBUG = $(patsubst %,$(ODIR)/debug_%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $(RDIR)/$@ $^ $(CFLAGS)


$(ODIR)/debug_%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(DFLAGS)
# debug compila o programa com flag de debugging para o g++
# e também retira a necessidade de colocar o número de iterações
# na chamada do comando. É útil para testes em valgrind e etc
debug: $(OBJDEBUG)
	$(CC) -o $(DDIR)/$(TARGET) $^ $(DFLAGS)

.PHONY: test
test: debug
	sh $(SCRDIR)/test.sh

# run depende da variável de ambiente N_ITERATIONS
# para rodar o comando, coloque o comando como N_ITERATIONS=x make run
run: $(TARGET)
	. ./.env; ./$(RDIR)/main $$N_ITERATIONS

.PHONY: clean
clean:
	rm -f bin/**/main
	rm -f bin/**/*.o