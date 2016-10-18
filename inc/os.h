/*
 * os.h
 *
 *  Created on: Oct 14, 2016
 *      Author: yushev
 *      Brief:	OS dependent functions
 */

#ifndef OS_H_
#define OS_H_


/* Open a file */
int8_t  os_fOpen(uint8_t inout, st_encoder_t * enc);
/* Skip a given offset from a file read pointer */
int8_t os_fOffset(FILE* p_fp, int32_t off);
int32_t os_fExplore(char* dirPath, st_encArgs_t* tArgs, uint16_t maxElems);

extern void os_mkPath(char* p_path, char* p_dirPath, char* p_fname, uint16_t lim);

extern uint32_t os_fread_unlocked(void* p_buf, size_t size, size_t cnt, FILE* p_fp);
extern void os_fwrite_unlocked(void* p_buf, size_t size, size_t cnt, FILE* p_fp);
extern void os_fclose(st_encoder_t* p_enc);
/* Read 4 bytes in big-endian format */
extern int32_t os_read32be(FILE* p_fp);
/* Read 4 bytes in little-endian format */
extern int32_t os_read32le(FILE* p_fp);
/* Read 2 bytes in big-endian format */
extern int16_t os_read16be(FILE* p_fp);
/* Read 2 bytes in little-endian format */
extern int16_t os_read16le(FILE* p_fp);

extern void os_splitFlopUI8(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);
extern void os_splitFlopI16(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);
extern void os_splitFlopI24(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);
extern void os_splitFlopI32(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);

#endif /* OS_H_ */
