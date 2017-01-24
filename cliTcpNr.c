#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  // mesajul trimis
  int nr=0;
  char buf[10];
  char username[20];
  int timeStart;

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* Adaugarea unui pseudonim */
  printf("Introduceti username-ul: ");
  scanf("%s", username);  

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

  /* citirea pozitiei countdown-ului dat de server 
     (apel blocant pina cind serverul raspunde) */
  if (read (sd, &timeStart, sizeof(int)) < 0)
    {
      perror ("[client]Eroare la read() de la server a pozitiei countdown-ului.\n");
      return errno;
    }
  int time=timeStart;
  while(time>=0)
    {
      //printf("%f\n",(double)time/1000000);
      if(((double)time/1000000)==(time/1000000))
	{
	  if(time==0)
	    printf("\r        JOCUL A INCEPUT!      ");
	  else if(time>9000000)
	    printf("\rJocul va incepe in: %d secunde",time/1000000);
	  else
	    printf("\rJocul va incepe in: 0%d secunde",time/1000000);
	  fflush(stdout);
	}
      time=time-100000;
      usleep(100000);
    }

  /* trimiterea username-ului la server */
  if (write (sd, username, sizeof(username)) <= 0)
    {
      perror ("[client]Eroare la write() spre server a username-ului.\n");
      return errno;
    }

  /* citirea username-ului dat de server 
     (apel blocant pina cind serverul raspunde) */
  if (read (sd, username, sizeof(username)) < 0)
    {
      perror ("[client]Eroare la read() de la server a username-ului.\n");
      return errno;
    }
  printf("Te-ai logat cu username-ul: %s\n", username);
  

  /* citirea mesajului */
  printf ("[client]Introduceti un numar: ");
  fflush (stdout);
  read (0, buf, sizeof(buf));
  nr=atoi(buf);
  //scanf("%d",&nr);
  
  printf("[client] Am citit %d\n",nr);

  /* trimiterea mesajului la server */
  if (write (sd,&nr,sizeof(int)) <= 0)
    {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }

  /* citirea raspunsului dat de server 
     (apel blocant pina cind serverul raspunde) */
  if (read (sd, &nr,sizeof(int)) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
    }
  /* afisam mesajul primit */
  printf ("[client]Mesajul primit este: %d\n", nr);

  /* inchidem conexiunea, am terminat */
  close (sd);
}
