CC=gcc
FLAGS=-Wall -g

all: TCP_Sender TCP_Receiver 

TCP_Sender: TCP_Sender.o 
	$(CC) $(FLAGS) -o TCP_Sender TCP_Sender.o 

TCP_Sender.o: TCP_Sender.c 
	$(CC) $(FLAGS) -c TCP_Sender.c

TCP_Receiver: TCP_Receiver.o LinkedList.o
	$(CC) $(FLAGS) -o TCP_Receiver TCP_Receiver.o LinkedList.o

TCP_Receiver.o: TCP_Receiver.c LinkedList.h
	$(CC) $(FLAGS) -c TCP_Receiver.c

LinkedList.o: LinkedList.c LinkedList.h
	$(CC) $(FLAGS) -c LinkedList.c

.PHONY: clean

clean:
	rm -f *.o *txt TCP_Sender TCP_Receiver