/* El número máximo de datos en bytes */
#define MAXDATASIZE 300   

/* Estructura de mensaje */
struct message {
   char len[3];
   int code;
   char * message;
};

typedef enum {
  deposito,
  retiro,
  invalida
} transaccion;

char * inst[] = {"Deposito", "Retiro", "Invalida"};

/* funcion para encriptar nuestros mensajes */
/* string mensaje a ser encriptado */
/* len largo del mensaje a ser encriptado */
void xor(char * string, int len){
  int i;
  for(i = 0; i < len; i++){
    string[i] = string[i] ^ '`';
  }
}

/* estructura el mensaje para cumplir con el protocolo */
/* msg buffer de resultado */
/* m1  texto a enviar */
/* code codigo de protocolo a enviar */
void format_messge(char * msg, char * m1, int code){
  memset(msg, 0, MAXDATASIZE);
  sprintf(msg, "%d - %s", code, m1);
}

/* funcion de comunicacion con el cliente, permite cumplir 
facilemente con el protocolo asi como con la encriptacion */
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
