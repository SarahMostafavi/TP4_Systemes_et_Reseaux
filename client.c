#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "segdef.h"
#include <sys/wait.h>
#define NB_SEMS 3
#define SHMAT_ERROR ((void *)-1)
/* Nombre de requêtes envoyées au serveur */
#define NB_REQUESTS 10
/* Nombre de clients lancés en parallèle avec fork() */
#define NB_CLIENTS 5

struct sembuf sop;

/* Récupère les identifiants du segment et des sémaphores créés par le serveur,
   attache le segment et initialise le générateur.
   Renvoie EXIT_SUCCESS si tout va bien, EXIT_FAILURE sinon. */
static int init_client(int *semid, segment **seg)
{
    // Récupérer les sémaphores
    *semid = semget(cle, NB_SEMS, 0);
    if (*semid == -1) {
        perror("semget (le serveur est-il lancé ?)");
        return EXIT_FAILURE;
    }

    // Récupérer le segment
    int shmid = shmget(cle, segsize, 0);
    if (shmid == -1) {
        perror("shmget (le serveur est-il lancé ?)");
        return EXIT_FAILURE;
    }

    // Attacher le segment
    *seg = shmat(shmid, NULL, 0);
    if (*seg == SHMAT_ERROR) {
        perror("shmat");
        return EXIT_FAILURE;
    }

    // Initialiser le générateur
    init_rand();
    return EXIT_SUCCESS;
}

/* Initialise le segment (pid, numéro de requête, valeurs aléatoires)
   et renvoie la moyenne calculée localement. */
static long fill_segment(segment *seg, int req)
{
    long sum = 0;

    seg->pid = getpid();
    seg->req = req;
    for (int i = 0; i < maxval; i++) {
        seg->tab[i] = getrand();
        sum += seg->tab[i];
    }
    return sum / maxval;
}

/* Envoie une requête au serveur en suivant le protocole des sémaphores.
   Met la moyenne locale dans *local_result et le résultat du serveur
   dans *server_result.
   Renvoie 1 si la réponse correspond bien à notre pid et à notre requête. */
static int send_request(int semid, segment *seg, int req,
                        long *local_result, long *server_result)
{
    acq_sem(semid, seg_dispo);      /* Attend que le segment soit libre */
    *local_result = fill_segment(seg, req);
    acq_sem(semid, seg_init);       /* Signale au serveur : segment initialisé */
    wait_sem(semid, res_ok);        /* Attend que le résultat soit prêt */

    /* Lecture avant de rendre la main au serveur */
    *server_result = seg->result;
    int same_request = (seg->pid == getpid() && seg->req == req);

    lib_sem(semid, seg_init);       /* Signale au serveur : résultat lu */
    acq_sem(semid, res_ok);         /* Attend que le serveur libère res_ok */
    lib_sem(semid, res_ok);         /* Remet res_ok à sa valeur initiale */
    lib_sem(semid, seg_dispo);      /* Rend le segment aux autres clients */

    return same_request;
}
/* Travail d'un client*/
static int run_client(void)
{
    int semid;
    segment *seg;

    if (init_client(&semid, &seg) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    /* Boucle contrôlée par le compteur de requêtes */
    int nb_ok = 0;
    for (int req = 1; req <= NB_REQUESTS; req++) {
        long local_result, server_result;
        int same_request = send_request(semid, seg, req,
                                        &local_result, &server_result);

        int ok = same_request && local_result == server_result;
        nb_ok += ok;
        printf("[pid %d] requete %2d : client = %ld, serveur = %ld -> %s\n",
               getpid(), req, local_result, server_result,
               ok ? "OK" : (same_request ? "MOYENNE DIFFERENTE"
                                         : "MAUVAISE REQUETE"));
    }
    printf("[pid %d] %d/%d requetes correctes\n", getpid(), nb_ok, NB_REQUESTS);

    // Détacher le segment
    if (shmdt(seg) == -1) {
        perror("shmdt");
        return EXIT_FAILURE;
    }
    return (nb_ok == NB_REQUESTS) ? EXIT_SUCCESS : EXIT_FAILURE;
}
int main(void)
{
    /* Création des clients : chaque enfant exécute run_client */
    int nb_created = 0;
    for (int i = 0; i < NB_CLIENTS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            break;                
        }
        if (pid == 0)
            exit(run_client());    
        nb_created++;              
    }

    /* Le parent attend tous ses enfants*/
    int nb_clients_ok = 0;
    for (int i = 0; i < nb_created; i++) {
        int status;
        if (wait(&status) == -1) {
            perror("wait");
            return EXIT_FAILURE;
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
            nb_clients_ok++;
    }

    
    return (nb_clients_ok == NB_CLIENTS) ? EXIT_SUCCESS : EXIT_FAILURE;
}
