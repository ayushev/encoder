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
    int16_t         dirSize = 0;
    int16_t         dirOff = 0;
    char*           dirPath = NULL;
    pthread_t       threads[MAX_THREADS] = {0};
    st_encArgs_t    tArgs[MAX_THREADS] = {{NULL}};
    pthread_attr_t  attr;
    en_encoderFSM_t encFSM = en_efsm_prepData;
    int             ret;
    int             i;
    /* Currently processing thread and ThreadArguments struct */
    uint16_t        curArgs = 0;
    uint16_t        curThread = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (argc < 2) {
        fprintf(stderr,"Error: Specify a directory with input files\n");
        exit(-1);
    }
    dirPath = strdup(argv[1]);
    while (encFSM != en_efsm_end) {
        switch (encFSM) {
        case en_efsm_prepData:
            dirSize = os_fExplore(dirPath, dirOff, tArgs, MAX_THREADS);
            if (dirSize) {
                curArgs = 0;
                curThread = 0;
                dirOff += dirSize;
                encFSM = en_efsm_makeThread;
            } else
                encFSM = en_efsm_end;
            break;
        case en_efsm_makeThread:
            printf("Converting-> [%s]\n", tArgs[curArgs].p_filename);
            tArgs[curArgs].p_dirPath = argv[1];
            ret = pthread_create(&threads[curThread], &attr, music_process,
                                 (void *)&tArgs[curArgs]);
            if (ret){
                fprintf(stderr," Error in pthread_create(), Code [%d]\n", ret);
                encFSM = en_efsm_end;
                break;
            }
            curThread++;
            curArgs++;
            if ((curThread == MAX_THREADS) || (curArgs == dirSize)) {
                encFSM = en_efsm_waitThread;
            }
            break;
        case en_efsm_waitThread:
            for(i = 0; i < curThread; i++)
            {
               ret = pthread_join(threads[i], NULL);
               if (ret) {
                   fprintf(stderr," ERROR; return code from pthread_join() is %d\n", ret);
                   encFSM = en_efsm_end;
                   break;
               }
            }
            encFSM = en_efsm_prepData;
            break;
        default:
            encFSM = en_efsm_end;
            break;
        }
    }

    pthread_attr_destroy(&attr);

    /* Free allocated memory */
    for (i = 0; i < dirSize; i++) {
        if ((tArgs[i].p_filename) != NULL) {
            free(tArgs[i].p_filename);
        }
    }

    pthread_exit(NULL);

}
