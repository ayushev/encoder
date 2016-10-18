/*
 * music.h
 *
 *  Created on: Oct 16, 2016
 *      Author: yushev
 */

#ifndef MUSIC_H_
#define MUSIC_H_

int8_t music_procFile(char* p_dirPath, char* p_fname);
void* music_procFiles(void *threadarg);

#endif /* MUSIC_H_ */
