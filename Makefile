CC = gcc
CFLAGS = -g

all : central tcpechoclient 

central : central.c
	$(CC) $(CFLAGS) -o central central.c -lpthread

tcpechoclient : tcpechoclient.c 
	$(CC) $(CFLAGS) -o tcpechoclient tcpechoclient.c

clear :
	rm *.o


