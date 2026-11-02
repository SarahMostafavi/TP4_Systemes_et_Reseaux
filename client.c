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
int main(void)
{
    int semid;
    segment *seg;

    if (init_client(&semid, &seg) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    printf("Initialisation OK : semid = %d, segment attache a %p\n",
           semid, (void *)seg);

    // Détacher le segment
    if (shmdt(seg) == -1) {
        perror("shmdt");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
