#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>   
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include "general.c"

#define MAXCLIENT 100
#define MINICIAL 8000
#define MAXIMORETIRO 3000
#define MAXNRETIROS 3
#define MAXDATASIZE 300
#define NAVISORETIRO 5000

/* estructura que mantiene una transaccion de un cliente */
struct cliente {
	int monto;
	int error;
	transaccion instruccion;
	char name[100];
	char fecha[30];
	char error_message[100];
	struct message last_packet;
};

int monto_total, imov, fsocket;
FILE *f;
pthread_mutex_t monto_mutex;
pthread_mutex_t mov_mutex;
pthread_mutex_t bit_mutex;
struct cliente * movimientos[MAXCLIENT];
char * dvalue, * fretiro, * fdeposito;


/* funcion que permite imprimir en pantalla un movimiento */
/* c estructura de cliente a imprimir */
void printmov(struct cliente * c){
	printf("%s - Usuario: [ %s ] || Transaccion: %s || Monto: %d\n", c->fecha, c->name, inst[(int)c->instruccion], c->monto);
}

/* funcion que permite imprimir todos las transacciones efectuadas en un dia */
void imprimir_movimientos(){
	int i = 0;
	for( i = 0; i < imov; i++){
		printmov(movimientos[i]);
	}
}

/* funcion para escribir un string en la bitacora */
/* string texto a ser escrito en la bitacora */
void escribir_bitacora(char * string, char * file){
   	pthread_mutex_lock( &bit_mutex );
    f = fopen(file, "a");
    fprintf(f, "%s", string);
    fclose(f);
   	pthread_mutex_unlock( &bit_mutex );
}

/* funcion de inicializacion de la bitacora */
void inicializar_bitacora(){
    char temp[100];
    sprintf(temp, "# INICIALIZANDO DIA #\n");
	escribir_bitacora(temp, fdeposito);
	escribir_bitacora(temp, fretiro);
}

/* funcion de finalizacion de dia para la bitacora */
void finalizar_bitacora(){
	char temp[100];
    sprintf(temp, "Cerrando dia. Monto disponible: %d\n", monto_total);
	escribir_bitacora(temp, fdeposito);
	escribir_bitacora(temp, fretiro);
}

/* funcion para escribir un movimiento en la bitacora */
/* c movimiento a escribir en la bitacora */
void agregar_movimiento_bitacora(struct cliente * c){
	char temp[200];
	if(c->error){
		sprintf(temp, "%s - Usuario: [ %s ] || Transaccion: %s || Monto: %d || Error: %s \n", c->fecha, c->name, inst[(int)c->instruccion], c->monto, c->error_message);
	} else {
		sprintf(temp, "%s - Usuario: [ %s ] || Transaccion: %s || Monto: %d\n", c->fecha, c->name, inst[(int)c->instruccion], c->monto);
	}
	if(c->instruccion == deposito){
		escribir_bitacora(temp,fdeposito);
	} else {
		escribir_bitacora(temp,fretiro);
	}
}

/* funcion de inicializacion de dia de la central */
void inicializar(){
	monto_total = MINICIAL;
	imov = 0;
	inicializar_bitacora();
	printf("Cajero inicializando.\n");
}

/* funcion de finalizacion del cajero */
void finalizar(){
	int i;
	finalizar_bitacora();
	close(fsocket);
	for( i = 0; i < imov; i++){
		free(movimientos[i]);
	}
	exit(1);
}

/* funcion de reinicio de dia del cajero */
void reiniciar(){
	finalizar_bitacora();
	inicializar();
}

/* funcion para guardar un movimiento en nuestra memoria */
/* movimiento a ser guardado */
void agregar_movimiento(struct cliente * c){
	pthread_mutex_lock( &mov_mutex );
	agregar_movimiento_bitacora(c);
	movimientos[imov] = c;
	imov++;
	pthread_mutex_unlock( &mov_mutex );
}

/* funcion para verificar que una persona no retire mas de MAXNRETIRO veces en un dia */
/* c cliente que busca retirar */
int verificarRetiros(struct cliente * c){
	imprimir_movimientos();
	int i,k=0;
	struct cliente * temp; 
	for( i = 0; i < imov; i++){
		if ( strcmp(c->name, movimientos[i]->name) == 0 && movimientos[i]->instruccion == retiro && movimientos[i]->monto < 0){
			k++;
		}
	}
	return k < MAXNRETIROS;
}


