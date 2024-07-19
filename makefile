CC = gcc 
OBJS = primecount.o 
ALL= primecount
DEBUG = -g
CFLAGS = -std=c99 -Wall -Werror $(DEBUG) 

$(ALL):$(OBJS) 
	$(CC) primecount.o -o primecount -lm 

primecount.o: primecount.c
	gcc -g -c primecount.c 
clean:
	rm -f $(OBJS) $(ALL)

