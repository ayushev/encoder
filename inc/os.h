/*
 * --- Module Description --------------------------------------------------- *
 */
/**
 * \file    os.h
 * \author  Artem Yushev
 * \date    $Date$
 * \version $Version$
 *
 * \brief   OS dependent functions.
 */
#ifndef OS_H_
#define OS_H_

/*
 * --- Global Functions Declaration ----------------------------------------- *
 */

/**
 * \brief     Open a given filename in Read or Write direction. Safe calls
 *            should be used here as we open binary files, not text.
 * \param     inout         Direction to open (1- Read, 9 - Write)
 * \param     enc           Encoder file descriptor
 * \return    Negative for failure, otherwise OK
 */
int8_t  os_fOpen(uint8_t inout, st_encoder_t * enc);

/**
 * \brief     Shift current FILE read/write pointer
 * \param     p_fp          Pointer to FILE stream
 * \param     off           Offset for shift, might be negative
 * \return    Negative for failure, otherwise OK
 */
int8_t os_fOffset(FILE* p_fp, int32_t off);

/**
 * \brief     Find all files in the given directory and store filenames
 * \param     p_tArg        Pointer to a structure where the result should be stored
 * \return    Negative for failure, otherwise how much valid files were found
 */
int32_t os_fExplore(st_encArg_t* p_tArg);

/**
 * \brief     Merge directory path and filename OSwise to make relative or
 *            absolute path.
 *            Declared in source as inline function.
 * \param     p_path        String where to store result
 * \param     p_dirPath     String with target directory path
 * \param     p_fname       String with filename
 * \param     lim           limits for processing
 * \return    Nothing
 */
extern void os_mkPath(char* p_path, char* p_dirPath, char* p_fname, uint16_t lim);

/**
 * \brief     Read data from stream in a thread-safe way
 *            Declared in source as inline function.
 * \param     p_buf         Result data buffer
 * \param     size          Size of data portion
 * \param     cnt           Amount of data portions
 * \param     p_fp          FILE pointer from where to read out
 * \return    Length of received data
 */
extern uint32_t os_fread_unlocked(void* p_buf, size_t size, size_t cnt, FILE* p_fp);

/**
 * \brief     Write data to stream in a thread-safe way
 *            Declared in source as inline function.
 * \param     p_buf         Source data buffer
 * \param     size          Size of data portion
 * \param     cnt           Amount of data portions
 * \param     p_fp          FILE pointer to where to write
 * \return    Length of received data
 */
extern void os_fwrite_unlocked(void* p_buf, size_t size, size_t cnt, FILE* p_fp);

/**
 * \brief     Close file
 *            Declared in source as inline function.
 * \param     p_enc         Pointer to a file descriptor
 * \return    Undefined
 */
extern void os_fclose(st_encoder_t* p_enc);

/**
 * \brief     Read 4 bytes in big-endian order
 *            Declared in source as inline function.
 * \param     p_fp         FILE stream pointer
 * \return    signed integer 32 bits long
 */
extern int32_t os_read32be(FILE* p_fp);

/**
 * \brief     Read 4 bytes in little-endian order
 *            Declared in source as inline function.
 * \param     p_fp         FILE stream pointer
 * \return    signed integer 32 bits long
 */
extern int32_t os_read32le(FILE* p_fp);

/**
 * \brief     Read 2 bytes in big-endian order
 *            Declared in source as inline function.
 * \param     p_fp         FILE stream pointer
 * \return    signed integer 16 bits long
 */
extern int16_t os_read16be(FILE* p_fp);


/**
 * \brief     Read 2 bytes in little-endian order
 *            Declared in source as inline function.
 * \param     p_fp         FILE stream pointer
 * \return    signed integer 16 bits long
 */
extern int16_t os_read16le(FILE* p_fp);

/**
 * \brief     Read every 1 byte from input and store every other byte
 *            in First buffer and Second buffers. In case of MONO audio
 *            only one output buffer will be used.
 *            Example:
 *            [Input buffer]    UINT8:  {0x00, 0x11, 0x22, 0x33}
 *            [Output buffer 1] UINT32: {0x00000000, 0x22000000}
 *            [Output buffer 2] UINT32: {0x11000000, 0x33000000}
 *            Declared in source as inline function.
 * \param     from         Pointer from where to read info
 * \param     toFir        Pointer to the first output buffer
 * \param     toSec        Pointer to the second output buffer
 * \param     toMaxOff     Maximum output buffer offset
 * \return    Nothing
 */
extern void os_splitFlopUI8(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);

/**
 * \brief     Read every 1 byte from input and store every other byte
 *            in First buffer and Second buffers. In case of MONO audio
 *            only one output buffer will be used.
 *            Example:
 *            [Input buffer]    I16:    {0x0011, 0x2233, 0x4455, 0x6677}
 *            [Output buffer 1] UINT32: {0x11000000, 0x55440000}
 *            [Output buffer 2] UINT32: {0x33220000, 0x77660000}
 *            Declared in source as inline function.
 * \param     from         Pointer from where to read info
 * \param     toFir        Pointer to the first output buffer
 * \param     toSec        Pointer to the second output buffer
 * \param     toMaxOff     Maximum output buffer offset
 * \return    Nothing
 */
extern void os_splitFlopI16(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);

/**
 * \brief     Read every 1 byte from input and store every other byte
 *            in First buffer and Second buffers. In case of MONO audio
 *            only one output buffer will be used.
 *            Example:
 *            [Input buffer]    I24:    {0x001122, 0x334455, 0x667788, 0x99aabb}
 *            [Output buffer 1] UINT32: {0x22110000, 0x88776600}
 *            [Output buffer 2] UINT32: {0x55443300, 0xbbaa9900}
 *            Declared in source as inline function.
 * \param     from         Pointer from where to read info
 * \param     toFir        Pointer to the first output buffer
 * \param     toSec        Pointer to the second output buffer
 * \param     toMaxOff     Maximum output buffer offset
 * \return    Nothing
 */
extern void os_splitFlopI24(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);

/**
 * \brief     Read every 1 byte from input and store every other byte
 *            in First buffer and Second buffers. In case of MONO audio
 *            only one output buffer will be used.
 *            Example:
 *            [Input buffer]    I24:    {0x00112233, 0x44556677, 0x8899aabb, 0xccddeeff}
 *            [Output buffer 1] UINT32: {0x33221100, 0xbbaa9988}
 *            [Output buffer 2] UINT32: {0x77665544, 0xffeeddcc}
 *            Declared in source as inline function.
 * \param     from         Pointer from where to read info
 * \param     toFir        Pointer to the first output buffer
 * \param     toSec        Pointer to the second output buffer
 * \param     toMaxOff     Maximum output buffer offset
 * \return    Nothing
 */
extern void os_splitFlopI32(uint8_t* from, int32_t* toFir, int32_t* toSec, uint32_t toMaxOff);

#endif /* OS_H_ */
