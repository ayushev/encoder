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
int8_t  os_fSkip(FILE* p_fp, uint32_t off);
int8_t  os_fSeek(FILE* p_fp, int32_t off);
int16_t os_fExplore(char* dirPath, uint16_t dirOff,
                    st_encArgs_t* threadArgs, uint16_t maxElems);
/* Read 4 bytes in big-endian format */
int32_t os_read32be(FILE* p_fp);
/* Read 4 bytes in little-endian format */
int32_t os_read32le(FILE* p_fp);
/* Read 2 bytes in big-endian format */
int16_t os_read16be(FILE* p_fp);
/* Read 2 bytes in little-endian format */
int16_t os_read16le(FILE* p_fp);

#endif /* OS_H_ */
