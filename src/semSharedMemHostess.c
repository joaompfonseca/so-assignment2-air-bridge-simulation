/**
 *  \file semSharedMemHostess.c (implementation file)
 *
 *  \brief Problem name: Air Lift.
 *
 *  Synchronization based on semaphores and shared memory.
 *  Implementation with SVIPC.
 *
 *  Definition of the operations carried out by the hostess:
 *     \li waitForNextFlight
 *     \li waitForPassenger
 *     \li checkPassport
 *     \li signalReadyToFlight
 *
 *  \author Nuno Lau - January 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#include "probConst.h"
#include "probDataStruct.h"
#include "logging.h"
#include "sharedDataSync.h"
#include "semaphore.h"
#include "sharedMemory.h"

/** \brief logging file name */
static char nFic[51];

/** \brief shared memory block access identifier */
static int shmid;

/** \brief semaphore set access identifier */
static int semgid;

/** \brief pointer to shared memory region */
static SHARED_DATA *sh;

/** \brief hostess waits for next flight */
static void waitForNextFlight ();

/** \brief hostess waits for passenger */
static void waitForPassenger();

/** \brief hostess checks passport */
static bool checkPassport ();

/** \brief hostess signals boarding is complete */
static void signalReadyToFlight ();


/** \brief getter for number of passengers flying */
static int nPassengersInFlight ();

/** \brief getter for number of passengers waiting */
static int nPassengersInQueue ();

/**
 *  \brief Main program.
 *
 *  Its role is to generate the life cycle of one of intervening entities in the problem: the hostess.
 */

int main (int argc, char *argv[])
{
    int key;                                                           /*access key to shared memory and semaphore set */
    char *tinp;                                                                      /* numerical parameters test flag */

    /* validation of command line parameters */

    if (argc != 4) { 
        freopen ("error_HT", "a", stderr);
        fprintf (stderr, "Number of parameters is incorrect!\n");
        return EXIT_FAILURE;
    }
    else freopen (argv[3], "w", stderr);

    strcpy (nFic, argv[1]);
    key = (unsigned int) strtol (argv[2], &tinp, 0);
    if (*tinp != '\0')
    { fprintf (stderr, "Error on the access key communication!\n");
        return EXIT_FAILURE;
    }

    /* connection to the semaphore set and the shared memory region and mapping the shared region onto the
       process address space */

    if ((semgid = semConnect (key)) == -1) { 
        perror ("error on connecting to the semaphore set");
        return EXIT_FAILURE;
    }
    if ((shmid = shmemConnect (key)) == -1) { 
        perror ("error on connecting to the shared memory region");
        return EXIT_FAILURE;
    }
    if (shmemAttach (shmid, (void **) &sh) == -1) { 
        perror ("error on mapping the shared region on the process address space");
        return EXIT_FAILURE;
    }

    srandom ((unsigned int) getpid ());                                                 /* initialize random generator */

    /* simulation of the life cycle of the hostess */

    int nPassengers=0;
    bool lastPassengerInFlight;

    while(nPassengers < N ) {
        waitForNextFlight();
        do { 
            waitForPassenger();
            lastPassengerInFlight = checkPassport();
            nPassengers++;
        } while (!lastPassengerInFlight);
        signalReadyToFlight();
    }

    /* unmapping the shared region off the process address space */

    if (shmemDettach (sh) == -1) {
        perror ("error on unmapping the shared region off the process address space");
        return EXIT_FAILURE;;
    }

    return EXIT_SUCCESS;
}

/**
 *  \brief wait for Next Flight.
 *
 *  Hostess updates its state and waits for plane to be ready for boarding
 *  The internal state should be saved.
 *
 */

