#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

//iggiotti.florian@gmail.com

#define PORT 6000
#define MAX_BUFFER 1000
#define MAX_CLIENTS 10
#define TAILLE 15

const char *EXIT = "exit";
    char tampon[MAX_BUFFER];
    char buffer[MAX_BUFFER];

int sema1; // id de sémaphore
int nbClients = 0; //nombre de client connecté

typedef struct 
{
	char pseudo[10];//nom du joueur
	char word[11];//mot distribué au joueur
	int score;//note du joueur
}infojoueur;

typedef struct 
{
  infojoueur joueur[10]; 
}infotousjoueurs;
      
//typedef struct infojoueur joueur;
void afficherInfo(infotousjoueurs* info);
void commencer(int fdSocketCommunication);
void ouvrirfichier(char mot[10][11]);
void echanger(char listemot[10][11], char mot[10][11]);
void jouer(char listemot[10][11],int fdSocketCommunication);
void preparerServeur();
void p(int semid);
void v(int semid);

void lireMessage(char tampon[]) {
    printf("Saisir un message à envoyer :\n");
    fgets(tampon, MAX_BUFFER, stdin);
    strtok(tampon, "\n");
}

void preparerServeur(){
    int fdSocketAttente;
    int fdSocketCommunication;
    struct sockaddr_in coordonneesServeur;
    struct sockaddr_in coordonneesAppelant;
    int nbRecu;
    int longueurAdresse;
    int pid;

    fdSocketAttente = socket(PF_INET, SOCK_STREAM, 0);
    if (fdSocketAttente < 0) {
        printf("socket incorrecte\n");
        exit(EXIT_FAILURE);
    }
    // On prépare l’adresse d’attachement locale
    longueurAdresse = sizeof(struct sockaddr_in);
    memset(&coordonneesServeur, 0x00, longueurAdresse);
    coordonneesServeur.sin_family = PF_INET;
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_port = htons(PORT);

    if (bind(fdSocketAttente, (struct sockaddr *) &coordonneesServeur, sizeof(coordonneesServeur)) == -1) {
        printf("erreur de bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(fdSocketAttente, 5) == -1) {
        printf("erreur de listen\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("The server is ready\n");
    }

    socklen_t tailleCoord = sizeof(coordonneesAppelant);

    

    while (nbClients < MAX_CLIENTS) {
        if ((fdSocketCommunication = accept(fdSocketAttente, (struct sockaddr *) &coordonneesAppelant,
                                            &tailleCoord)) == -1) {
            printf("erreur de accept\n");
            exit(EXIT_FAILURE);
        }
        printf("Client connecté - %s:%d\n",
               inet_ntoa(coordonneesAppelant.sin_addr),
               ntohs(coordonneesAppelant.sin_port));
        nbClients++;
        printf("nombre de client pour l'instant: %d\n",nbClients);
        /*--------activer multiples processus------------*/
            pid = fork();
        if(pid>0){ // processus père
            /*------------creer le semaphore-----------------*/
            key_t clefsema=ftok(".",20);
            sema1=semget(clefsema, 1, IPC_CREAT | 0666);
            semctl(sema1, 0, SETVAL,1);
            if(sema1==-1){
             printf("Erreur de semget\n");
             exit(0);
            }
            /*----------------------------------------------*/

            /*-----------creer la memoire partage-----------*/
           key_t clefshm=ftok(".",19);
            int shmid=shmget(clefshm,1000,0666|IPC_CREAT);
            if(shmid==-1){
                printf("Allocation de la shm en echec!\n");
                printf("%s\n",strerror(errno)); 
                exit(0);
            }
            infotousjoueurs *info;// la structure de l'information des joueurs
            p(sema1);
            info=shmat(shmid,NULL,0);
            if(info!=(void*)(-1))
             {   
              afficherInfo(info);
              shmdt(info);// déttacher un segment de mémoire partagé
              v(sema1);
           }
        }
          /*-------------------------------------------------*/
        
       if (pid == 0) { // processus fils
            //close(fdSocketAttente);
            while (1) {
              /*---------------------semaphore-----------------------*/
                key_t clefsema=ftok(".", 19);
                 int sema1=semget(clefsema, 1, IPC_CREAT|0666);// acceder au semaphore
                 if(sema1==-1){
                     printf("Erreur de semget\n");
                     exit(0);
                }          
              /*-----------------------------------------------------*/    
                
                nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0);// NO1 recv le code de commencer 555
                printf("Recu de %s:%d : %s\n",inet_ntoa(coordonneesAppelant.sin_addr),ntohs(coordonneesAppelant.sin_port),tampon);
                if (nbRecu > 0) {
                    tampon[nbRecu] = 0;
                
                    if(strcmp(tampon,"555")==0){               
                        commencer(fdSocketCommunication);
                    }
                }
            }
            //exit(EXIT_SUCCESS);
        }
    }
    close(fdSocketCommunication);
    close(fdSocketAttente);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        wait(NULL);
    }
    printf("Fin du programme.\n");
}


