CC = gcc
CFLAGS = -Wall

myshell: myshell.c
	$(CC) $(CFLAGS) myshell.c -o myshell 

.PHONY: clean

clean:
	rm -f myshell
