#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>    
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

/* El Puerto Abierto del nodo remoto */
#define PORT 3550         

/* El número máximo de datos en bytes */
#define MAXDATASIZE 300   

#define MAXRETRY 3

/* Estructura de mensaje */
struct message {
   int code;
   char * message;
};

/* Funcion para enviar los mensajes al servidor */
/* string es el texto a enviar */
/* fd es el file descriptor por el que se enviara el send */
int enviar(char * string, int fd){
   int len = strlen(string);
   int i;
   for( i = 0; i < MAXRETRY; i++ ){
      if ( send(fd, string, len, 0) > 0 ){
         return 0;
      }
      sleep(3000);
   } 
}

/* Funcion para conectarse al servidor */
/* fd es el file descriptor donde se conectara el servidor */
/* server es una estructura sockaddr_in con la informacion del servidor */
int conectar(int * fd, struct sockaddr_in * server){
   int retry;
   for(retry = 0; retry < MAXRETRY; retry++){
      printf("Intentando conectar al servidor..\n");
      if(connect(*fd, (struct sockaddr *)server,
         sizeof(struct sockaddr))!=-1){ 
         printf("Conectado\n");
         return 0;
      }
      sleep(2);
   }
   return -1;
}


/* Funcion para decodificar los mensajes del servidor */
/* msg es una estructura destino donde se decodificara el mensaje */
/* read es el mensaje recibido desde el servidor */
void parse_message(struct message * msg, char * read){
   msg->message = malloc(296 * sizeof(char));
   if( sscanf(read, "%d - %[^\t]", &msg->code, msg->message) < 0){
      printf("Error scanning: %s",read);
   }
   memset(read,0,MAXDATASIZE);
}

int main(int argc, char *argv[])
{
   /* ficheros descriptores */
   int fd, numbytes, flags, port, opcion;       
   int dflag = 0,pflag = 0,cflag = 0,iflag = 0;
   char * dvalue,* pvalue,* cvalue,* ivalue;

   /* en donde es almacenará el texto recibido */
   char buf[MAXDATASIZE];  

   /* estructura que recibirá información sobre el nodo remoto */
   struct hostent *he;         

   /* información sobre la dirección del servidor */
   struct sockaddr_in server;  

   /* estructura donde se recibiran los mensajes del servidor */
   struct message * msg = (struct message *) malloc(sizeof(struct message));

   /* esto es porque nuestro programa necesitará dos argumenos, la IP, la identificacion*/
   if (argc != 9) { 
      printf("Uso: bsb_cli -d <nombre_módulo_atención> -p <puerto_bsb_svr> -c <op>[d|r] -i <codigo_usuario>");
      exit(-1);
   }
   while (( flags = getopt(argc, argv, "d:p:c:i:")) != -1){
    switch(flags){
       case 'd':
           dflag = 1;
           dvalue = optarg;
           break;
       case 'p':
           pflag = 1;
           pvalue = optarg;
           break;
       case 'c':
           cflag = 1;
           cvalue = optarg;
           break;
       case 'i':
           iflag = 1;
           ivalue = optarg;
           break;
       default:
         printf("Uso: bsb_cli -d <nombre_módulo_atención> -p <puerto_bsb_svr> -c <op>[d|r] -i <codigo_usuario>");
         exit(-1);
       }
   }

   if (cvalue[0] == 'd'){
      opcion = 1;
   } else if (cvalue[0] == 'r'){
      opcion = 2;
   }

   /* llamada a gethostbyname() para resolver el dominio*/
   if ((he=gethostbyname(dvalue))==NULL){       
      perror("error en gethostbyname()\n");
      exit(-1);
   }

   /* llamada a socket() */
   if ((fd=socket(AF_INET, SOCK_STREAM, 0))==-1){  
      perror("error en socket()\n");
      exit(-1);
   }

   server.sin_family = AF_INET;
   port = atoi(pvalue);
   server.sin_port = htons(port); 
   /* htons() es necesaria nuevamente ;-o */
   server.sin_addr = *((struct in_addr *)he->h_addr);  
   /* he->h_addr pasa la información de ``*he'' a "h_addr" */
   bzero(&(server.sin_zero),8);

   if ( conectar(&fd,&server) == -1){
      printf("El servidor no responde.\n");
      exit(-1);
   };
   int opt;
   int i;
   printf("Enviando identificacion\n");
   send(fd,ivalue,5,0);
   
   // Recibiendo mensaje inicial
   msg->code = 10;
   while(msg->code != 3){
      switch(msg->code){
         case 0:
            printf("%s\n",msg->message);
            sprintf(buf,"%d", opcion);
            enviar(buf,fd);
            break;
         case 1:
            printf("Mensaje del Servidor: %s\n",msg->message);
            scanf("%d", &opt);
            sprintf(buf,"%d", opt);
            enviar(buf,fd);
            break;
         case 2:
            printf("%s\n",msg->message);
            exit(1);
            break;
      }
      if ((numbytes=recv(fd,buf,MAXDATASIZE,0)) == -1){
         perror(" error Error \n");
         exit(-1);
      }
      parse_message(msg, buf);
   }

   /* liberamos la estructura de mensaje */
   free(msg);
   
   /* cerramos el fd de comunicacion */
   close(fd);

}
