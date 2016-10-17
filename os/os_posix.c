/*
 * os_posix.c
 *
 *  Created on: Oct 14, 2016
 *      Author: yushev
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <errno.h>
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
	struct  stat st;
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

			/* Ensure that the file is a regular file */
            if ((fstat(fd, &st) != 0) || (!S_ISREG(st.st_mode))) {
                E4C_THROW(RuntimeException, "Not a regular file.\n");
            }

            p_enc->len = st.st_size;
            if (p_enc->len == -1) {
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
            fd = open(mp3Filename, O_RDWR|O_CREAT, 0666);
            free(mp3Filename);
            if (fd == -1) {
                E4C_THROW(RuntimeException, "Failed to open a file.\n");
            }
            p_enc->opened = 1;

            p_enc->p_fp = fdopen(fd, "wb");
            if (p_enc->p_fp == NULL) {
                E4C_THROW(RuntimeException, "Failed to associate a descriptor "
                                            "with opened file.\n");
            }

            /* Ensure that the file is a regular file */
            if ((fstat(fd, &st) != 0) || (!S_ISREG(st.st_mode))) {
                E4C_THROW(RuntimeException, "Not a regular file.\n");
            }


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

int8_t os_fSkip(FILE* p_fp, uint32_t off)
{
    int8_t err = 0;
    /* Find the end of the file in a safe way */
    if (fseeko(p_fp, off , SEEK_CUR) != 0) {
        err = -1;
    }

    return (err);
}

int8_t os_fSeek(FILE* p_fp, int32_t off)
{
    int8_t err = 0;
    /* Move a pointer from current place backwards */
    if (fseeko(p_fp, off , SEEK_CUR) != 0) {
        err = -1;
    }

    return (err);
}

int16_t os_fExplore(char* dirPath, uint16_t dirOff,
                    st_encArgs_t* threadArgs, uint16_t maxElems)
{

    assert(threadArgs != NULL);
    assert(dirPath != NULL);

    DIR*            dirDesc = NULL;
    struct dirent*  dirFile = NULL;
    int16_t         dirSize = 0;

    /* Scanning the in directory */
    if ((dirDesc = opendir (dirPath)) == NULL) {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        dirSize = -1;
    } else {
        /* While we have files or correctly reallocated memory keep reading*/
        while ((dirFile = readdir(dirDesc)) && (dirSize < maxElems) )
        {
            /* Skip all directories, unsupported files and already processed files */
            if (!strcmp (dirFile->d_name, "."))
                continue;
            if (!strcmp (dirFile->d_name, ".."))
                continue;
            if (dirFile->d_type == 4)
                continue;
            if (__extIsSupported(dirFile->d_name) <= 0)
                continue;
            if (dirOff > 0) {
                dirOff--;
                continue;
            }
            /* We've found a file, duplicate memory*/
            threadArgs[dirSize].p_filename = strdup(dirFile->d_name);
            /* Move pointer to a next element in array of filenames */
            /* Increment file amount of files */
            dirSize++;
        }
    }

    return (dirSize);
}

int32_t os_read32be(FILE* p_fp)
{
    uint8_t data[4] = { 0, 0, 0, 0 };
    fread_unlocked(data, 1, 4, p_fp);
    {
        int32_t const high = data[0];
        int32_t const medh = data[1];
        int32_t const medl = data[2];
        int32_t const low = data[3];
        return (high << 24) | (medh << 16) | (medl << 8) | low;
    }
}

int32_t os_read32le(FILE* p_fp)
{
    uint8_t data[4] = { 0, 0, 0, 0 };
    fread_unlocked(data, 1, 4, p_fp);
    {
        int32_t const high = data[3];
        int32_t const medh = data[2];
        int32_t const medl = data[1];
        int32_t const low = data[0];
        return (high << 24) | (medh << 16) | (medl << 8) | low;
    }
}

int16_t os_read16be(FILE* p_fp)
{
    uint8_t data[2] = { 0, 0};
    fread_unlocked(data, 1, 2, p_fp);
    {
        int32_t const high = data[0];
        int32_t const low = data[1];
        return (high << 8) | low;
    }
}

int16_t os_read16le(FILE* p_fp)
{
    uint8_t data[2] = { 0, 0 };
    fread_unlocked(data, 1, 2, p_fp);
    {
        int32_t const high = data[1];
        int32_t const low = data[0];
        return (high << 8) | low;
    }
}
