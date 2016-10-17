/*
 * music.c
 *
 *  Created on: Oct 14, 2016
 *      Author: yushev
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "lame/lame.h"
#include "encoder.h"
#include "os.h"
#include "e4c.h"

#define   MAX_UINT32                    0xFFFFFFFF
#define   MUSIC_IN                      1
#define   MUSIC_OUT                     0
/* PCM = 1 (i.e. Linear quantization)
 * Values other than 1 indicate some form of compression. */
#define   WAVE_FORMAT_PCM               0x0001
#define   WAVE_FORMAT_EXTENSIBLE        -2
#define   WAVE_FORMAT_IEEE_FLOAT        0x0003
/* Contains the letters "RIFF" in ASCII (0x52494646 big-endian form). */
#define   WAVE_ID_RIFF                  0x52494646
/* Contains the letters "WAVE" in ASCII (0x57415645 big-endian form). */
#define   WAVE_ID_WAVE                  0x57415645
/* Contains the letters "FMT" in ASCII (0x666d7420 big-endian form). */
#define   WAVE_ID_FMT                   0x666d7420
/* Contains the letters "DATA" in ASCII (0x64617461 big-endian form). */
#define   WAVE_ID_DATA                  0x64617461


typedef enum en_musicFSM {
    en_mfsm_invalid,
    en_mfsm_akkudata,
    en_mfsm_encode,
    en_mfsm_flush,
    en_mfsm_exit
} en_musicFSM_t;

static int8_t __wavePrepare(lame_t p_lame, st_encoder_t* p_enc);
static int8_t __musicPrepare(lame_t p_lame, st_encoder_t* p_enc);
static int8_t __encPrepare(uint8_t inout, st_encoder_t * enc, const char* path);
static void __swapBytes(uint8_t* p_in, uint16_t inSize,
                        int32_t* p_outL, int32_t* p_outR, uint16_t maxOut,
                        uint8_t bps);

static int8_t __encPrepare(uint8_t inout, st_encoder_t* p_enc, const char* path)
{
    assert(p_enc != NULL);
    assert(path != NULL);

    int8_t err = 0;

    if (p_enc != NULL) {
        p_enc->type = en_music_invalid;
        p_enc->len = 0;
        p_enc->path = path;
        p_enc->blkLen = 0;
        p_enc->blkLen = 0;
        p_enc->p_fp = NULL;
        p_enc->opened = 0;

        /* Open the given files */
        if (os_fOpen(inout, p_enc) < 0) {
            fprintf(stderr, "Failed to open an input file.\n");
            err = -1;
        }
    }
    return (err);
}

