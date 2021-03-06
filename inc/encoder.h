/*
 * --- Module Description --------------------------------------------------- *
 */

/**
 * \file    encoder.h
 * \author  Artem Yushev
 * \date    $Date$
 * \version $Version$
 *
 * \brief   Main routines for threads scheduling and controlling
 */

#ifndef ENCODER_H_
#define ENCODER_H_

/*
 * --- Macro Definitions ---------------------------------------------------- *
 */

#define BLOCK_SIZE      2048
#define INBUF_SIZE      BLOCK_SIZE
#define OUTBUF_SIZE     BLOCK_SIZE
#define MAX_FILEPATH    256
#define MAX_THREADS     20

/*
 * --- Type Definitions ----------------------------------------------------- *
 */

typedef enum en_music
{
    en_music_invalid,
    en_music_wave
} en_music_t;

typedef struct st_encFDesc
{
    char*      p_fname;
    uint8_t    flocked;
}st_encFDesc_t;

typedef struct st_encArgs
{
    st_encFDesc_t*  p_fdesc;
    int32_t         files;
    char*           p_trgPath;
    uint16_t        threadID;
}st_encArg_t;

typedef struct st_encoder
{
    /* File struct pointer to opened file, otherwise NULL */
    FILE*           p_fp;
    /* Absolute path to the file */
    const char*     path;
    /* The overall length of the file */
    uint32_t        fsize;
    /* Shows whether file is still opened */
    uint8_t         opened;

    /* Format of the file */
    en_music_t      fmt;
    /* File encoded as float or integer */
    uint8_t         isFloat;
    /* Bit per sample */
    uint8_t			bps;
} st_encoder_t;

/*
 * --- Variables ------------------------------------------------------------ *
 */

#endif /* ENCODER_H_ */
