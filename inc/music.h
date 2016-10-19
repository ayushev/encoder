/*
 * music.h
 *
 *  Created on: Oct 16, 2016
 *      Author: yushev
 */

#ifndef MUSIC_H_
#define MUSIC_H_

/*
 * --- Global Functions Declaration ----------------------------------------- *
 */

/**
 * \brief     Process individual music file and convert it in mp3
 *            We have separated these two strings, as storing relative/absolute
 *            paths for each file to expensive
 * \param     p_dirPath     Directory name string
 * \param     p_fname       Filename string
 * \return    Negative for failure, otherwise OK
 */
int8_t music_procFile(char* p_dirPath, char* p_fname);

/**
 * \brief     Function to process files in the given directory
 *
 * \param     p_threadarg     Pointer to argument given to thread
 * \return    Void
 */
void* music_procFiles(void* p_threadarg);

#endif /* MUSIC_H_ */
