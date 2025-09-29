CC = gcc
CFLAGS = -std=c99 -g -Wall -fsanitize=address,undefined

OBJS = mysh.o arraylist.o tokenizer.o

all: mysh

mysh: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o mysh

mysh.o: mysh.c arraylist.h tokenizer.h
	$(CC) $(CFLAGS) -c mysh.c

arraylist.o: arraylist.c arraylist.h
	$(CC) $(CFLAGS) -c arraylist.c

tokenizer.o: tokenizer.c tokenizer.h
	$(CC) $(CFLAGS) -c tokenizer.c

clean:
	rm -f *.o mysh