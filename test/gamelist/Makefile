TOPDIR	= ../..
CC		= gcc
CFLAGS  = -g -Wall -DDEBUG
INCLUDE =  -I$(TOPDIR)/src/main -I$(TOPDIR)/src/utils \
			-I$(TOPDIR)/src/interface -I$(TOPDIR)\
			-I$(TOPDIR)/src/configuration -I$(TOPDIR)/src/log \
			-I$(TOPDIR)/src/protocols \
			`pkg-config glib-2.0 --cflags-only-I` \
			`pkg-config gtk+-2.0 --cflags-only-I` \
			`pkg-config libxml-2.0 --cflags-only-I`
LIBS	= `pkg-config glib-2.0 --libs` \
			`pkg-config libxml-2.0 --libs`

MUDSRC	= $(TOPDIR)/src/utils/utils.o \
			$(TOPDIR)/src/utils/strings.o \
			$(TOPDIR)/src/interface/gamelist.o \
			$(TOPDIR)/src/log/debug.o

TARGET	= gamelist
SOURCE	= $(MUDSRC) main.o
OBJECTS	= main.o gamelist.o utils.o debug.o strings.o

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)

default: all

all: $(TARGET)

clean:
	rm -f *.o core $(TARGET)

# DO NOT DELETE
	