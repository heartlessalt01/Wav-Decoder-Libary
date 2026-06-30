/*
 * ========================================================================
 * Wav Decoder (Bare-Metal / Kernel Level)
 * ========================================================================
 *
 * Copyright © 2026 Heartless. All rights reserved.
 *
 * This Software is intended for use in bare-metal systems, operating
 * system kernels, bootloaders, and other low-level environments.
 *
 * OWNERSHIP
 * This Software and all associated source code are the exclusive property
 * of the author.
 *
 * PERMITTED USE (PRIVATE USE ONLY)
 * - You may use, copy, and modify this Software for personal or private
 * non-commercial purposes only, including experimental kernel or OS work.
 *
 * RESTRICTIONS
 * - Redistribution, sublicensing, publishing, or sharing of this Software,
 * in whole or in part, is strictly prohibited without explicit written
 * permission from the author.
 *
 * COMMERCIAL / DISTRIBUTION USE
 * - Any use in commercial products, paid software, embedded systems,
 * operating systems, firmware, or hardware distributions requires prior
 * written permission from the author.
 *
 * - Selling, licensing, or otherwise monetizing this Software or derived
 * works is not allowed without explicit approval.
 *
 * PERMISSION CONTACT
 * Email: heartlessalt01@gmail.com
 * heartlesalt01@gmail.com
 *
 * DISCLAIMER
 * This Software is provided "AS IS", without warranty of any kind,
 * express or implied, including but not limited to fitness for a
 * particular purpose or reliability in production systems.
 *
 * ========================================================================
 */

#ifndef WAV_DECODER_H
#define WAV_DECODER_H
#ifdef __cplusplus
extern "C" {
#endif
#ifndef NULL
    #define NULL ((void*)0)
#endif
typedef unsigned char      wav_uint8;
typedef signed char        wav_int8;
typedef unsigned short     wav_uint16;
typedef signed short       wav_int16;
typedef unsigned int       wav_uint32;
typedef signed int         wav_int32;
typedef unsigned long long wav_uint64;
#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64) || defined(__LP64__)
    typedef unsigned long wav_size_t;
#else
    typedef unsigned int wav_size_t;
#endif
typedef int wav_bool;
#define WAV_TRUE  1
#define WAV_FALSE 0
typedef enum {
    WAV_SUCCESS = 0,
    WAV_INVALID_ARGS,
    WAV_BAD_HEADER,
    WAV_INVALID_CHUNK,
    WAV_UNSUPPORTED_FORMAT,
    WAV_TRUNCATED
} wav_result;
#define WAV_FORMAT_PCM        1
#define WAV_FORMAT_MS_ADPCM   2
#define WAV_FORMAT_IMA_ADPCM  17
#define WAV_FORMAT_EXTENSIBLE 65534
#define WAV_MAX_CHANNELS 32
typedef struct {
    void* userData;
    wav_bool (*read)(void* userData, wav_uint64 offset, void* pBuffer, wav_size_t bytesToRead);
} wav_io;
typedef struct {
    wav_uint16 formatTag;
    wav_uint16 channels;
    wav_uint32 sampleRate;
    wav_uint16 blockAlign;
    wav_uint16 bitsPerSample;
    wav_uint16 validBitsPerSample;
    wav_uint32 channelMask;
    wav_uint16 samplesPerBlock;
    wav_uint8  msAdpcmNumCoefficients;
    wav_int16  msAdpcmCoefficients[7][2];
} wav_format;
typedef struct {
    wav_io     io;
    wav_size_t dataSize;
    wav_size_t currentBytePos;
    wav_size_t cacheOffset;
    wav_size_t cacheSize;
    wav_size_t dataChunkStart;
    wav_size_t dataChunkSize;
    wav_uint64 intraBlockSampleOffset;
    wav_uint8  currentBlockBufferBytesValid;
    wav_bool   isBigEndian;
    const wav_uint8* data;
    wav_uint8 cache[4096];
} wav_stream;
typedef struct {
    wav_stream  stream;
    wav_format  container;
    wav_bool isInitialized;
    wav_uint8  blockScratchpad[4096];
    wav_int16  sampleScratchpad[8192];
} wav_decoder;
wav_result wav_init_io(
    wav_decoder* pDecoder,
    wav_io ioBackend,
    wav_uint64 totalStorageSize
);
void wav_uninit(wav_decoder* pDecoder);
wav_uint64 wav_read_pcm_frames_s16(
    wav_decoder* pDec,
    wav_uint64 framesToRead,
    wav_int16* pOut
);
wav_uint64 wav_read_pcm_frames_s32(
    wav_decoder* pDec,
    wav_uint64 framesToRead,
    wav_int32* pOut
);
wav_result wav_seek_to_pcm_frame(
    wav_decoder* pDec,
    wav_uint64 frameIndex
);
wav_result wav_open_memory(
    wav_decoder* pDec,
    const void* data,
    wav_uint64 size
);
#ifdef __cplusplus
}
#endif
#endif