void afficherInfo(infotousjoueurs* info){
  int i;
  for(i=0;i<10;i++){
      printf("%s,%s,%d\n",(*info).joueur[i].pseudo,(*info).joueur[i].word,(*info).joueur[i].score);    
  }
}

void commencer(int fdSocketCommunication){
   int nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0); // NO2 recv le pseudo du joueur
   if (nbRecu > 0) {
     printf("Recu du joueur : %s\n", tampon);
  	}
  	char pseudo[10];
  	strcpy(pseudo,tampon);
    
    /*----------------la mémoire partagée---------------------------*/
    key_t clefshm=ftok(".",19);
    int shmid=shmget(clefshm, sizeof(infotousjoueurs),0666|IPC_CREAT);
    if(shmid==-1){
      printf("Allocation de la shm en échec!\n");
      printf("%s\n", strerror(errno));
      exit(0);
    }
    infotousjoueurs* info;
    p(sema1);
    info=shmat(shmid,NULL,0);
    if(info==(void*)(-1)){
      printf("erreur de shm");
      exit(0);
    }
    else{
      strcpy((*info).joueur[nbClients-1].pseudo,pseudo); // enregistrer le pseudo dans la structure
    }
    shmdt(info);
    v(sema1);
            
    /*--------------------------------------------------------------*/

  	char mot[10][11]={0};
        char listemot[10][11];


   	ouvrirfichier(mot);
   	echanger(listemot,mot);
   	jouer(listemot,fdSocketCommunication);

}

void ouvrirfichier(char mot[10][11]){
	 FILE *fp; 
	char c; 
	int i=0; 
	int j=0; 
  int fichier;
				

	if((fp=fopen("test.txt","r"))==NULL) 
	{ 
		printf("erreur de ouverture\n"); 
		exit(0); 
	} 
	else
	{
		c=fgetc(fp); 
		
		while(c!=EOF) 
		{ 
			if(c!='\n') 
			{ 
				mot[i][j]=c; 
				j++; 
			} 
			else 
			{ 
				i++; 
				j=0; 
			} 
			c=fgetc(fp); 
		} 
	}
//////////////////////////////////////////////////

}

void echanger(char listemot[10][11], char mot[10][11]){
	int i;
	int j;

	for(i=0;i<10;i++){
		for(j=0;j<11;j++){
			listemot[i][j]=mot[i][j];
		}
	}
}

