#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>    
#include <strings.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define PORT 3550         
/* El Puerto Abierto del nodo remoto */

#define MAXDATASIZE 100   
/* El número máximo de datos en bytes */

void enviar(char * string, int fd){
   int len = strlen(string);
   send(fd, string, len, 0);
}

int main(int argc, char *argv[])
{
   int fd, numbytes;       
   /* ficheros descriptores */

   char buf[MAXDATASIZE];  
   /* en donde es almacenará el texto recibido */

   struct hostent *he;         
   /* estructura que recibirá información sobre el nodo remoto */

   struct sockaddr_in server;  
   /* información sobre la dirección del servidor */

   if (argc !=3) { 
      /* esto es porque nuestro programa sólo necesitará un
      argumento, (la IP) */
      printf("Uso: %s <Dirección IP>\n",argv[0]);
      exit(-1);
   }

   if ((he=gethostbyname(argv[1]))==NULL){       
      /* llamada a gethostbyname() */
      perror("error en gethostbyname()\n");
      exit(-1);
   }

   if ((fd=socket(AF_INET, SOCK_STREAM, 0))==-1){  
      /* llamada a socket() */
      perror("error en socket()\n");
      exit(-1);
   }

   server.sin_family = AF_INET;
   server.sin_port = htons(PORT); 
   /* htons() es necesaria nuevamente ;-o */
   server.sin_addr = *((struct in_addr *)he->h_addr);  
   /*he->h_addr pasa la información de ``*he'' a "h_addr" */
   bzero(&(server.sin_zero),8);

   if(connect(fd, (struct sockaddr *)&server,
      sizeof(struct sockaddr))==-1){ 
      /* llamada a connect() */
      perror("error en connect()\n");
      exit(-1);
   }
   buf[0] = '1';
   int opt;
   printf("Enviando identificacion\n");
   send(fd,argv[2],5,0);
   while(buf[0] != '0'){

      if ((numbytes=recv(fd,buf,MAXDATASIZE,0)) == -1){  
         /* llamada a recv() */
         perror(" error Error \n");
         exit(-1);
      }

      buf[numbytes]='\0';
      if (numbytes != 0){
         printf("Mensaje del Servidor: %s\n",buf); 
      }
      scanf("%d", &opt);
      printf("Ha escogido la opcion %d\n", opt);
      sprintf(buf,"%d", opt);
      enviar(buf,fd);
      
   }



   close(fd);   /* cerramos fd =) */

}
