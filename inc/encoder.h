/*
 * encoder.h
 *
 *  Created on: Oct 14, 2016
 *      Author: yushev
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#define BLOCK_SIZE      1024
#define INBUF_SIZE      BLOCK_SIZE>>1
#define OUTBUF_SIZE     BLOCK_SIZE
#define MAX_FILEPATH    256
#define MAX_THREADS     150

typedef enum en_music
{
    en_music_invalid,
    en_music_wave
} en_music_t;

typedef struct st_encArgs
{
    char*       p_filename;
    char*       p_dirPath;
}st_encArgs_t;

typedef struct st_encoder
{
    FILE*           p_fp;
    uint8_t         opened;
	en_music_t      type;
	uint32_t        len;
	const char*     path;

	int             blkLen;
} st_encoder_t;

#endif /* ENCODER_H_ */
