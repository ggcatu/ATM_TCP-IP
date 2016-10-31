CC = gcc
CFLAGS = -g

all : bsb_svr bsb_cli 

bsb_svr : central.c general.c
	$(CC) $(CFLAGS) -o bsb_svr central.c -lpthread

bsb_cli : tcpechoclient.c general.c
	$(CC) $(CFLAGS) -o bsb_cli tcpechoclient.c

clean :
	rm bsb_svr bsb_cli