/* Good illustration for a format
* http://soundfile.sapp.org/doc/WaveFormat/
* https://msdn.microsoft.com/en-us/library/windows/hardware/ff536383(v=vs.85).aspx*/
static int8_t __wavePrepare(lame_t p_lame, st_encoder_t* p_enc)
{
    assert(p_enc != NULL);
    assert(p_enc->p_fp != NULL);
    assert(p_lame != NULL);

    int8_t      err = 0;
    int32_t     i32 = 0;
    /* Mono by default */
    int16_t     numChannels = 1;
    /* 44100 by default */
    int32_t     sampleRate = 44100;
    /* 8 by default */
    int32_t     bitsPerSample = 8;
    int16_t     audioFmt = 0;
    int32_t     chunkID = 0;
    int32_t     chunkSize = 0;
    int32_t     subChunkSize = 0;
    uint32_t    dataLength = 0;
    FILE*       p_fp = p_enc->p_fp;

    E4C_TRY {
        /* ChunkSize */
        chunkSize = os_read32le(p_fp);
        /* Format */
        i32 = os_read32be(p_fp);
        if (i32 != WAVE_ID_WAVE) {
            E4C_THROW(RuntimeException, "Not a WAVE audio format");
        }
        chunkSize -= 4;
        while (chunkSize > 0 && (dataLength == 0)) {
            /* SubchunkID */
            chunkID = os_read32be(p_fp);
            /* SubchunkSize */
            subChunkSize = os_read32le(p_fp);
            chunkSize -= subChunkSize;
            switch (chunkID) {
                case WAVE_ID_FMT:
                    /* AudioFormat */
                    audioFmt = os_read16le(p_fp);     subChunkSize -= 2;
                    /* NumChannels */
                    numChannels = os_read16le(p_fp);  subChunkSize -= 2;
                    /* SampleRate */
                    sampleRate = os_read32le(p_fp);   subChunkSize -= 4;
                    /* ByteRate */
                    os_read32le(p_fp);                subChunkSize -= 4;
                    /* BlockAlign */
                    os_read16le(p_fp);                subChunkSize -= 2;
                    /* BitPerSample */
                    bitsPerSample = os_read16le(p_fp);subChunkSize -= 2;

                    /* WAVE_FORMAT_EXTENSIBLE support */
                    if ((subChunkSize > 9) && (audioFmt == WAVE_FORMAT_EXTENSIBLE)) {
                        /* cbSize */
                        os_read16le(p_fp);            subChunkSize -= 2;
                        /* ValidBitsPerSample */
                        bitsPerSample = os_read16le(p_fp); subChunkSize -= 2;
                        /* dwChannelMask */
                        i32 = os_read32le(p_fp);      subChunkSize -= 4;
                        /* SubFormat */
                        audioFmt = os_read16le(p_fp); subChunkSize -= 2;
                    }

                    if ((audioFmt != WAVE_FORMAT_PCM) &&
                        ((audioFmt != WAVE_FORMAT_IEEE_FLOAT))){
                        E4C_THROW(RuntimeException, "Non PCM file format is't supported");
                    }

                    if (os_fSkip(p_fp, (long) subChunkSize) < 0) {
                        E4C_THROW(RuntimeException, "Failed to skip data");
                    }
                    break;
                case WAVE_ID_DATA:
                    dataLength = subChunkSize;
                    break;
                default:
                    if (os_fSkip(p_fp, (long) subChunkSize) < 0) {
                        E4C_THROW(RuntimeException, "Failed to skip data");
                    }
                    break;
                }
        }

        if (dataLength) {
            if (lame_set_num_channels(p_lame, numChannels) < 0) {
                E4C_THROW(RuntimeException, "Failed to setup WAVE numChannels,"
                                            "LAME supports up to 2");
            }
            if (lame_set_in_samplerate(p_lame, sampleRate) < 0) {
                E4C_THROW(RuntimeException, "Failed to setup WAVE sampleRate");
            }
            /* DataLength == NumSamples * NumChannels * BitsPerSample/8 */
            /* Make bytes from bits */
            i32 = (bitsPerSample + 7) >> 3;
            i32 *= numChannels;
            /* Number of samples =  DataLength/(NumChannels * BytesPerSample) */
            i32 = dataLength/i32;
            lame_set_num_samples(p_lame, i32);
            p_enc->bps = bitsPerSample;
        }
    }
    E4C_CATCH(RuntimeException){
        const e4c_exception * e = e4c_get_exception();
        fprintf(stderr, "Error: %s (%s).", e->name, e->message);
        err = -1;
    }

    return (err);
}

/* Currently supports only WAVE headers */
static int8_t __musicPrepare(lame_t p_lame, st_encoder_t* p_enc)
{
    assert(p_enc != NULL);
    assert(p_enc->p_fp != NULL);
    assert(p_lame != NULL);

    int32_t    i32 = 0;
    int8_t     err = 0;

    /* In case of WAVE file it's a RIFF header*/
    i32 = os_read32be(p_enc->p_fp);
    if (i32 == WAVE_ID_RIFF) {
        p_enc->type = en_music_wave;
        __wavePrepare(p_lame, p_enc);
    } else {
        os_fSeek(p_enc->p_fp, -4);
        fprintf(stderr, "File [%*.*s]: %s", 1, 80, p_enc->path, "Format not supported");
        err = -1;
    }

    return (err);
}

