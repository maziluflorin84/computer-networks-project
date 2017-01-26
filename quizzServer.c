#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int timeStart=20;

typedef struct thData{
  int idThread; //id-ul thread-ului tinut in evidenta de acest program
  int cl; //descriptorul intors de accept
  char username[20]; //username-ul tinut in evidenta de acest program
  int startTime;     //imtpul curent inainte de startul intrebarilor
}thData;

int callback(void *, int, char **, char **);

void *getQuestions();

/* functia executata de thread-ul pentru countdown timer */
void *countdown(void *);

/* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
static void *treat(void *);

void raspunde(void *);

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket
  char rv[5];           //valoare returnata la finalizarea procesului copil
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea pentru clienti
  pthread_t countdownThread;
  int i=0;

  /* Este creat thread-ul pentru countdown-ul de la inceperea quizz-ului*/
  pthread_create(&countdownThread, NULL, countdown, NULL);
  //pthread_join(countdownThread, NULL);
  
  
  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;	
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }
  
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
    {
      int client;
      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);
      
      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);
	  
      //client= malloc(sizeof(int));
      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
	{
	  perror ("[server]Eroare la accept().\n");
	  continue;
	}


      char username[20]; //username-ul
      /* Este citit username-ul*/
      if (read (client, username, sizeof(username)) <= 0)
	{
	  perror ("Eroare la read() de la client a username-ului.\n");		
	}
            
	  	
      /* s-a realizat conexiunea, se astepta mesajul */
      int idThread; //id-ul threadului
      int cl; //descriptorul intors de accept      
      int startTime; 

      td=(struct thData*)malloc(sizeof(struct thData));	
      td->idThread=i++;
      td->cl=client;
      strcpy(td->username,username);
      td->startTime=timeStart;

      pthread_create(&th[i], NULL, &treat, td);
				
    }//while
      
};

/* Functia ce afiseaza valorile din baza de date SQLite */
int callback(void *NotUsed, int argc, char **argv, char **azColName) {
  NotUsed = 0;
  for (int i = 0; i < argc; i++)
    {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
  printf("\n");    
  return 0;
}

/* Functia ce ruleaza interogarea sql */
void *getQuestions()
{
  sqlite3 *db;
  char *err_msg = 0;
    
  int rc = sqlite3_open("questions.db", &db);
  if(rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
  }

  char *sql = "SELECT * FROM questions";
        
  rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
  if (rc != SQLITE_OK )
    { 
      fprintf(stderr, "Failed to select data\n");
      fprintf(stderr, "SQL error: %s\n", err_msg);
      sqlite3_free(err_msg);
      sqlite3_close(db);
    }
  sqlite3_close(db);
};

void *countdown(void *arg)
{
  int time=20000000;
  int checker=20;
  while(time>=0)
    {
      timeStart=time;
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
  printf("\n");

  getQuestions();
};

static void *treat(void * arg)
{	  
  struct thData tdL;
  tdL= *((struct thData*)arg);
  
  printf ("[thread]- %d - %s - Asteptam mesajul...\n", tdL.idThread, tdL.username);
  fflush (stdout);
  pthread_detach(pthread_self());		
  raspunde((struct thData*)arg);
  /* am terminat cu acest client, inchidem conexiunea */
  close ((intptr_t)arg);
  return(NULL);	
  		
};


void raspunde(void *arg)
{
  int nr, i=0;
  char username[20];
  char message[10];
  struct thData tdL; 
  tdL= *((struct thData*)arg);

  /* returnam pozitia countdown-ului clientului */
  if (write (tdL.cl, &timeStart, sizeof(tdL.startTime)) <= 0)
    {
      printf("[Thread %d] ",tdL.idThread);
      perror ("[Thread]Eroare la write() catre client a pozitiei countdown-ului.\n");
    }
  
  if (read (tdL.cl, &nr, sizeof(int)) <= 0)
    {
      printf("[Thread %d]\n",tdL.idThread);
      perror ("Eroare la read() de la client.\n");
			
    }
	
  printf ("[Thread %d]Mesajul a fost receptionat...%d\n",tdL.idThread, nr);
		      
  /*pregatim mesajul de raspuns */
  nr++;      
  printf("[Thread %d]Trimitem mesajul inapoi...%d\n",tdL.idThread, nr);
		      
		      
  /* returnam mesajul clientului */
  if (write (tdL.cl, &nr, sizeof(int)) <= 0)
    {
      printf("[Thread %d] ",tdL.idThread);
      perror ("[Thread]Eroare la write() catre client.\n");
    }
  else
    printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	

}
