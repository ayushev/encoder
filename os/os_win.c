/*
 * os_win.c
 *
 *  Created on: Oct 14, 2016
 *      Author: yushev
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <io.h>
#include <windows.h>
#include <tchar.h>
#include <assert.h>

#include "encoder.h"
#include "os.h"
#include "e4c.h"

static char * __extSubstitute(char* to, const char* from);
static int8_t __extIsSupported(const char* from);

static char * __extSubstitute(char* to, const char* from)
{
    assert(to != NULL);
    assert(from != NULL);

    char *lastdot;

    strcpy (to, from);
    lastdot = strrchr (to, '.');
    if (lastdot != NULL)
        snprintf(lastdot, MAX_FILEPATH,".mp3");
    return to;
}

/* 0 not supported, < 0 error, 1 supported */
static int8_t __extIsSupported(const char* from)
{
    assert(from != NULL);

    char*       lastdot;
    int8_t      ret = 0;

    lastdot = strrchr (from, '.');
    if (lastdot != NULL) {
        /* ToDo: hardcoded part, but not critical */
        if ((strcmp(lastdot,".wave") == 0) ||
            (strcmp(lastdot,".wav") == 0)) {
            ret = 1;
        }
    } else {
        ret = -1;
    }
    return (ret);
}

int8_t os_fOpen(uint8_t read, st_encoder_t * p_enc)
{
    assert(p_enc != NULL);
    assert(p_enc->path != NULL);

    int8_t  err = 0;
    int     fd = 0;

    E4C_TRY {
        if (read) {
			fd = open(p_enc->path, O_RDONLY);
			if (fd == -1) {
				E4C_THROW(RuntimeException, "Failed to open a file\n");
			}
			p_enc->opened = 1;

            p_enc->p_fp = fdopen(fd, "rb");
            if (p_enc->p_fp == NULL) {
                E4C_THROW(RuntimeException, "Failed to associate a descriptor"
                                            "with opened file.\n");
            }

			p_enc->fsize = _filelength(fd);
            if (p_enc->fsize == -1) {
                E4C_THROW(RuntimeException, "Failed to calculate the size of a file.\n");
            }

        } else {
            char* mp3Filename = NULL;
            if ((mp3Filename = malloc (strlen (p_enc->path) + 1)) == NULL) {
                E4C_THROW(RuntimeException, "Failed to allocate memmory "
                                            "for filename.\n");
            }
            __extSubstitute(mp3Filename, p_enc->path);

            /* Open a file to write*/
			_sopen_s(&fd, p_enc->path, _O_RDONLY, _SH_DENYRW, _S_IREAD);
            if (fd == -1) {
                E4C_THROW(RuntimeException, "Failed to open a file \n");
            }
			
			p_enc->p_fp =_fdopen(fd, "wb");
			if (p_enc->p_fp == NULL) {
                E4C_THROW(RuntimeException, "Failed to associate a descriptor"
                                                            "with opened file.\n");
            }
            free(mp3Filename);
            p_enc->opened = 1;
        }
    }
    E4C_CATCH (RuntimeException){
        /* We opened a file, now we need to close it */
        if (p_enc->opened) {
            close(fd);
        }

        const e4c_exception * e = e4c_get_exception();
        fprintf(stderr, "Error: %s (%s) [%s].", e->name, e->message, p_enc->path);
        err = -1;
    }

    return (err);
}

int8_t os_fOffset(FILE* p_fp, int32_t off)
{
    int8_t err = 0;
    /* Find the end of the file in a safe way */
    if (fseeko(p_fp, off , SEEK_CUR) != 0) {
        err = -1;
    }
    return (err);
}

int32_t os_fExplore(char* dirPath, st_encArgs_t* threadArgs, uint16_t maxElems)
{

    assert(threadArgs != NULL);
    assert(dirPath != NULL);

    TCHAR 			p_dir[MAX_PATH];
    int32_t         dirSize = 0;
	WIN32_FIND_DATA ffd;
	HANDLE 			hFind = INVALID_HANDLE_VALUE;

    /* Scanning the in directory */
	/* While we have files or correctly reallocated memory keep reading*/
	do
	{
		/* Skip all directories, unsupported files and already processed files */
		strncpy(p_dir, dirPath, MAX_PATH);
		strncat(p_dir, TEXT("\\*"), MAX_PATH);
		
		// Find the first file in the directory.
		hFind = FindFirstFile(p_dir, &ffd);
		if (INVALID_HANDLE_VALUE == hFind) 
		{
			fprintf(stderr, "%s\n",TEXT("FindFirstFile"));
			dirSize = -1;
			break;
		}
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue
		if (__extIsSupported(ffd.cFileName) <= 0)
			continue;
		if (dirOff > 0) {
			dirOff--;
			continue;
		}
		/* We've found a file, duplicate memory*/
		threadArgs[dirSize].p_filename = strdup(ffd.cFileName);
		/* Move pointer to a next element in array of filenames */
		/* Increment file amount of files */
		dirSize++;
	} while ((FindNextFile(hFind, &ffd) != 0) && ((dirSize < maxElems)))


    return (dirSize);
}

