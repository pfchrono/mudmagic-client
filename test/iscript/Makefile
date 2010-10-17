CC		= gcc
CFLAGS  = -g -Wall

SRCDIR  = ../../src
#INCLUDE = -I../../utils -I../../main -I.. `pkg-config glib-2.0 --cflags-only-I`
#LIBS	= `pkg-config glib-2.0 --libs`
INCLUDE = -I$(SRCDIR)/script
LIBS	=

TARGET	= basic
SOURCE	= main.o $(SRCDIR)/script/iscript.o
OBJECTS	= main.o iscript.o

BAS_EXT = .bas
RES_EXT = .res
TESTS_SRC	= $(wildcard *.bas)
#TESTS_RES	= $(patsubst %.bas,%,$(SOURCEFILES))
TESTS_RES	= test1.res


#$(TESTS_RES): % : %.res

#%.res: %.bas:
#	./$(TARGET) $< > $@

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)

default: all

all: $(TARGET) $(TESTS_RES)

clean:
	rm -f *.o *$(RES_EXT) $(TARGET)

# DO NOT DELETE