void jouer(char listemot[10][11],int fdSocketCommunication){
	char caracessai; //les caracteres que le joueur a saisi
	char jouer;		// la preference du joueur
	char motessai[11]; //enregistrer le mot que le joueur a saisi
	char caracfaux[11]; //enregistrer les caracteres faux que le joueur a saisi

  int nbRecv;

	int len;
	int j;
	int k;
	int f;
	int pp=1;
	int q=0;
	int x=0;
	int y=0;
	int t=0;
	int drapeau=0; //le drapeau pour vérifier si le joueur trouve le caractere correct
  int fois=10;
  int score=0;

	while(pp){
		int c=1;
		srand(time(NULL));
		q=rand()%9;
		len=strlen(listemot[q]); //compter le lougueur du mot
    printf("le mot numero %d est selectionne\n",q+1);

		for(f=0;f<len;f++){
			motessai[f]='-';
		}
		motessai[len]='\0';

		strcpy(tampon,motessai);
    printf("----****----\n");
    char word[11];
    strcpy(word,listemot[q]);
    printf("mot envoyé %s\n", listemot[q]);
    printf("Le joueur est en train d'engager avec le mot %s\n",word);
    

    /*----------------la mémoire partagée---------------------------*/
    key_t clefshm=ftok(".",19);
    int shmid=shmget(clefshm, sizeof(infojoueur),0666|IPC_CREAT);
    if(shmid==-1){
      printf("Allocation de la shm en échec!\n");
      printf("%s\n", strerror(errno));
      exit(0);
    }
    infotousjoueurs* info;
    p(sema1);
    info=shmat(shmid,NULL,0);
    if(info==(void*)(-1)){
      printf("erreur de shm");
      exit(0);
    }
    else{
      strcpy((*info).joueur[nbClients-1].word,word);// enregistrer le mot distribué dans la structure
    }
    shmdt(info);
    v(sema1);
            
    /*--------------------------------------------------------------*/

    for(k=1;k<=fois&&c!=0;k++){  //vérifier si le joueur finit deviner
      send(fdSocketCommunication, tampon, strlen(tampon), 0); // NO1 send le mot caché --- au joueur
      drapeau=0;
      int foisres=fois-k+1;
      char strfois[10];
      sprintf(strfois,"%d",foisres);
      strcpy(tampon, strfois);
      send(fdSocketCommunication, tampon, strlen(tampon), 0); // NO 2 send au joueur : printf("vous avez %d fois!\n",fois-k+1);
      //problème : le tampon est en confusion
      recv(fdSocketCommunication, tampon, MAX_BUFFER, 0); // NO3 recv le caractaire que le joueur saisit
      strcpy(&caracessai,tampon);
      printf("le joueur a saisi le lettre %s\n",&caracessai);

      for(j=0;j<len;j++){ //vérifier si le caracessai exist dans ce mot
        //if(motessai[j]=='-'){ //vérifier si le joueur a déjà saisi ce caractere
          if(listemot[q][j]==caracessai){ //si le caracessai exist dans ce mot
            motessai[j]=listemot[q][j]; //ramplacer "-" avec le caracessai
            score++;
            printf("stat de complete %s\n",motessai);
            x++;
            drapeau=1;
          }
        //}
        else if(motessai[j]==caracessai){
          /*strcpy(tampon,"mot répété");
          send(fdSocketCommunication, tampon, strlen(tampon), 0);*/
          break;
        }
      }

      /*----------------la mémoire partagée---------------------------*/
     
      p(sema1);
      info=shmat(shmid,NULL,0);// attacher la mémoire partagée
      (*info).joueur[nbClients-1].score=score; // enregister le score dans la structure
      afficherInfo(info);
      shmdt(info);// détacher la mémoire partagée
      v(sema1);
            
    /*--------------------------------------------------------------*/

      if(drapeau==0){ //si le caracessai n'exist pas dans le mot
        caracfaux[t]=caracessai;
        t++;
      }
      strcpy(tampon,motessai);
      send(fdSocketCommunication, tampon, strlen(tampon), 0);

      if(x>y){
        y=x;
      }
      else{
        k++;
      }
      c=strcmp(motessai,listemot[q]); //vérifier si le joueur trouve le bon mot
}
      
      if(c==0){ //si le joueur trouve le bon mot
        strcpy(tampon,"correct");
        send(fdSocketCommunication, tampon, strlen(tampon), 0);
        int score;
        score ++;
      }
      else if(c!=0){ //sinon
        strcpy(tampon,"perdu");
        send(fdSocketCommunication, tampon, strlen(tampon), 0);
      }
      caracfaux[t]='\0';
	}
}

void p(int semid) { 
  int rep; 
  struct sembuf sb={0,-1,0}; //num sema, valeur à ajouter = -1, pas d’option 
  rep=semop(semid, &sb, 1); //1 = un seul sémaphore concerné 
}

void v(int semid) { 
  int rep; 
  struct sembuf sb={0,1,0}; //num sema, valeur à ajouter = +1, pas d’option 
  rep=semop(semid, &sb, 1); //1 = un seul sémaphore concerné 
}
int main(int argc, char const *argv[]) {

    preparerServeur();

}