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
#include "lame.h"
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

typedef enum en_musicFSM
{
    en_mfsm_invalid,
    en_mfsm_akkudata,
    en_mfsm_encode,
    en_mfsm_flush,
    en_mfsm_exit
} en_musicFSM_t;

static pthread_mutex_t music_mutex = PTHREAD_MUTEX_INITIALIZER;

static int8_t __wavePrepare(lame_t p_lame, st_encoder_t* p_enc);
static int8_t __musicPrepare(lame_t p_lame, st_encoder_t* p_enc);
static int8_t __encPrepare(uint8_t inout, st_encoder_t * enc, const char* path);
static void __flopBytes(uint8_t* p_in, uint16_t inSize, int32_t* p_outL,
        int32_t* p_outR, uint16_t maxOut, uint8_t bps);

static int8_t __encPrepare(uint8_t inout, st_encoder_t* p_enc, const char* path)
{
    assert(p_enc != NULL);
    assert(path != NULL);

    int8_t err = 0;

    if (p_enc != NULL)
    {
        p_enc->fmt = en_music_invalid;
        p_enc->isFloat = en_music_invalid;
        p_enc->fsize = 0;
        p_enc->path = path;
        p_enc->p_fp = NULL;
        p_enc->opened = 0;

        /* Open the given files */
        if (os_fOpen(inout, p_enc) < 0)
        {
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
    int32_t         i32 = 0;
    /* Mono by default */
    int16_t         numChannels = 1;
    /* 44100 by default */
    int32_t         sampleRate = 44100;
    /* 8 by default */
    int32_t         bitsPerSample = 8;
    int16_t         audioFmt = 0;
    int32_t         chunkID = 0;
    int32_t         chunkSize = 0;
    int32_t         subChunkSize = 0;
    uint32_t        dataLength = 0;
    FILE*           p_fp = p_enc->p_fp;

    E4C_TRY{
    /* ChunkSize */
    chunkSize = os_read32le(p_fp);
    /* Format */
    i32 = os_read32be(p_fp);
    if (i32 != WAVE_ID_WAVE)
    {
        E4C_THROW(RuntimeException, "Not a WAVE audio format");
    }
    chunkSize -= 4;
    while (chunkSize > 0 && (dataLength == 0))
    {
        /* SubchunkID */
        chunkID = os_read32be(p_fp);
        /* SubchunkSize */
        subChunkSize = os_read32le(p_fp);
        chunkSize -= subChunkSize;
        switch (chunkID)
        {
            case WAVE_ID_FMT:
            /* AudioFormat */
            audioFmt = os_read16le(p_fp); subChunkSize -= 2;
            /* NumChannels */
            numChannels = os_read16le(p_fp); subChunkSize -= 2;
            /* SampleRate */
            sampleRate = os_read32le(p_fp); subChunkSize -= 4;
            /* ByteRate */
            os_read32le(p_fp); subChunkSize -= 4;
            /* BlockAlign */
            os_read16le(p_fp); subChunkSize -= 2;
            /* BitPerSample */
            bitsPerSample = os_read16le(p_fp);subChunkSize -= 2;

            /* WAVE_FORMAT_EXTENSIBLE support */
            if ((subChunkSize > 9) && (audioFmt == WAVE_FORMAT_EXTENSIBLE))
            {
                /* cbSize */
                os_read16le(p_fp); subChunkSize -= 2;
                /* ValidBitsPerSample */
                bitsPerSample = os_read16le(p_fp); subChunkSize -= 2;
                /* dwChannelMask */
                i32 = os_read32le(p_fp); subChunkSize -= 4;
                /* SubFormat */
                audioFmt = os_read16le(p_fp); subChunkSize -= 2;
                p_enc->isFloat = audioFmt;
            }

            if ((audioFmt != WAVE_FORMAT_PCM) &&
                    ((audioFmt != WAVE_FORMAT_IEEE_FLOAT)))
            {
                E4C_THROW(RuntimeException, "Non PCM file format is't supported");
            }

            if (os_fOffset(p_fp, (long) subChunkSize) < 0)
            {
                E4C_THROW(RuntimeException, "Failed to skip data");
            }
            break;
            case WAVE_ID_DATA:
            dataLength = subChunkSize;
            break;
            default:
            if (os_fOffset(p_fp, (long) subChunkSize) < 0)
            {
                E4C_THROW(RuntimeException, "Failed to skip data");
            }
            break;
        }
    }

    if (dataLength)
    {
        if (lame_set_num_channels(p_lame, numChannels) < 0)
        {
            E4C_THROW(RuntimeException, "Failed to setup WAVE numChannels,"
                    "LAME supports up to 2");
        }
        if (lame_set_in_samplerate(p_lame, sampleRate) < 0)
        {
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
E4C_CATCH(RuntimeException)
{
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

    int32_t i32 = 0;
    int8_t err = 0;

    /* In case of WAVE file it's a RIFF header*/
    i32 = os_read32be(p_enc->p_fp);
    if (i32 == WAVE_ID_RIFF)
    {
        p_enc->fmt = en_music_wave;
        __wavePrepare(p_lame, p_enc);
    }
    else
    {
        os_fOffset(p_enc->p_fp, -4);
        fprintf(stderr, "File [%*.*s]: %s", 1, 80, p_enc->path,
                "Format not supported");
        err = -1;
    }

    return (err);
}

static void __flopBytes(uint8_t* p_in, uint16_t inSize, int32_t* p_outL,
        int32_t* p_outR, uint16_t maxOut, uint8_t bps)
{
    assert(p_in != NULL);
    assert(p_outL != NULL);

    /* Input pointer */
    uint8_t* p_ip = (uint8_t *) p_in;

    switch (bps)
    {
    case 1:
        os_splitFlopUI8(p_ip, p_outL, p_outR, inSize);
        break;
    case 2:
        os_splitFlopI16(p_ip, p_outL, p_outR, inSize);
        break;
    case 3:
        os_splitFlopI24(p_ip, p_outL, p_outR, inSize);
        break;
    case 4:
        os_splitFlopI32(p_ip, p_outL, p_outR, inSize);
        break;
    }
}

int8_t music_procFile(char* p_dirPath, char* p_fname)
{
    /* Absolute path to the file because it's too expensive to
     * store absolute path for each file */
    char p_path[MAX_FILEPATH] = { '\0' };

    lame_t          p_lame = NULL;
    /* Structure to hold info about input file */
    st_encoder_t    inFile;
    /* We create a separate buffer for each channel */
    int32_t         p_channels[2][INBUF_SIZE];
    /* Number of channels, will be detected further */
    uint8_t         numChannels = 1;
    /* We read inFile into this buffer bytewise */
    uint8_t         p_inBuf[INBUF_SIZE];
    /* Bytes per sample, will be detected further */
    uint8_t         bytesPS = 0;

    /* Structure to hold info about output file */
    st_encoder_t    outFile =
    { 0 };
    /* LAME requires buffer of unsigned char as output */
    uint8_t         p_outBuf[OUTBUF_SIZE];

    /* We read data from a given file framewise,
     * so we need to store its size  */
    uint32_t        frameLen = 0;

    /* Finite State Machine to store state of data processing
     * entry -> en_mfsm_akkudata <->  en_mfsm_encode
     *                            ->  en_mfsm_flush  -> en_mfsm_exit -> exit*/
    en_musicFSM_t   encFSM = en_mfsm_akkudata;
    int32_t         numSamples = 0;
    int8_t          ret = 0;

    /* Begin the exception context for a single thread */
    e4c_context_begin(E4C_TRUE);

    E4C_TRY{
        p_lame = lame_init();
        if (!p_lame)
        {
            E4C_THROW(RuntimeException,"LAME initialization failed. Exit.");
        }

        /* Prepare full file path from filename and common directory */
        if ((p_fname == NULL) || (p_dirPath == NULL))
        {
            E4C_THROW(ProgramSignalException, "Filename empty. Exit.");
        }

        /* Make a relative path from two inputs */
        os_mkPath(p_path, p_dirPath, p_fname, MAX_FILEPATH);

        /* Initialize encoder structure with all relevant values */
        if (__encPrepare(MUSIC_IN, &inFile, p_path) < 0)
        {
            E4C_THROW(ProgramSignalException, "Encoder struct initialization failed. Exit.");
        }
        memset(p_channels[0], 0, INBUF_SIZE);
        memset(p_channels[1], 0, INBUF_SIZE);
        memset(p_inBuf, 0, INBUF_SIZE);

        if (__musicPrepare(p_lame, &inFile) < 0)
        {
            E4C_THROW(ProgramSignalException, "Failed to parse a header for input. Exit.");
        }

        lame_set_VBR(p_lame, vbr_default);
        /* https://sourceforge.net/p/lame/mailman/message/18557283/
         * before calling lame_init_param, disable automatic ID3 tag writing: */
        lame_set_write_id3tag_automatic(p_lame, 0);
        if (lame_init_params(p_lame) < 0)
        {
            E4C_THROW(RuntimeException, "Failed to init LAME parameters. Exit.\n");
        }

        if (__encPrepare(MUSIC_OUT, &outFile, p_path) < 0)
        {
            E4C_THROW(RuntimeException, "Encoder struct initialization failed. Exit.");
        }
        memset(p_outBuf, 0, OUTBUF_SIZE);

        numChannels = lame_get_num_channels(p_lame);
        bytesPS = (inFile.bps + 7) >> 3;
        do
        {
            switch (encFSM)
            {
                case en_mfsm_akkudata:
                {
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
                    frameLen = os_fread_unlocked(p_inBuf, bytesPS, INBUF_SIZE/bytesPS, inFile.p_fp);

                    if (frameLen == 0)
                        encFSM = en_mfsm_flush;
                    else
                        encFSM = en_mfsm_encode;
                    break;
                }
                case en_mfsm_encode:
                {
                    __flopBytes(p_inBuf,bytesPS * frameLen ,
                                p_channels[0], numChannels==2?p_channels[1]:NULL, INBUF_SIZE,
                                bytesPS);

                    numSamples = frameLen/numChannels;

                    if (numChannels == 2){
                        frameLen = lame_encode_buffer_int(p_lame, p_channels[0], p_channels[1], 
                                                          numSamples, p_outBuf, OUTBUF_SIZE);
                    }
                    else {
                        frameLen = lame_encode_buffer_int(p_lame, p_channels[0], NULL,
                                                          numSamples, p_outBuf, OUTBUF_SIZE);
                    }
                    if (frameLen < 0)
                    {
                        E4C_THROW(RuntimeException, "Failed to encode file. Exit.");
                    }

                    os_fwrite_unlocked(p_outBuf, 1, frameLen, outFile.p_fp);

                    encFSM = en_mfsm_akkudata;
                }
                break;

                case en_mfsm_flush:
                {
                    frameLen = lame_encode_flush(p_lame, p_outBuf, OUTBUF_SIZE);
                    os_fwrite_unlocked(p_outBuf, frameLen, 1, outFile.p_fp);
                    encFSM = en_mfsm_exit;
                }
                break;

                case en_mfsm_invalid:
                default:
                encFSM = en_mfsm_exit;
                break;
            }
        }while (encFSM != en_mfsm_exit);
        ret = 1;

        printf("[%s] Converting OK \n", p_fname);
    }
    E4C_CATCH (RuntimeException)
    {
        const e4c_exception * e = e4c_get_exception();
        fprintf(stderr, "[%s] Converting FAILED. Reason: %s (%s).", e->name, e->message);
        ret = -1;
    }
    E4C_CATCH (ProgramSignalException)
    {
        const e4c_exception * e = e4c_get_exception();
        fprintf(stderr, "[%s] Converting FAILED. Reason: %s (%s).", e->name, e->message);
        ret = -1;
    }

    lame_close(p_lame);

    /* Leave exception context */
    e4c_context_end();

    os_fclose(&inFile);
    os_fclose(&outFile);

    return (ret);
}

void* music_procFiles(void *threadarg)
{
    assert(threadarg != NULL);

    st_encArgs_t*   tArgs = (st_encArgs_t*) threadarg;
    int32_t         tArgsIndex = -1;
    uint16_t        procFiles = 0;
    uint16_t        tID = tArgs->threadID;

    while (1)
    {
        tArgsIndex = -1;

        pthread_mutex_lock(&music_mutex);
        for (int i = 0; i < tArgs->files; i++)
        {
            if (tArgs->p_fdesc[i].flocked == 0)
            {
                tArgs->p_fdesc[i].flocked = 1;
                tArgsIndex = i;
                break;
            }
        }
        pthread_mutex_unlock(&music_mutex);

        if (tArgsIndex >= 0) {
            music_procFile(tArgs->p_trgPath, tArgs->p_fdesc[tArgsIndex].p_fname);
            procFiles++;
        }else
            break;
    }

    printf("[%lu] Thread converted %lu files\n",tID, procFiles);
    pthread_exit(NULL);
	return NULL;
}
