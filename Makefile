IDIR=inc
CC=g++
CFLAGS=-g -Wall -lpthread -I$(IDIR)

ODIR=obj
SDIR=src

TARGET=main

_DEPS = monitor.hpp person.hpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o monitor.o person.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o
	rm -f $(TARGET)
	rm -f $(TESTER) 2> /dev/null