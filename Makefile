
# cory and nova use gcc version 3.3.2

TARGET=client

CC = gcc
DEBUG = -g #-v

LDFLAGS = -lpthread 
OS = LINUX

CCFLAGS = -Wall $(DEBUG) -D$(OS)

# add object file names here
OBJS = app.o utils.o

all: client

%.o : %.c
	$(CC) -c $(CCFLAGS) $<

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CCFLAGS) $(LDFLAGS) 

clean: 
	rm -rf *.o