inline int32_t os_read32be(FILE* p_fp)
{
    uint8_t data[4] = { 0, 0, 0, 0 };
    _fread_nolock(data, 1, 4, p_fp);
	int32_t const high = data[0];
	int32_t const medh = data[1];
	int32_t const medl = data[2];
	int32_t const low = data[3];
	return (high << 24) | (medh << 16) | (medl << 8) | low;
}

inline int32_t os_read32le(FILE* p_fp)
{
    uint8_t data[4] = { 0, 0, 0, 0 };
    _fread_nolock(data, 1, 4, p_fp);
	int32_t const high = data[3];
	int32_t const medh = data[2];
	int32_t const medl = data[1];
	int32_t const low = data[0];
	return (high << 24) | (medh << 16) | (medl << 8) | low;
}

inline int16_t os_read16be(FILE* p_fp)
{
    uint8_t data[2] = { 0, 0};
    _fread_nolock(data, 1, 2, p_fp);
	int32_t const high = data[0];
	int32_t const low = data[1];
	return (high << 8) | low;
}

inline int16_t os_read16le(FILE* p_enc)
{
    uint8_t data[2] = { 0, 0 };
    _fread_nolock(data, 1, 2, p_fp);
    int32_t const high = data[1];
    int32_t const low = data[0];
    return (high << 8) | low;
}

inline void os_fread_unlocked(void* p_buf, size_t size, size_t cnt, FILE* p_fp)
{
	_fread_nolock(p_buf, size, cnt, p_fp);
}

inline void os_fwrite_unlocked(void* p_buf, size_t size, size_t cnt, FILE* p_fp)
{
	_fwrite_nolock(p_buf, size, cnt, p_fp);
}

inline void os_fclose(st_encoder_t* p_enc)
{
	if (p_enc->opened)
		fclose( p_enc->p_fp);
}


inline void os_splitFlopUI8(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff)
{
    int     i = 0;
    uint8_t off = sizeof(uint8_t);

    while (1)
    {
        if (((i + off) < toMaxOff) && (toFir != NULL))
        {
            *toFir = (from[i] ^ 0x80) << 24;
            toFir++;
            i += off;
        }

        if (((i + off) < toMaxOff) && (toSec != NULL))
        {
            *toSec = (from[i] ^ 0x80) << 24;
            toSec++;
            i += off;
        }

        if ((i + off) == toMaxOff)
            break;
    }
}

inline void os_splitFlopI16(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff)
{
    int     i = 0;
    uint8_t off = sizeof(int16_t);

    while (1)
    {
        if (((i + off) < toMaxOff) && (toFir != NULL))
        {
            *toFir = from[i + 1] << 24 | from[i] << 16;
            toFir++;
            i += off;
        }

        if (((i + off) < toMaxOff) && (toSec != NULL))
        {
            *toSec = from[i + 1] << 24 | from[i] << 16;
            toSec++;
            i += off;
        }

        if ((i + off) == toMaxOff)
            break;
    }
}

inline void os_splitFlopI24(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff)
{
    int     i = 0;
    uint8_t off = 3;

    while (1)
    {
        if (((i + off) < toMaxOff) && (toFir != NULL))
        {
            *toFir = from[i + 2] << 24 | from[i + 1] << 16 | from[i] << 8;
            toFir++;
            i += off;
        }

        if (((i + off) < toMaxOff) && (toSec != NULL))
        {
            *toSec = from[i + 2] << 24 | from[i + 1] << 16 | from[i] << 8;
            toSec++;
            i += off;
        }

        if ((i + off) == toMaxOff)
            break;
    }
}

inline void os_splitFlopI32(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff)
{
    int     i = 0;
    uint8_t off = sizeof(int32_t);

    while (1)
    {
        if (((i + off) < toMaxOff) && (toFir != NULL))
        {
            *toFir = from[i + 3] << 24 | from[i + 2] << 16 | from[i + 1] << 8 | from[i];
            toFir++;
            i += off;
        }

        if (((i + off) < toMaxOff) && (toSec != NULL))
        {
            *toSec = from[i + 3] << 24 | from[i + 2] << 16 | from[i + 1] << 8 | from[i];
            toSec++;
            i += off;
        }

        if ((i + off) == toMaxOff)
            break;
    }
}

inline uint8_t os_flop_ui8i32(uint8_t* from, int32_t* to, uint32_t toOff, uint32_t toMaxOff)
{
    uint8_t res = 1;
    if ((toOff < toMaxOff) && (to != NULL))
    {
        *to = (*from ^ 0x80) << 24;
    } else {
        res = 0;
    }
    return (res);
}

inline uint8_t os_flop_i16i32(uint8_t* from, int32_t* to, uint32_t toOff, uint32_t toMaxOff)
{
    uint8_t res = 1;
    if ((toOff < toMaxOff) && (to != NULL))
    {
        *to = (*from+1) << 24 | *from << 16;
    } else {
        res = 0;
    }
    return (res);
}

inline uint8_t os_flop_i32i32(uint8_t* from, int32_t* to, uint32_t toOff, uint32_t toMaxOff)
{
    uint8_t res = 1;
    if ((toOff < toMaxOff) && (to != NULL))
    {
        *to = (*from+3) << 24 | (*from+2) << 16 | (*from+1) << 8 | *from;
    } else {
        res = 0;
    }
    return (res);
}
