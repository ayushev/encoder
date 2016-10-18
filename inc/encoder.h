/*
 * encoder.h
 *
 *  Created on: Oct 14, 2016
 *      Author: yushev
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#define BLOCK_SIZE      2048
#define INBUF_SIZE      BLOCK_SIZE
#define OUTBUF_SIZE     BLOCK_SIZE
#define MAX_FILEPATH    256
#define MAX_THREADS     2

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
    char*           p_dirPath;
}st_encArgs_t;

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

#endif /* ENCODER_H_ */
