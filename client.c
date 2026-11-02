#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "segdef.h"
#define NB_SEMS 3
#define SHMAT_ERROR ((void *)-1)
/* récupère les identifiant du segment, des sémaphores et initialise le générateur.Renvoie EXIT_SUCCESS si tout va bien, EXIT_FAILURE sinon.*/
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


// initialiser le segment et calculer le résultat local

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
Renvoie le résultat calculé par le serveur, et met la moyenne locale dans *local_result. */ 
static long send_request(int semid, segment *seg, int req, long *local_result) 
{ 
    acq_sem(semid, seg_dispo); /* Attend que le segment soit libre */ 
    *local_result = fill_segment(seg, req); 
    acq_sem(semid, seg_init); /* Attend que le segment soit initialisé */ 
    wait_sem(semid, res_ok); /* Attend que le résultat soit prêt */ 
    long server_result = seg->result; /* Lit le résultat */ 
    lib_sem(semid, seg_init); /* Dit au serveur que le segment est lu */ 
    acq_sem(semid, res_ok); /* Attend que le serveur libère res_ok */ 
    lib_sem(semid, res_ok); /* Remet res_OK a sa valeur initiale */
    lib_sem(semid, seg_dispo); /* Rend le segment aux autres clients */ 
    return server_result; 
}



int main(void)
{
    int semid;
    segment *seg;

    if (init_client(&semid, &seg) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    printf("Initialisation OK : semid = %d, segment attache a %p\n",
           semid, (void *)seg);

    /* Test de fill_segment sur une variable locale */
    segment test;
    long local_result; 
    long server_result = send_request(semid, seg, 1, &local_result); 
    printf("client = %ld, serveur = %ld -> %s\n", local_result, server_result, local_result == server_result ? "OK" : "DIFFERENT");


    // Détacher le segment
    if (shmdt(seg) == -1) {
        perror("shmdt");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
