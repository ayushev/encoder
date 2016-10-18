#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
/* Lame C library header*/
#include "lame/lame.h"

#include "encoder.h"
/* Exceptions for C header */
#include "e4c.h"
/* OS dependent functions */
#include "os.h"
/* Music dependent functions */
#include "music.h"


typedef enum en_encoderFSM {
    en_efsm_invalid,
    en_efsm_prepData,
    en_efsm_makeThread,
    en_efsm_waitThread,
    en_efsm_end
} en_encoderFSM_t;

int main(int argc, char* argv[])
{
    char*           trgPath = NULL;
    pthread_t       threads[MAX_THREADS] = {0};
    st_encArgs_t    tArgs = {.p_fdesc = NULL,
                             .files = 0,
                             .p_dirPath = NULL};
    pthread_attr_t  attr;
    int             ret;
    int             i;
    uint8_t         activeThreads = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (argc < 2)
    {
        fprintf(stderr, "Error: Specify a directory with input files\n"
                "Usage: %s PATH", argv[0]);
        exit(-1);
    }

    /* It's recommended to store the argument value*/
    trgPath = strdup(argv[1]);
    if (trgPath == NULL)
    {
        fprintf(stderr, "Error: strdup failed \n");
        exit(-1);
    }

    os_fExplore(trgPath, &tArgs, MAX_THREADS);
    if (tArgs.files < 0)
    {
        fprintf(stderr, "Error: No valid files were found\n");
    }
    else
    {
        tArgs.p_dirPath = argv[1];
        /* Create several threads */
        for (i = 0; i < MAX_THREADS && i < tArgs.files; i++)
        {
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

        /* Free allocated memory */
        for (i = 0; i < tArgs.files; i++)
        {
            if ((tArgs.p_fdesc[i].p_fname) != NULL)
            {
                free(tArgs.p_fdesc[i].p_fname);
            }
        }
        free(tArgs.p_fdesc);
        free(trgPath);
    }

    pthread_attr_destroy(&attr);
    pthread_exit(NULL);

}