static void __swapBytes(uint8_t* p_in, uint16_t inSize,
                        int32_t* p_outL, int32_t* p_outR, uint16_t maxOut,
                        uint8_t bps)
{
	assert(p_in != NULL);
	assert(p_outL != NULL);

	/* Input pointer */
	uint8_t*    p_ip = (uint8_t *) p_in;
	int i = 0;
	switch (bps) {
	case 1:
	    while (1) {
	        if (((i+bps) < inSize) && (p_outL != NULL)) {
	            /* From unsigned integer */
                *p_outL = (p_ip[i] ^ 0x80) << 24;
                p_outL++;
                i++;
                if (((i+bps) < inSize) && (p_outR != NULL)) {
                    *p_outR = (p_ip[i] ^ 0x80) << 24;
                    p_outR++;
                    i+=bps;
                }
            } else {
                break;
            }
	    }
    break;
	case 2:
        while (1) {
            if (((i+bps) < inSize) && (p_outL != NULL)) {
                *p_outL = p_ip[i+1] << 24 | p_ip[i] << 16;
                p_outL++;
                i+=bps;
                if (((i+bps) < inSize) && (p_outR != NULL)) {
                    *p_outR = p_ip[i+1] << 24 | p_ip[i] << 16;
                    p_outR++;
                    i+=bps;
                }
            } else {
                break;
            }
        }
	  break;
	case 3:
        while (1) {
            if (((i+bps) < inSize) && (p_outL != NULL)) {
                *p_outL = p_ip[i+2] << 24 | p_ip[i+1] << 16 | p_ip[i] << 8;
                p_outL++;
                i+=bps;
                if (((i+bps) < inSize) && (p_outR != NULL)) {
                    *p_outR = p_ip[i+2] << 24 | p_ip[i+1] << 16 | p_ip[i] << 8;
                    p_outR++;
                    i+=bps;
                }
            } else {
                break;
            }
        }
        break;
	case 4:
        while (1) {
            if (((i+bps) < inSize) && (p_outL != NULL)) {
                *p_outL = p_ip[i+3] << 24 | p_ip[i+2] << 16 | p_ip[i+1] << 8 | p_ip[i];
                p_outL++;
                i+=bps;
                if (((i+bps) < inSize) && (p_outR != NULL)) {
                    *p_outR = p_ip[i+3] << 24 | p_ip[i+2] << 16 | p_ip[i+1] << 8 | p_ip[i];
                    p_outR++;
                    i+=bps;
                }
            } else {
                break;
            }
        }
	  break;
	}
}

