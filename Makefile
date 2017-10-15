CFLAGS = -Wall -g
LDFLAGS = -lreadline

OBJS = mysh.o linked_list.o

all: mysh linked_list.o

mysh: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

linked_list.o: linked_list.c linked_list.h
	$(CC) $(CFLAGS) -c linked_list.c

clean:
	rm -f *.o *~ mysh

.PHONY: all clean

