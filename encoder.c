/*
 * --- Module Description --------------------------------------------------- *
 */
/**
 * \file    encoder.c
 * \author  Artem Yushev
 * \date    $Date$
 * \version $Version$
 *
 * \brief   Main routines for threads scheduling and controlling
 */

/*
 * --- Includes ------------------------------------------------------------- *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <getopt.h>
#include "encoder.h"
/* Exceptions for C header */
#include "e4c.h"
/* OS dependent functions */
#include "os.h"
/* Music dependent functions */
#include "music.h"
/*
 * --- Macro Definitions ---------------------------------------------------- *
 */


/*
 * --- Type Definitions ----------------------------------------------------- *
 */
typedef enum en_encoderFSM {
    en_efsm_invalid,
    en_efsm_prepData,
    en_efsm_makeThread,
    en_efsm_waitThread,
    en_efsm_end
} en_encoderFSM_t;

/*
 * --- Variables ------------------------------------------------------------ *
 * /


/*
 * --- Local Functions Declaration ------------------------------------------ *
 */

int main(int argc, char* argv[])
{
    pthread_t       threads[MAX_THREADS] = {0};
    st_encArg_t    tArgs = {.p_fdesc = NULL,
                             .files = 0,
                             .p_trgPath = NULL,
                             .threadID = 0};
    pthread_attr_t  attr;
    int             ret;
    int             i;
    uint8_t         activeThreads = 0;
    /* Let a user to define maxThreads value*/
    uint16_t        maxThreads = MAX_THREADS;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (argc < 2)
    {
        fprintf(stderr, "Error: Specify a directory with input files\n"
                "Usage: %s [-th] PATH \n"
                "Options:\n"
                "        -t  N  Specifies how much threads the application should use \n"
                "        -h     This help\n", argv[0]);
        exit(-1);
    }

    while (optind < argc)
    {
        if ((i = getopt(argc, argv, "ht:")) != -1) {
            switch (i) {
                case 'h':
                    fprintf(stderr, "Sound encoder usage:\n"
                                    "-t    Specifies how much threads the application should use \n"
                                    "-h    This help\n", optopt);
                    exit(0);
                    break;
                case 't':
                    if (strtol(optarg, NULL, 5) > MAX_THREADS) {
                        fprintf(stderr, "Threads limit is %lu, selecting maximum\n", MAX_THREADS);
                    } else {
                        maxThreads = strtol(optarg, NULL, 5);
                    }
                    break;
                default:
                    abort();
            }
        } else {
            tArgs.p_trgPath = strdup(argv[optind]);
            optind++;
        }
    }

    /* It's recommended to store the argument value*/

    if (tArgs.p_trgPath == NULL)
    {
        fprintf(stderr, "Error: strdup failed \n");
        exit(-1);
    }

    os_fExplore(&tArgs);
    if (tArgs.files < 0)
    {
        fprintf(stderr, "Error: No valid files were found\n");
    }
    else
    {
        /* Create several threads */
        for (i = 0; i < maxThreads && i < tArgs.files; i++)
        {
            tArgs.threadID = i;
            ret = pthread_create(&threads[i], &attr, music_procFiles,
                    (void *) &tArgs);
            if (ret)
            {
                fprintf(stderr, " Error in pthread_create(), Code [%d]\n", ret);
                break;
            }
            activeThreads += 1;
        }

        /* Wait until any thread exist */
        for (i = 0; i < activeThreads; i++)
        {
            ret = pthread_join(threads[i], NULL);
            if (ret)
            {
                fprintf(stderr,
                        " ERROR; return code from pthread_join() is %d\n", ret);
                break;
            }
        }

        printf("Finished: %lu files processed\n",tArgs.files);

        /* Free allocated memory */
        for (i = 0; i < tArgs.files; i++)
        {
            if ((tArgs.p_fdesc[i].p_fname) != NULL)
            {
                free(tArgs.p_fdesc[i].p_fname);
            }
        }
        free(tArgs.p_fdesc);
        free(tArgs.p_trgPath);
    }

    pthread_attr_destroy(&attr);
    pthread_exit(NULL);

}

/*
 * --- Global Functions Definition ------------------------------------------ *
 */
