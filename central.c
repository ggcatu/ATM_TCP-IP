#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>   
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PUERTO 3550
#define MAXCLIENT 100
#define MINICIAL 80000

int monto_total = MINICIAL;

struct cliente {
	int monto;
	char name[100];
};

void enviar(char * string, int fd){
	int len = strlen(string);
	send(fd, string, len, 0);
}


void manejador_deposito(int ds){
	char buffer[10];
	int numbytes = 0;
	int total;
  	enviar("Monto a depositar: \n", descriptor);
  	if ( (numbytes=recv(descriptor,buffer,10,0)) == -1){  
	    perror("Error en la llamada Recv()");
	    exit(-1);
	}
	total = atoi(buffer);
	incrementar_monto(total);
}


void atencion_cliente(void * fd){
	int descriptor = *(int *) fd;
	printf("%d\n", descriptor);
	struct cliente c;
	int p = 100;
	int numbytes = 0;
	char buffer[10];

	printf("Esperando identificacion de usuario..\n");
	while(numbytes == 0){
		if( (numbytes=recv(descriptor,buffer,10,0)) == -1){  
		      perror("Error en la llamada Recv()");
		      exit(-1);
	   	}
	   	if (numbytes != 0){
		  	strcpy(c.name, buffer);
		  	bzero(buffer, 10); 
	   	 }
		
	}
	enviar("Bienvenido, se te ha asignado un proceso.\n Selecciona tu opcion: \n 1. Depositar \n 2. Retirar \n", descriptor);
	
	if ( (numbytes=recv(descriptor,buffer,10,0)) == -1){  
	    perror("Error en la llamada Recv()");
	    exit(-1);
	}

	if (numbytes != 0){
  	int opcion;
  	opcion = atoi(buffer);
  	printf("Mensaje del Cliente[ %s ] : %s\n",c.name, buffer);
  	switch (opcion) {
  		case 1:
  			manejador_deposito(descriptor);
  			break;
  		case 2:
  			enviar("Monto a retirar: \n", descriptor);
  			break;
  		default:
  			return;
  		}
  	}

	close(descriptor);
	return;
}

int main(){
	int fde, fd2;
	struct sockaddr_in server; 
   	struct sockaddr_in client; 
   	int size_sock = sizeof(struct sockaddr_in);
   	int option = 1;
   	// Creando socket
   	// int socket(int domain, int type, int protocol);  
	if ( ( fde = socket(AF_INET, SOCK_STREAM, 0) ) == -1 ) {
		perror("Error en la llamada Socket()");
		exit(-1);
	}
	setsockopt(fde, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	// Configurando estructura para el bind
	memset(&server, 0, sizeof(server)); 
    server.sin_family = AF_INET;         
    server.sin_port = htons(PUERTO); 
    server.sin_addr.s_addr = INADDR_ANY; 
    
    //int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    if ( bind(fde, (struct sockaddr *) & server, sizeof(server) ) == -1){
    	perror("Error en la llamada de Bind()");
    	exit(-1);

    }

    //int listen(int sockfd, int backlog);
    if ( listen(fde, MAXCLIENT) == -1){
    	perror("Error en la llamada de Listen()");
    	exit(-1);
    }

    pthread_t client_t;
    int numbytes;
    char buffer[10];


    while(1){
    	if ( (fd2 = accept(fde, NULL , NULL))  < -1){
	    		perror("Error en el Accept()");
	    }
	    
		printf("Cajero conectandose... Asignando thread.\n");
   
    	if( pthread_create(&client_t, NULL, (void *)&atencion_cliente, (void *)&fd2) ) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
    }

}