static void waitForNextFlight ()
{
    if (semDown (semgid, sh->mutex) == -1)  {                                                  /* enter critical region */
        perror ("error on the down operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    sh->fSt.st.hostessStat = WAIT_FOR_FLIGHT; // Alterar estado da hospedeira
    saveState(nFic, &(sh->fSt));              // Guardar estados
    
    if (semUp (semgid, sh->mutex) == -1)                                                       /* exit critical region */
    { 
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    semDown(semgid, sh->readyForBoarding); // Esperar autorização do piloto para começar o embarque
}

/**
 *  \brief hostess waits for passenger
 *
 *  hostess waits for passengers to arrive at airport.
 *  The internal state should be saved.
 */

static void waitForPassenger ()
{
    if (semDown (semgid, sh->mutex) == -1)                                                      /* enter critical region */
    { 
        perror ("error on the down operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    sh->fSt.st.hostessStat = WAIT_FOR_PASSENGER; // Alterar estado da hospedeira
    saveState(nFic, &(sh->fSt));                 // Guardar estados

    if (semUp (semgid, sh->mutex) == -1) {                                                  /* exit critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    semDown(semgid, sh->passengersInQueue); // Esperar pelo próximo passageiro
}

/**
 *  \brief passport check
 *
 *  The hostess checks passenger passport and waits for passenger to show id
 *  The internal state should be saved twice.
 *
 *  \return should be true if this is the last passenger for this flight
 *    that is: 
 *      - flight is at its maximum capacity 
 *      - flight is at or higher than minimum capacity and no passenger waiting 
 *      - no more passengers
 */

static bool checkPassport()
{
    bool last;

    /* insert your code here */

    if (semDown (semgid, sh->mutex) == -1) {                                                     /* enter critical region */
        perror ("error on the down operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    sh->fSt.st.hostessStat = CHECK_PASSPORT; // Alterar estado da hospedeira
    saveState(nFic, &(sh->fSt));             // Guardar estados

    if (semUp (semgid, sh->mutex) == -1)     {                                                 /* exit critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    semUp(semgid, sh->passengersWaitInQueue); // Autorizar passageiro a sair da fila
    semDown(semgid, sh->idShown);             // Esperar que o passageiro mostre o ID

    if (semDown (semgid, sh->mutex) == -1)  {                                                 /* enter critical region */
        perror ("error on the down operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    sh->fSt.nPassInQueue--;     // Decrementar nr de passageiros na fila
    sh->fSt.nPassInFlight++;    // Incrementar nr de passageiros no voo
    sh->fSt.totalPassBoarded++; // Incrementar nr de passageiros totais que já embarcaram

    last = nPassengersInFlight() == MAXFC 
        || (nPassengersInFlight() >= MINFC && nPassengersInQueue() == 0)
        || sh->fSt.totalPassBoarded == N; // Determinar se é o último passageiro no voo

    savePassengerChecked(nFic, &(sh->fSt)); // Identificar o passageiro que embarcou
    saveState(nFic, &(sh->fSt));            // Guardar estados

    if (semUp (semgid, sh->mutex) == -1) {                                                     /* exit critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */

    return last;
}

static int nPassengersInFlight()
{
    return sh->fSt.nPassInFlight;
}

static int nPassengersInQueue()
{
    return sh->fSt.nPassInQueue;
}

/**
 *  \brief signal ready to flight 
 *
 *  The flight is ready to go.
 *  The hostess updates her state, registers the number of passengers in this flight 
 *  and checks if the airlift is finished (all passengers have boarded).
 *  Hostess informs pilot that plane is ready to flight.
 *  The internal state should be saved.
 */
void signalReadyToFlight()
{
    if (semDown (semgid, sh->mutex) == -1) {                                                /* enter critical region */
        perror ("error on the down operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    sh->fSt.st.hostessStat = READY_TO_FLIGHT;                               // Alterar estado da hospedeira
    sh->fSt.nPassengersInFlight[sh->fSt.nFlight-1] = sh->fSt.nPassInFlight; // Guardar nr de passageiros no voo
    sh->fSt.finished = sh->fSt.totalPassBoarded == N;                       // Determinar se todos os passageiros já embarcaram
    saveState(nFic, &(sh->fSt));                                            // Guardar estados
    saveFlightDeparted(nFic, &(sh->fSt));                                   // Indicar o começo do voo

    if (semUp (semgid, sh->mutex) == -1) {                                                     /* exit critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    semUp(semgid, sh->readyToFlight); // Autorizar piloto a descolar
}


