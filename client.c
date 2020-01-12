#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 6000
#define MAX_BUFFER 1000
#define MAX 1024

const char *EXIT = "exit";
char tampon[MAX_BUFFER];
char tampon2[MAX_BUFFER];


//void interfacedejeu(int fdSocket);
void commencer(int fdSocket); 
void eraser(char str[]);

void lireMessage(char tampon[]) {
    printf("Saisir :\n");
    fgets(tampon, MAX_BUFFER, stdin);
    strtok(tampon, "\n");
}

void preparerClient(){
    int fdSocket;
    int nbRecu;
    struct sockaddr_in coordonneesServeur;
    int longueurAdresse;
    
    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fdSocket < 0) {
        printf("socket incorrecte\n");
        exit(EXIT_FAILURE);
    }
    // On prépare les coordonnées du serveur
    longueurAdresse = sizeof(struct sockaddr_in);
    memset(&coordonneesServeur, 0x00, longueurAdresse);
    coordonneesServeur.sin_family = PF_INET;
    // adresse du serveur
    inet_aton("127.0.0.1", &coordonneesServeur.sin_addr);
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_port = htons(PORT);

    if (connect(fdSocket, (struct sockaddr *) &coordonneesServeur, sizeof(coordonneesServeur)) == -1) {
        printf("connexion impossible\n");
        exit(EXIT_FAILURE);
    }

    printf("connexion ok\n");
    printf("Bienvenu a ce jeu!\n");

    while (1) {
        printf("Saisir 555 à commencer le jeu\n");
        lireMessage(tampon);
        if(strcmp(tampon,"555")==0){
            commencer(fdSocket);
            break;
        }
    }
}

void commencer(int fdSocket){
    int fin=0;
    send(fdSocket, tampon, strlen(tampon), 0);// NO1 send 555 pour activer la fonction commencer coté serveur
    printf("Veuillez vous saisir votre pseudo\n");
    lireMessage(tampon);
    send(fdSocket, tampon, strlen(tampon), 0);// NO2 send envoyer le pseudo au serveur
    printf("attente du message du serveur...\n");
        //boucle//
    while(fin!=1){
        int nbRecu=recv(fdSocket, tampon, MAX_BUFFER, 0);// NO1 recv le mot caché "---" du serveur
        
        printf("le mot recu : %s\n", tampon); //  afficher le mot caché
        tampon[nbRecu] = 0;
        
        recv(fdSocket, tampon, MAX_BUFFER, 0);// NO2 recv la fois restant
        printf("vous avez encore %s fois!\n",tampon);
       
        //le tampon est en confusion
        
          
        printf("veuiller vous saisir un caractere\n");
        lireMessage(tampon);
        send(fdSocket, tampon, strlen(tampon), 0);// NO3 send envoyer un caractere au serveur
        
        //recv(fdSocket, tampon, MAX_BUFFER, 0);// NO3 recv le résultat
        if((strcmp(tampon,"correct"))==0||(strcmp(tampon,"perdu"))==0){
            fin=1;
            break;
        }
    } //fin boucle//
 }  
       


void eraser(char str[]){ // idéal c'est pour viter le tampon, mais il ne marche pas
    int k;
    for( k=0;k<10000;k++){
        memset(str,0,(MAX*MAX*sizeof(int)));
    }
}

int main(int argc, char const *argv[]) {
    preparerClient();
}