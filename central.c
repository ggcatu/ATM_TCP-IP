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
#define MAXIMORETIRO 3000
#define MAXNRETIROS 3

typedef enum {
	deposito,
	retiro
} transaccion;

struct cliente {
	int monto;
	transaccion instruccion;
	char name[100];
};

int monto_total;
int imov;
pthread_mutex_t monto_mutex;
pthread_mutex_t mov_mutex;
struct cliente * movimientos[MAXCLIENT];

void printmov(struct cliente * c){
	char * inst[] = {"Deposito", "Retiro"};
	printf("Usuario: [ %s ] || Transaccion: %s || Monto: %d\n", c->name, inst[(int)c->instruccion], c->monto);
}
void imprimir_movimientos(){
	int i = 0;
	for( i = 0; i < imov; i++){
		printmov(movimientos[i]);
	}
}

void inicializar(){
	monto_total = MINICIAL;
	imov = 0;
}

void agregar_movimiento(struct cliente * c){
	pthread_mutex_lock( &mov_mutex );
	// Escribir movimiento en bitacora;
	movimientos[imov] = c;
	imov++;
	pthread_mutex_unlock( &mov_mutex );
}

int verificarRetiros(struct cliente * c){
	imprimir_movimientos();
	// Necesita mutex?
	int i,k=0;
	struct cliente * temp; 
	for( i = 0; i < imov; i++){
		if ( strcmp(c->name, movimientos[i]->name) == 0 && movimientos[i]->instruccion == retiro && movimientos[i]->monto < 0){
			k++;
		}
	}
	printf("K: %d, IMOV %d\n", k, imov);
	return k < 3;
}

void enviar(char * string, int fd){
	int len = strlen(string);
	send(fd, string, len, 0);
}

int incrementar_monto(int n){
	if( -n > monto_total || -n > MAXIMORETIRO ){
		printf("ERROR\n");
		return -1;
	}
	pthread_mutex_lock( &monto_mutex );
	monto_total += n;
	pthread_mutex_unlock( &monto_mutex );
	printf("New ammount: %d\n", monto_total);
	return 1;
}

void manejador_deposito(struct cliente * c,int ds){
	char buffer[10];
	int numbytes = 0;
	int total;
	c->instruccion = deposito;
	printf("Iniciando manejador de deposito\n");
  	enviar("Monto a depositar: \n", ds);
  	if ( (numbytes=recv(ds,buffer,10,0)) == -1){  
	    perror("Error en la llamada Recv()");
	    exit(-1);
	}
	total = atoi(buffer);
	if (total <= 0){
		c->monto = total;
		incrementar_monto(total);
		printf("Monto recibido\n");
	} else {
		printf("Error al depositar\n");
	}
	return;
}


void manejador_retiro(struct cliente * c, int ds){
	char buffer[10];
	int numbytes = 0;
	int total, res;
	c->instruccion = retiro;
	printf("Iniciando manejador de retiro\n");
	if (!verificarRetiros(c)){
		printf("Ya ha exedido el limite de retiros diarios.\n");
		return;
	}
  	enviar("Monto a retirar: \n", ds);
  	if ( (numbytes=recv(ds,buffer,10,0)) == -1){  
	    perror("Error en la llamada Recv()");
	    exit(-1);
	}
	total = -atoi(buffer);
	if (total > 0){
		res = -1;
		c->monto = 0;
	} else {
		c->monto = total;
		res = incrementar_monto(total);
	}

	if (res < 0){
		printf("No se ha efectuado la transaccion.\n");
		c->monto = 0;
	} else {
		printf("Monto retirado\n");
	}
	return;
}


void atencion_cliente(void * fd){
	int descriptor = *(int *) fd;
	struct cliente *c = (struct cliente *)malloc(sizeof(struct cliente));
	//int p = 100;
	int numbytes = 0;
	char buffer[10];

	printf("Esperando identificacion de usuario..\n");
	while(numbytes == 0){
		if( (numbytes=recv(descriptor,buffer,10,0)) == -1){  
		      perror("Error en la llamada Recv()");
		      exit(-1);
	   	}
	   	if (numbytes != 0){
		  	strcpy(c->name, buffer);
		  	bzero(buffer, 10); 
	   	 }
		
	}
	printf("Usuario [ %s ] conectado.\n", c->name);
	enviar("Bienvenido, se te ha asignado un proceso.\n Selecciona tu opcion: \n 1. Depositar \n 2. Retirar \n", descriptor);
	
	if ( (numbytes=recv(descriptor,buffer,10,0)) == -1){  
	    perror("Error en la llamada Recv()");
	    exit(-1);
	}

	if (numbytes != 0){
  	int opcion;
  	opcion = atoi(buffer);
  	printf("Mensaje del Cliente[ %s ] : %s\n",c->name, buffer);
  	switch (opcion) {
  		case 1:
  			manejador_deposito(c, descriptor);
  			break;
  		case 2:
			manejador_retiro(c, descriptor);
  			break;
  		default:
  			return;
  		}
  	}
  	agregar_movimiento(c);
  	printf("Transaccion finalizada, cerrando descriptor %d\n", descriptor);
	close(descriptor);
	return;
}

int main(){
	inicializar();
	int fde, fd2, numbytes;
	struct sockaddr_in server; 
   	struct sockaddr_in client;
   	pthread_t client_t;
    char buffer[10];
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

    while(1){
    	if ( (fd2 = accept(fde, NULL , NULL))  < -1){
	    		perror("Error en el Accept()");
	    }
	    
		printf("Cajero conectandose...\n");
   
    	if( pthread_create(&client_t, NULL, (void *)&atencion_cliente, (void *)&fd2) ) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
    }

}