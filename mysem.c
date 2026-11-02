#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "segdef.h"

/* Valeurs de sem_op pour semop */
#define SEM_ACQUIRE  -1   /* prend le sémaphore (attend s'il vaut 0) */
#define SEM_RELEASE   1   /* rend le sémaphore */
#define SEM_WAIT_ZERO 0   /* attend que le sémaphore vaille 0 */

extern struct sembuf sop;

/* Applique l'opération op au sémaphore sem de l'ensemble semid.
   semop bloque le processus jusqu'à ce que l'opération soit possible. */
static void sem_operation(int semid, int sem, int op)
{
    sop.sem_num = sem;
    sop.sem_op = op;
    sop.sem_flg = 0;

    if (semop(semid, &sop, 1) == -1) {
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

/* Attend que le sémaphore sem passe à 0 */
void wait_sem(int semid, int sem)
{
    sem_operation(semid, sem, SEM_WAIT_ZERO);
}

/* Acquiert le sémaphore sem */
void acq_sem(int semid, int sem)
{
    sem_operation(semid, sem, SEM_ACQUIRE);
}

/* Libère le sémaphore sem */
void lib_sem(int semid, int sem)
{
    sem_operation(semid, sem, SEM_RELEASE);
}

/* Initialise le générateur aléatoire */
void init_rand()
{
    srand(time(NULL) ^ getpid());
}

/* Retourne un entier long signé */
long getrand()
{
    return rand();
}