void* music_process(void *threadarg)
{
    assert(threadarg != NULL);

    lame_t              p_lame = NULL;
    st_encoder_t        inFile;
    /* We create a separate buffer for each channel */
    int32_t    			p_channels[2][INBUF_SIZE];
    uint8_t             p_inBuf[INBUF_SIZE];
    st_encoder_t        outFile;
    /* LAME requires buffer of unsigned char as output */
    uint8_t             p_outBuf[OUTBUF_SIZE];
    en_musicFSM_t       encFSM = en_mfsm_akkudata;
    int                 numSamples = 0;
    st_encArgs_t*       encArgs = (st_encArgs_t*) threadarg;
    char                p_absPath[MAX_FILEPATH] = {'\0'};

    /* Begin the exception context for a single thread */
    e4c_context_begin(E4C_TRUE);

    E4C_TRY{
        p_lame = lame_init();
        if (!p_lame) {
            E4C_THROW(RuntimeException,"LAME initialization failed. Exit.");
        }

        /* Prepare full file path from filename and common directory */
        if ((encArgs->p_filename == NULL) || (encArgs->p_dirPath == NULL)) {
            E4C_THROW(ProgramSignalException, "Filename empty. Exit.");
        }
        strncat(p_absPath,encArgs->p_dirPath,MAX_FILEPATH);
        strncat(p_absPath,encArgs->p_filename,MAX_FILEPATH - strlen(encArgs->p_dirPath));

        /* Initialize encoder structure with all relevant values */
        if (__encPrepare(MUSIC_IN, &inFile, p_absPath) < 0) {
            E4C_THROW(ProgramSignalException, "Encoder struct initialization failed. Exit.");
        }
        memset(p_channels[0], 0, INBUF_SIZE);
        memset(p_channels[1], 0, INBUF_SIZE);
        memset(p_inBuf, 0, INBUF_SIZE);

        if (__musicPrepare(p_lame, &inFile) < 0) {
            E4C_THROW(ProgramSignalException, "Failed to parse a header for input. Exit.");
        }

        lame_set_VBR(p_lame, vbr_default);
        /* https://sourceforge.net/p/lame/mailman/message/18557283/
         * before calling lame_init_param, disable automatic ID3 tag writing: */
        lame_set_write_id3tag_automatic(p_lame, 0);
        if (lame_init_params(p_lame) < 0) {
            E4C_THROW(RuntimeException, "Failed to init LAME parameters. Exit.\n");
        }

        if (__encPrepare(MUSIC_OUT, &outFile, p_absPath) < 0) {
            E4C_THROW(RuntimeException, "Encoder struct initialization failed. Exit.");
        }
        memset(p_outBuf, 0, OUTBUF_SIZE);

        uint8_t numCh = lame_get_num_channels(p_lame);
        uint8_t bytesPerSample = (inFile.bps + 7) >> 3;
        do {
            switch (encFSM)
            {
                case en_mfsm_akkudata:

                    /* We read fixed portion of samples from file to
                     * uin8_t buffer and try to  rearrange them in a channel
                     * buffer of uint32_t so that LAME can
                     * understand those files
                     * Example:
                     * 1) p_channels --> L[00:00:00:00]R[00:00:00:00]
                     * 2) p_inBuf -->     [11:22:33:44:55:66:77:88]
                     * 3) bytesPerSample = 4
                     * 4) __swapBytes()
                     * 5) p_channels --> L[44:33:22:11]R[88:77:66:55]
                     * */
                    inFile.blkLen = fread_unlocked(p_inBuf, bytesPerSample, INBUF_SIZE/bytesPerSample, inFile.p_fp);
                    __swapBytes(p_inBuf,bytesPerSample * inFile.blkLen ,
                            p_channels[0], numCh==2?p_channels[1]:NULL, INBUF_SIZE,
                            bytesPerSample);

                    if (inFile.blkLen == 0) {
                        encFSM = en_mfsm_flush;
                    } else {
                        encFSM = en_mfsm_encode;
                    }
                break;

                case en_mfsm_encode:
                {
                    numSamples = inFile.blkLen/numCh;

                    if (numCh == 2) {
                    	outFile.blkLen = lame_encode_buffer_int(p_lame, p_channels[0],
                    	                                    p_channels[1],
															numSamples,
                    										p_outBuf, OUTBUF_SIZE);
                    } else {
                    	outFile.blkLen = lame_encode_buffer_int(p_lame, p_channels[0],
                    										NULL, numSamples,
															p_outBuf, OUTBUF_SIZE);
                    }
                    if (outFile.blkLen < 0) {
                        E4C_THROW(RuntimeException, "Failed to encode file. Exit.");
                    }

                    fwrite_unlocked(p_outBuf, 1, outFile.blkLen, outFile.p_fp);
                    //fflush_unlocked(outFile.p_fp);

                    encFSM = en_mfsm_akkudata;
                }
                break;

                case en_mfsm_flush:
                    outFile.blkLen = lame_encode_flush(p_lame, p_outBuf, OUTBUF_SIZE);
                    fwrite_unlocked(p_outBuf, outFile.blkLen, 1, outFile.p_fp);
                    encFSM = en_mfsm_exit;
                break;

                case en_mfsm_invalid:
                default:
                    encFSM = en_mfsm_exit;
                break;
            }
        }while (encFSM != en_mfsm_exit);
    }
    E4C_CATCH (RuntimeException) {
        const e4c_exception * e = e4c_get_exception();
        fprintf(stderr, "%s (%s).", e->name, e->message);
    }
    E4C_CATCH (ProgramSignalException) {
        const e4c_exception * e = e4c_get_exception();
        fprintf(stderr, "%s (%s).", e->name, e->message);
    }

    /* Leave exception context */
    e4c_context_end();

    lame_close(p_lame);
    if (inFile.opened)
        fclose(inFile.p_fp);
    if (outFile.opened)
        fclose(outFile.p_fp);

    pthread_exit(NULL);
}
