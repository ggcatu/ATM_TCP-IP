#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>    
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* El número máximo de datos en bytes */
#define MAXDATASIZE 300   

/* El numero maximo de intentos */
#define MAXRETRY 3

/* Estructura de mensaje */
struct message {
   char len[3];
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

/* funcion para encriptar nuestros mensajes */
/* string mensaje a ser encriptado */
/* len largo del mensaje a ser encriptado */
void xor(char * string, int len){
   int i;
   for(i = 0; i < len; i++){
      string[i] = string[i] ^ '`';
   }
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

void error_entrada(){
   printf("Uso: bsb_cli -d <nombre_módulo_atención> -p <puerto_bsb_svr> -c <op>[d|r] -i <codigo_usuario>\n");
   exit(-1);
}

void recibir_mensaje(char * buffer, int fd){
   int numbytes, tam;
   int ctam = 0;
   char tamc[3] = "", amc[3]= "";
   char tmensaje[300] = "", mensaje[300] = "";
   
   while( ctam < 3){
      if ((numbytes=recv(fd,amc,3-ctam,0)) == -1){
         perror(" error Error \n");
         exit(-1);
      }
      strcat(tamc, amc);
      ctam+=numbytes;
   }
   tam = atoi(tamc);
   ctam = 0;
   while( ctam < tam){
      if ((numbytes=recv(fd,mensaje,tam - ctam,0)) == -1){
            perror(" error Error \n");
            exit(-1);
      }
      strcat(tmensaje, mensaje);
      ctam += numbytes;
   }
   xor(tmensaje, tam);
   strcpy(buffer, tmensaje);
}



int main(int argc, char *argv[])
{
   /* ficheros descriptores */
   int fd, numbytes, flags, port, opcion = 0, opt;       
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

   /* esto es porque nuestro programa necesitará varios argumentos*/
   if (argc != 9) { 
      error_entrada();
   }
   /* manejador de argumentos */
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
         error_entrada();
       }
   }
   if (dflag + pflag + cflag + iflag != 4){
      error_entrada();
   }
   /* transformacion del argumento -c */
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

   /* argumento -p */
   port = atoi(pvalue);

   /* configuracion del servidor donde nos conectaremos */   
   server.sin_family = AF_INET;
   server.sin_port = htons(port); 
   server.sin_addr = *((struct in_addr *)he->h_addr);  
   bzero(&(server.sin_zero),8);

   /* nos intentaremos conectar al servidor 3 veces */
   if ( conectar(&fd,&server) == -1){
      printf("El servidor no responde.\n");
      exit(-1);
   };

   /* se envia la identificacion del usuario */
   printf("Enviando identificacion\n");
   send(fd,ivalue,5,0);

   /* manejador de mensajes recibidos */   
   msg->code = 10;
   while(msg->code != 9){
      switch(msg->code){
         case 0:
            /* enviando mensaje que indica tipo de transaccion */
            printf("%s\n",msg->message);
            sprintf(buf,"%d", opcion);
            enviar(buf,fd);
            break;
         case 1:
            /* el servidor nos esta requiriendo una respuesta numerica */
            printf("Mensaje del Servidor: %s\n",msg->message);
            if (scanf("%d", &opt) == 0){
               printf("Se ha ingresado un valor incorrecto, reinicie la operacion.\n");
               exit(-1);
            }
            sprintf(buf,"%d", opt);
            enviar(buf,fd);
            break;
         case 2:
            /* el servidor nos indica que debemos mostrar un mensaje y cerrar nuestro cliente */
            printf("%s\n",msg->message);
            exit(1);
            break;
         case 3:
            printf("%s\n",msg->message);
            break;
      }
      
      recibir_mensaje(buf, fd);
      parse_message(msg, buf);
   }

   /* liberamos la estructura de mensaje */
   free(msg);
   
   /* cerramos el fd de comunicacion */
   close(fd);

}