/* funcion que maneja los errores durante las transacciones*/
/* c estructura que mantiene informacion de la transaccion */
/* mensaje mensaje de error */
/* ds descriptor del cliente */
void set_error(struct cliente * c, char * mensaje, int ds){
	char message[MAXDATASIZE];
	c->error = 1;
	strcpy(c->error_message,mensaje);
	printf("%s\n", mensaje);
	format_messge(message, mensaje, 3);
	enviar(message, ds);

}

/* funcion que permite el incremento/decremento del total del cajero */
/* n diferencia del monto total a procesar*/
int incrementar_monto(int n){
	if( -n > monto_total ){
		printf("Se intento retirar mas de lo que posee el cajero.\n");
		return -2;
	}
	if ( -n > MAXIMORETIRO ){
		printf("No se puede retirar mas de 3000.\n");
		return -1;
	}
	pthread_mutex_lock( &monto_mutex );
	monto_total += n;
	pthread_mutex_unlock( &monto_mutex );
	printf("Monto disponible en el cajero: %d\n", monto_total);
	return 1;
}

/* modulo de depositos, una vez recibida una solicitud de deposito */
/* c estructura de cliente para almacenar la transaccion */
/* ds descriptor utilizado para la comunicacion con el cliente */
void manejador_deposito(struct cliente * c,int ds){
	char message[MAXDATASIZE];
	char buffer[10];
	int numbytes = 0;
	int total;
	c->instruccion = deposito;
	printf("Iniciando manejador de deposito\n");
	format_messge(message, "Monto a depositar: \n", 1);
  	enviar(message, ds);	

  	recibir_mensaje(buffer,ds);
	parse_message(&c->last_packet, buffer);
	total = atoi(c->last_packet.message);

	if (total > 0){
		c->monto = total;
		incrementar_monto(total);
		printf("Monto recibido\n");
		format_messge(message, "Su deposito se ha efectuado.\n", 3);
  		enviar(message, ds);
	} else {
		set_error(c, "Se ingreso un monto incorrecto.", ds);
	}
	return;
}

/* modulo de retiros, una vez recibida una solicitud de retiro */
/* c estructura de cliente para almacenar la transaccion */
/* ds descriptor utilizado para la comunicacion con el cliente */
void manejador_retiro(struct cliente * c, int ds){
	char message[MAXDATASIZE];
	char buffer[MAXDATASIZE];
	int numbytes = 0;
	int total, res;
	c->instruccion = retiro;
	printf("Iniciando manejador de retiro\n");
	if (!verificarRetiros(c)){
		printf("Ya ha exedido el limite de retiros diarios.\n");
		set_error(c, "Ya ha exedido el limite de retiros diarios.", ds);
		return;
	}
	if(monto_total < NAVISORETIRO){
		printf("El cajero posee menos de %d\n", NAVISORETIRO);
		format_messge(message, "El cajero posee menos de 5000, tenga esto en cuenta al momento de retirar.\n", 3);
  		enviar(message, ds);
	}
	format_messge(message, "Monto a retirar: \n", 1);
  	enviar(message, ds);

  	recibir_mensaje(buffer,ds);
	parse_message(&c->last_packet, buffer);

	total = -atoi(c->last_packet.message);
	if (total >= 0){
		res = -1;
		c->monto = 0;
		set_error(c, "Monto incorrecto", ds);
		return;
	} else {
		c->monto = total;
		res = incrementar_monto(total);
	}

	if (res == -1){
		c->monto = 0;
		set_error(c, "Debe retirar un monto <= 3000.", ds);
	} else if (res == -2){
		c->monto = 0;
		set_error(c, "El cajero no posee suficiente efectivo.", ds);
	} else if (res >= 0) {
		printf("Monto retirado\n");
		format_messge(message, "Ha retirado su dinero.\n", 3);
  		enviar(message, ds);
	}
	return;
}

/* funcion para enviar el comprobante al cajero para ser emitido */
/* c transaccion efectuada */
/* descriptor file descriptor de comunicacion con el cajero */
void enviar_comprobante(struct cliente * c, int descriptor){
	time_t timer;
    char buffer[26];
    char r[100] = "Transaccion registrada: ";
    char message[MAXDATASIZE];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(c->fecha, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    strcpy(buffer,c->fecha);
    if (c->error){
    	sprintf(r,"%s [%s] = %s = %s (%s)",r,c->name,inst[(int)c->instruccion],buffer, c->error_message);
    } else {
    	sprintf(r,"%s [%s] = %s = %s",r,c->name,inst[(int)c->instruccion],buffer);
    }
	format_messge(message, r, 3);
	enviar(message, descriptor);
}

/* modulo de atencion al cliente, una vez recibida una solicitud de conexion */
/* fd descriptor utilizado para la comunicacion con el cliente */
void atencion_cliente(void * fd){
	char message[MAXDATASIZE];
	int descriptor = *(int *) fd;
	struct cliente *c = (struct cliente *)malloc(sizeof(struct cliente));
	int numbytes = 0, opcion;
	char buffer[300];

	c->error = 0;
	printf("Esperando identificacion de usuario..\n");

	recibir_mensaje(buffer,descriptor);
	parse_message(&c->last_packet, buffer);

	strcpy(c->name,c->last_packet.message);
	printf("Usuario [ %s ] conectado.\n", c->name);
	format_messge(message, "Bienvenido, se te ha asignado un proceso.\n Selecciona tu opcion: \n -d. Depositar \n -r. Retirar \n", 0);
	enviar(message, descriptor);

	recibir_mensaje(buffer,descriptor);
	parse_message(&c->last_packet, buffer);

  	opcion = atoi(c->last_packet.message);
  	printf("Opcion del cliente [ %s ] : %d\n",c->name, opcion);
  	switch (opcion) {
  		case 1:
  			manejador_deposito(c, descriptor);
  			break;
  		case 2:
			manejador_retiro(c, descriptor);
  			break;
  		default:
  			c->instruccion = invalida;
  			set_error(c, "No se conoce el numero de operacion.", descriptor);
  			break;
  		}
  
  	enviar_comprobante(c,descriptor);
  	agregar_movimiento(c);
  	printf("Transaccion finalizada, cerrando descriptor %d\n", descriptor);
	format_messge(message, "Transaccion finalizada.", 2);
	enviar(message, descriptor);
	free(c->last_packet.message);
	close(descriptor);
	return;
}

/* imprime un error y sale del programa, puesto que la invocacion fue incorrecta */
void error_entrada(){
   printf("Uso: bsb_svr -l <puerto a ofrecer> -i <bitacora de deposito> -o <bitacora de retiro>\n");
   exit(-1);
}

int main(int argc, char *argv[]){
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTSTP, reiniciar);
	signal(SIGINT, finalizar);
	char * lvalue;
	int fd2, numbytes, flags, lflag, iflag, oflag, port;
	struct sockaddr_in server; 
   	struct sockaddr_in client;
   	pthread_t client_t;
   	int size_sock = sizeof(struct sockaddr_in);
   	int option = 1;

   	while (( flags = getopt(argc, argv, "l:i:o:")) != -1){
    switch(flags){
       case 'l':
           lflag = 1;
           lvalue = optarg;
           break;
       case 'i':
           iflag = 1;
           fdeposito = optarg;
           break;
       case 'o':
           oflag = 1;
           fretiro = optarg;
           break;
       default:
         error_entrada();
       }
    }

    port = atoi(lvalue);

    /* verifica que todos los flags fueron activados */
    if (oflag + iflag + lflag != 3){
    	error_entrada();
    	exit(-1);
    }

    inicializar();

   	// Creando socket
   	// int socket(int domain, int type, int protocol);  
	if ( ( fsocket = socket(AF_INET, SOCK_STREAM, 0) ) == -1 ) {
		perror("Error en la llamada Socket()");
		exit(-1);
	}
	setsockopt(fsocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	// Configurando estructura para el bind
	memset(&server, 0, sizeof(server)); 
    server.sin_family = AF_INET;         
    server.sin_port = htons(port); 
    server.sin_addr.s_addr = INADDR_ANY; 
    
    //int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    if ( bind(fsocket, (struct sockaddr *) & server, sizeof(server) ) == -1){
    	perror("Error en la llamada de Bind()");
    	exit(-1);

    }

    //int listen(int sockfd, int backlog);
    if ( listen(fsocket, MAXCLIENT) == -1){
    	perror("Error en la llamada de Listen()");
    	exit(-1);
    }

    while(1){
    	if ( (fd2 = accept(fsocket, NULL , NULL))  < -1){
	    		perror("Error en el Accept()");
	    		exit(-1);
	    }
	    
		printf("Cajero conectandose...\n");
   
    	if( pthread_create(&client_t, NULL, (void *)&atencion_cliente, (void *)&fd2) ) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
    }
}