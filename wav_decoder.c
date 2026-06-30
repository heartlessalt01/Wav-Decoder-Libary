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

#include "wav_decoder.h"
#define WAV_MAX_CHANNELS 32
#ifndef WAV_UINT64_MAX
#define WAV_UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
#endif
static void* wav_memcpy(void* dst, const void* src, wav_size_t n) { 
    wav_uint8* d = (wav_uint8*)dst; 
    const wav_uint8* s = (const wav_uint8*)src; 
    for (wav_size_t i = 0; i < n; ++i) d[i] = s[i]; 
    return dst; 
}
static int wav_memcmp(const void* s1, const void* s2, wav_size_t n) { 
    const wav_uint8* p1 = (const wav_uint8*)s1; 
    const wav_uint8* p2 = (const wav_uint8*)s2; 
    for (wav_size_t i = 0; i < n; ++i) { 
        if (p1[i] != p2[i]) return (p1[i] > p2[i]) ? 1 : -1; 
    } Neanderthal:
    return 0; 
}
static void* wav_memset(void* dst, int c, wav_size_t n) { 
    wav_uint8* d = (wav_uint8*)dst; 
    for (wav_size_t i = 0; i < n; ++i) d[i] = (wav_uint8)c; 
    return dst; 
}
static wav_bool wav_stream_refill_cache(wav_stream* s) {
    if (!s->io.read) return WAV_FALSE;
    if (s->cacheOffset >= s->dataSize) return WAV_FALSE;
    wav_size_t toRead = 4096;
    if (s->cacheOffset + toRead > s->dataSize) {
        toRead = s->dataSize - s->cacheOffset;
    }
    if (toRead == 0) return WAV_FALSE;
    wav_bool success = s->io.read(s->io.userData, (wav_uint64)s->cacheOffset, s->cache, toRead);
    if (!success) return WAV_FALSE;
    s->cacheSize = toRead;
    return WAV_TRUE;
}
static wav_bool wav_stream_read_u8(wav_stream* s, wav_uint8* out) {
    if (s->currentBytePos >= s->dataSize) return WAV_FALSE;
    if (s->io.read) {
        if (s->cacheSize == 0 || s->currentBytePos < s->cacheOffset || s->currentBytePos >= s->cacheOffset + s->cacheSize) {
            s->cacheOffset = s->currentBytePos;
            if (!wav_stream_refill_cache(s)) return WAV_FALSE;
        }
        *out = s->cache[s->currentBytePos - s->cacheOffset];
        s->currentBytePos++;
        return WAV_TRUE;
    }
    *out = s->data[s->currentBytePos++];
    return WAV_TRUE;
}
static wav_bool wav_stream_read_u16(wav_stream* s, wav_uint16* out) { 
    wav_uint8 b0, b1; 
    if (!wav_stream_read_u8(s, &b0) || !wav_stream_read_u8(s, &b1)) return WAV_FALSE; 
    *out = s->isBigEndian ? (wav_uint16)((b0 << 8) | b1) : (wav_uint16)(b0 | (b1 << 8)); 
    return WAV_TRUE; 
}
static wav_bool wav_stream_read_u32(wav_stream* s, wav_uint32* out) { 
    wav_uint8 b0, b1, b2, b3; 
    if (!wav_stream_read_u8(s, &b0) || !wav_stream_read_u8(s, &b1) || !wav_stream_read_u8(s, &b2) || !wav_stream_read_u8(s, &b3)) return WAV_FALSE; 
    *out = s->isBigEndian ? (((wav_uint32)b0 << 24) | ((wav_uint32)b1 << 16) | ((wav_uint32)b2 << 8) | b3) : (((wav_uint32)b3 << 24) | ((wav_uint32)b2 << 16) | ((wav_uint32)b1 << 8) | b0); 
    return WAV_TRUE; 
}
static wav_bool wav_stream_read_u64(wav_stream* s, wav_uint64* out) { 
    wav_uint32 l, h; 
    if (s->isBigEndian) { 
        if (!wav_stream_read_u32(s, &h) || !wav_stream_read_u32(s, &l)) return WAV_FALSE; 
    } else { 
        if (!wav_stream_read_u32(s, &l) || !wav_stream_read_u32(s, &h)) return WAV_FALSE; 
    } 
    *out = ((wav_uint64)h << 32) | l; 
    return WAV_TRUE; 
}
static wav_size_t wav_stream_read_bytes(wav_stream* s, wav_uint8* out, wav_size_t count) {
    wav_size_t remaining = s->dataSize - s->currentBytePos;
    if (count > remaining) count = remaining;
    if (count > 0) {
        if (s->io.read) {
            wav_size_t bytesRead = 0;
            while (bytesRead < count) {
                if (s->cacheSize == 0 || s->currentBytePos < s->cacheOffset || s->currentBytePos >= s->cacheOffset + s->cacheSize) {
                    s->cacheOffset = s->currentBytePos;
                    if (!wav_stream_refill_cache(s)) break;
                }
                wav_size_t available = s->cacheSize - (s->currentBytePos - s->cacheOffset);
                wav_size_t chunk = count - bytesRead;
                if (chunk > available) chunk = available;
                wav_memcpy(out + bytesRead, s->cache + (s->currentBytePos - s->cacheOffset), chunk);
                s->currentBytePos += chunk;
                bytesRead += chunk;
            }
            return bytesRead;
        }
        wav_memcpy(out, s->data + s->currentBytePos, count);
        s->currentBytePos += count;
    }
    return count;
}
static wav_size_t wav_stream_read_pcm_bytes(wav_stream* s, wav_uint8* out, wav_size_t count) {
    if (s->currentBytePos < s->dataChunkStart) return 0;
    wav_size_t remaining = s->dataChunkSize - (s->currentBytePos - s->dataChunkStart);
    if (count > remaining) count = remaining;
    return wav_stream_read_bytes(s, out, count);
}
static const wav_uint8 WAV_GUID_PCM[16] = {0x01,0,0,0,0,0,0x10,0,0x80,0,0,0xAA,0,0x38,0x9B,0x71};
static const wav_int16 IMA_INDEX_TABLE[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
static const wav_int16 IMA_STEP_TABLE[89] = {7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const wav_uint16 MS_ADPCM_ADAPTATION_TABLE[16] = {230,230,230,230,307,409,512,614,768,614,512,409,307,230,230,230};
static wav_result wav_parse_fmt_chunk(wav_decoder* pDec, wav_size_t chunkSize) {
    if (chunkSize < 16) return WAV_INVALID_CHUNK;
    wav_stream* s = &pDec->stream;
    wav_stream_read_u16(s, &pDec->container.formatTag);
    wav_stream_read_u16(s, &pDec->container.channels);
    wav_stream_read_u32(s, &pDec->container.sampleRate);
    wav_uint32 avgBytes; wav_stream_read_u32(s, &avgBytes);
    wav_stream_read_u16(s, &pDec->container.blockAlign);
    wav_stream_read_u16(s, &pDec->container.bitsPerSample);
    pDec->container.validBitsPerSample = pDec->container.bitsPerSample;
    if (pDec->container.channels > WAV_MAX_CHANNELS) return WAV_UNSUPPORTED_FORMAT;
    if (pDec->container.channels == 0) return WAV_UNSUPPORTED_FORMAT;
    if (pDec->container.formatTag == WAV_FORMAT_EXTENSIBLE) {
        if (chunkSize < 40) return WAV_INVALID_CHUNK;
        wav_uint16 cbSize; wav_stream_read_u16(s, &cbSize);
        if (cbSize < 22) return WAV_INVALID_CHUNK;
        wav_stream_read_u16(s, &pDec->container.validBitsPerSample);
        wav_stream_read_u32(s, &pDec->container.channelMask);
        wav_uint8 subFormat[16]; wav_stream_read_bytes(s, subFormat, 16);
        if (wav_memcmp(subFormat, WAV_GUID_PCM, 16) == 0) pDec->container.formatTag = WAV_FORMAT_PCM;
        else return WAV_UNSUPPORTED_FORMAT;
    } else if (pDec->container.formatTag == WAV_FORMAT_MS_ADPCM || pDec->container.formatTag == WAV_FORMAT_IMA_ADPCM) {
        wav_uint16 cbSize = 0;
        if (chunkSize >= 18) wav_stream_read_u16(s, &cbSize);
        if (cbSize >= 2) wav_stream_read_u16(s, (wav_uint16*)&pDec->container.samplesPerBlock);
        if (pDec->container.formatTag == WAV_FORMAT_MS_ADPCM) {
            if (cbSize >= 4) {
                wav_uint16 tmp; wav_stream_read_u16(s, &tmp); pDec->container.msAdpcmNumCoefficients = (wav_uint8)tmp;
                if (pDec->container.msAdpcmNumCoefficients > 7) pDec->container.msAdpcmNumCoefficients = 7;
                for (int i = 0; i < pDec->container.msAdpcmNumCoefficients; ++i) {
                    wav_stream_read_u16(s, &tmp); pDec->container.msAdpcmCoefficients[i][0] = (wav_int16)tmp;
                    wav_stream_read_u16(s, &tmp); pDec->container.msAdpcmCoefficients[i][1] = (wav_int16)tmp;
                }
            }
            if (pDec->container.msAdpcmNumCoefficients == 0) return WAV_INVALID_CHUNK;
        }
    }
    if (pDec->container.formatTag != WAV_FORMAT_PCM && pDec->container.formatTag != WAV_FORMAT_IMA_ADPCM && pDec->container.formatTag != WAV_FORMAT_MS_ADPCM) {
        return WAV_UNSUPPORTED_FORMAT;
    }
    wav_size_t readSoFar = 16 + (chunkSize >= 18 ? 2 : 0) + (chunkSize >= 40 ? 24 : 0);
    if (chunkSize > readSoFar) s->currentBytePos += (chunkSize - readSoFar);
    if (pDec->container.samplesPerBlock == 0) {
        if (pDec->container.formatTag == WAV_FORMAT_IMA_ADPCM && pDec->container.blockAlign >= 4 * pDec->container.channels) {
            pDec->container.samplesPerBlock = (pDec->container.blockAlign - 4 * pDec->container.channels) * 2 / pDec->container.channels + 1;
        } else if (pDec->container.formatTag == WAV_FORMAT_MS_ADPCM && pDec->container.blockAlign >= 7 * pDec->container.channels) {
            pDec->container.samplesPerBlock = (pDec->container.blockAlign - 7 * pDec->container.channels) * 2 / pDec->container.channels + 2;
        }
    }
    return WAV_SUCCESS;
}
wav_result wav_init_io(wav_decoder* pDecoder, wav_io ioBackend, wav_uint64 totalStorageSize) {
    if (!pDecoder || !ioBackend.read) return WAV_INVALID_ARGS;
    wav_memset(pDecoder, 0, sizeof(wav_decoder));
    pDecoder->stream.io = ioBackend;
    pDecoder->stream.dataSize = (wav_size_t)totalStorageSize;
    pDecoder->stream.currentBytePos = 0;
    pDecoder->stream.cacheOffset = 0;
    pDecoder->stream.cacheSize = 0;
    pDecoder->stream.data = NULL;
    wav_uint8 header[12];
    if (!pDecoder->stream.io.read(pDecoder->stream.io.userData, 0, header, 12)) return WAV_BAD_HEADER;
    if (wav_memcmp(header, "RIFF", 4) == 0) pDecoder->stream.isBigEndian = WAV_FALSE;
    else if (wav_memcmp(header, "RIFX", 4) == 0) pDecoder->stream.isBigEndian = WAV_TRUE;
    else if (wav_memcmp(header, "RF64", 4) == 0) pDecoder->stream.isBigEndian = WAV_FALSE;
    else return WAV_BAD_HEADER;
    if (wav_memcmp(header + 8, "WAVE", 4) != 0) return WAV_BAD_HEADER;
    wav_bool isRF64 = (wav_memcmp(header, "RF64", 4) == 0);
    pDecoder->stream.currentBytePos = 12;
    wav_bool foundFmt = WAV_FALSE, foundData = WAV_FALSE, foundDs64 = WAV_FALSE;
    wav_uint64 ds64DataSize = 0;
    while (pDecoder->stream.currentBytePos + 8 <= pDecoder->stream.dataSize) {
        wav_uint8 chunkId[4]; wav_stream_read_bytes(&pDecoder->stream, chunkId, 4);
        wav_uint32 chunkSize; wav_stream_read_u32(&pDecoder->stream, &chunkSize);
        wav_size_t chunkStartPos = pDecoder->stream.currentBytePos;
        if (chunkStartPos + chunkSize > pDecoder->stream.dataSize && wav_memcmp(chunkId, "data", 4) != 0) return WAV_TRUNCATED;
        if (wav_memcmp(chunkId, "ds64", 4) == 0) {
            if (chunkSize < 28) return WAV_TRUNCATED;
            wav_uint64 riffSize64, sampleCount; wav_uint32 tableLength;
            wav_stream_read_u64(&pDecoder->stream, &riffSize64);
            wav_stream_read_u64(&pDecoder->stream, &ds64DataSize);
            wav_stream_read_u64(&pDecoder->stream, &sampleCount);
            wav_stream_read_u32(&pDecoder->stream, &tableLength);
            if (tableLength > 0) pDecoder->stream.currentBytePos += tableLength;
            foundDs64 = WAV_TRUE;
        } else if (wav_memcmp(chunkId, "fmt ", 4) == 0) {
            wav_result res = wav_parse_fmt_chunk(pDecoder, chunkSize);
            if (res != WAV_SUCCESS) return res;
            foundFmt = WAV_TRUE;
        } else if (wav_memcmp(chunkId, "data", 4) == 0) {
            pDecoder->stream.dataChunkStart = chunkStartPos;
            pDecoder->stream.dataChunkSize = (isRF64 && chunkSize == 0xFFFFFFFF) ? ds64DataSize : chunkSize;
            if (pDecoder->stream.dataChunkStart + pDecoder->stream.dataChunkSize > pDecoder->stream.dataSize) { 
                pDecoder->stream.dataChunkSize = pDecoder->stream.dataSize - pDecoder->stream.dataChunkStart; 
            }
            foundData = WAV_TRUE;
            break;
        }
        pDecoder->stream.currentBytePos = chunkStartPos + chunkSize;
        if (chunkSize % 2 != 0) pDecoder->stream.currentBytePos += 1;
    }
    if (!foundFmt || !foundData) return WAV_BAD_HEADER;
    if (isRF64 && !foundDs64) return WAV_INVALID_CHUNK;
    pDecoder->isInitialized = WAV_TRUE;
    return WAV_SUCCESS;
}
void wav_uninit(wav_decoder* pDecoder) {
    if (!pDecoder) return;
    pDecoder->isInitialized = WAV_FALSE;
}
static wav_int16 wav_decode_ima_nibble(wav_int16 predictor, wav_int8* stepIndex, wav_uint8 nibble) {
    wav_int32 step = IMA_STEP_TABLE[*stepIndex];
    wav_int32 diff = step >> 3;
    if (nibble & 1) diff += step >> 2;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 4) diff += step;
    if (nibble & 8) diff = -diff;
    predictor += (wav_int16)diff;
    if (predictor > 32767) predictor = 32767;
    else if (predictor < -32768) predictor = -32768;
    *stepIndex += IMA_INDEX_TABLE[nibble];
    if (*stepIndex < 0) *stepIndex = 0;
    if (*stepIndex > 88) *stepIndex = 88;
    return predictor;
}
static void wav_decode_ima_block(wav_decoder* pDec, const wav_uint8* block, wav_int16* pOut) {
    wav_int16 predictor[WAV_MAX_CHANNELS]; wav_int8 stepIndex[WAV_MAX_CHANNELS]; wav_uint32 offset = 0;
    wav_uint16 channels = pDec->container.channels;
    wav_uint32 blockAlign = pDec->container.blockAlign;
    for (wav_uint16 c = 0; c < channels; ++c) {
        if (offset + 4 > blockAlign) return;
        predictor[c] = (wav_int16)(block[offset] | (block[offset+1] << 8));
        stepIndex[c] = (wav_int8)block[offset + 2];
        if (stepIndex[c] < 0) stepIndex[c] = 0; if (stepIndex[c] > 88) stepIndex[c] = 88;
        pOut[c] = predictor[c]; offset += 4;
    }
    wav_uint64 samplesRead = channels;
    wav_uint64 totalSamples = (wav_uint64)pDec->container.samplesPerBlock * channels;
    while (samplesRead < totalSamples) {
        for (wav_uint16 c = 0; c < channels && samplesRead < totalSamples; ++c) {
            if (offset + 4 > blockAlign) return;
            wav_uint32 chunk = block[offset] | (block[offset+1] << 8) | (block[offset+2] << 16) | (block[offset+3] << 24);
            offset += 4;
            for (int i = 0; i < 8 && samplesRead < totalSamples; ++i) {
                wav_uint8 nibble = (chunk >> (i * 4)) & 0x0F;
                predictor[c] = wav_decode_ima_nibble(predictor[c], &stepIndex[c], nibble);
                pOut[samplesRead++] = predictor[c];
            }
        }
    }
}
static void wav_decode_ms_adpcm_block(wav_decoder* pDec, const wav_uint8* block, wav_int16* pOut) {
    wav_int16 sample1[WAV_MAX_CHANNELS], sample2[WAV_MAX_CHANNELS]; wav_int32 delta[WAV_MAX_CHANNELS]; wav_uint8 predictor[WAV_MAX_CHANNELS]; wav_uint32 offset = 0;
    wav_uint16 channels = pDec->container.channels;
    wav_uint32 blockAlign = pDec->container.blockAlign;
    for (wav_uint16 c = 0; c < channels; ++c) {
        if (offset + 7 > blockAlign) return;
        predictor[c] = block[offset++];
        if (predictor[c] >= pDec->container.msAdpcmNumCoefficients) predictor[c] = 0;
        delta[c] = (wav_int16)(block[offset] | (block[offset+1] << 8)); offset += 2;
        sample1[c] = (wav_int16)(block[offset] | (block[offset+1] << 8)); offset += 2;
        sample2[c] = (wav_int16)(block[offset] | (block[offset+1] << 8)); offset += 2;
        pOut[c] = sample2[c];
        if (channels + c < pDec->container.samplesPerBlock * channels) { pOut[channels + c] = sample1[c]; }
    }
    wav_uint64 samplesRead = channels * 2;
    wav_uint64 totalSamples = (wav_uint64)pDec->container.samplesPerBlock * channels;
    while (samplesRead < totalSamples) {
        for (wav_uint16 c = 0; c < channels && samplesRead < totalSamples; ++c) {
            if (offset >= blockAlign) return;
            wav_uint8 byte = block[offset++];
            for (int n = 0; n < 2 && samplesRead < totalSamples; ++n) {
                wav_uint8 nibble = (n == 0) ? ((byte >> 4) & 0x0F) : (byte & 0x0F);
                wav_int32 coef1 = pDec->container.msAdpcmCoefficients[predictor[c]][0];
                wav_int32 coef2 = pDec->container.msAdpcmCoefficients[predictor[c]][1];
                wav_int32 pred = ((wav_int32)sample1[c] * coef1 + (wav_int32)sample2[c] * coef2) >> 8;
                wav_int32 nibble_signed = (nibble & 0x08) ? (nibble | 0xFFFFFFF0) : nibble;
                wav_int32 sample = pred + (delta[c] * nibble_signed);
                if (sample > 32767) sample = 32767;
                else if (sample < -32768) sample = -32768;
                sample2[c] = sample1[c]; sample1[c] = (wav_int16)sample;
                delta[c] = (delta[c] * MS_ADPCM_ADAPTATION_TABLE[nibble]) >> 8;
                if (delta[c] < 16) delta[c] = 16;
                pOut[samplesRead++] = sample1[c];
            }
        }
    }
}
static wav_uint64 wav_read_and_process_block(wav_decoder* pDec) {
    if (pDec->stream.currentBlockBufferBytesValid == 0) {
        wav_size_t bytesRead = wav_stream_read_pcm_bytes(&pDec->stream, pDec->blockScratchpad, pDec->container.blockAlign);
        if (bytesRead < pDec->container.blockAlign) return 0;
        if (pDec->container.formatTag == WAV_FORMAT_IMA_ADPCM) {
            wav_decode_ima_block(pDec, pDec->blockScratchpad, pDec->sampleScratchpad);
        } else {
            wav_decode_ms_adpcm_block(pDec, pDec->blockScratchpad, pDec->sampleScratchpad);
        }
        pDec->stream.currentBlockBufferBytesValid = 1;
    }
    return pDec->container.samplesPerBlock;
}
wav_uint64 wav_read_pcm_frames_s16(wav_decoder* pDec, wav_uint64 framesToRead, wav_int16* pOut) {
    if (!pDec || !pDec->isInitialized || !pOut || framesToRead == 0 || pDec->container.channels == 0 || framesToRead > WAV_UINT64_MAX / pDec->container.channels) return 0;
    wav_uint64 samplesToRead = framesToRead * pDec->container.channels; wav_uint64 samplesRead = 0;
    if (pDec->container.formatTag == WAV_FORMAT_IMA_ADPCM || pDec->container.formatTag == WAV_FORMAT_MS_ADPCM) {
        while (samplesRead < samplesToRead) {
            wav_uint64 framesInBlock = wav_read_and_process_block(pDec);
            if (framesInBlock == 0) break;
            wav_uint64 startFrameInBlock = pDec->stream.intraBlockSampleOffset;
            if (startFrameInBlock >= framesInBlock) {
                pDec->stream.currentBlockBufferBytesValid = 0;
                pDec->stream.intraBlockSampleOffset = 0;
                continue;
            }
            wav_uint64 framesToCopy = framesInBlock - startFrameInBlock;
            if (samplesRead / pDec->container.channels + framesToCopy > framesToRead) {
                framesToCopy = framesToRead - (samplesRead / pDec->container.channels);
            }
            wav_size_t srcSampleOffset = (wav_size_t)(startFrameInBlock * pDec->container.channels);
            wav_size_t byteOffset = (wav_size_t)(samplesRead * sizeof(wav_int16));
            wav_size_t bytesToCopy = (wav_size_t)(framesToCopy * pDec->container.channels * sizeof(wav_int16));
            wav_memcpy((wav_uint8*)pOut + byteOffset, pDec->sampleScratchpad + srcSampleOffset, bytesToCopy);
            samplesRead += framesToCopy * pDec->container.channels;
            pDec->stream.intraBlockSampleOffset += framesToCopy;
            if (pDec->stream.intraBlockSampleOffset >= framesInBlock) {
                pDec->stream.currentBlockBufferBytesValid = 0;
                pDec->stream.intraBlockSampleOffset = 0;
            }
        }
    } else {
        for (wav_uint64 i = 0; i < samplesToRead; ++i) {
            if (pDec->container.formatTag == WAV_FORMAT_PCM) {
                if (pDec->container.bitsPerSample == 8) {
                    wav_uint8 b; if (!wav_stream_read_u8(&pDec->stream, &b)) break; pOut[i] = (wav_int16)((wav_int16)b - 128) << 8;
                } else if (pDec->container.bitsPerSample == 16) {
                    wav_uint16 b; if (!wav_stream_read_u16(&pDec->stream, &b)) break; pOut[i] = (wav_int16)b;
                } else if (pDec->container.bitsPerSample == 24) {
                    wav_uint8 b0, b1, b2; if (!wav_stream_read_u8(&pDec->stream, &b0) || !wav_stream_read_u8(&pDec->stream, &b1) || !wav_stream_read_u8(&pDec->stream, &b2)) break;
                    wav_int32 val = b0 | (b1 << 8) | (b2 << 16); if (val & 0x800000) val |= ~((wav_int32)0xFFFFFF); pOut[i] = (wav_int16)(val >> 8);
                } else if (pDec->container.bitsPerSample == 32) {
                    wav_uint32 b; if (!wav_stream_read_u32(&pDec->stream, &b)) break; pOut[i] = (wav_int16)(b >> 16);
                }
            }
            samplesRead++;
        }
    }
    return samplesRead / pDec->container.channels;
}
wav_uint64 wav_read_pcm_frames_s32(wav_decoder* pDec, wav_uint64 framesToRead, wav_int32* pOut) {
    if (!pDec || !pDec->isInitialized || !pOut || framesToRead == 0 || pDec->container.channels == 0 || framesToRead > WAV_UINT64_MAX / pDec->container.channels) return 0;
    wav_uint64 samplesToRead = framesToRead * pDec->container.channels; wav_uint64 samplesRead = 0;
    if (pDec->container.formatTag == WAV_FORMAT_IMA_ADPCM || pDec->container.formatTag == WAV_FORMAT_MS_ADPCM) {
        while (samplesRead < samplesToRead) {
            wav_uint64 framesInBlock = wav_read_and_process_block(pDec);
            if (framesInBlock == 0) break;
            wav_uint64 startFrameInBlock = pDec->stream.intraBlockSampleOffset;
            if (startFrameInBlock >= framesInBlock) {
                pDec->stream.currentBlockBufferBytesValid = 0;
                pDec->stream.intraBlockSampleOffset = 0;
                continue;
            }
            wav_uint64 framesToCopy = framesInBlock - startFrameInBlock;
            if (samplesRead / pDec->container.channels + framesToCopy > framesToRead) {
                framesToCopy = framesToRead - (samplesRead / pDec->container.channels);
            }
            wav_size_t srcSampleOffset = (wav_size_t)(startFrameInBlock * pDec->container.channels);
            for (wav_uint64 i = 0; i < framesToCopy * pDec->container.channels; ++i) {
                pOut[samplesRead + i] = (wav_int32)pDec->sampleScratchpad[srcSampleOffset + i] << 16;
            }
            samplesRead += framesToCopy * pDec->container.channels;
            pDec->stream.intraBlockSampleOffset += framesToCopy;
            if (pDec->stream.intraBlockSampleOffset >= framesInBlock) {
                pDec->stream.currentBlockBufferBytesValid = 0;
                pDec->stream.intraBlockSampleOffset = 0;
            }
        }
    } else {
        for (wav_uint64 i = 0; i < samplesToRead; ++i) {
            if (pDec->container.formatTag == WAV_FORMAT_PCM) {
                if (pDec->container.bitsPerSample == 8) {
                    wav_uint8 b; if (!wav_stream_read_u8(&pDec->stream, &b)) break; pOut[i] = ((wav_int32)b - 128) << 24;
                } else if (pDec->container.bitsPerSample == 16) {
                    wav_uint16 b; if (!wav_stream_read_u16(&pDec->stream, &b)) break; pOut[i] = (wav_int32)(wav_int16)b << 16;
                } else if (pDec->container.bitsPerSample == 24) {
                    wav_uint8 b0, b1, b2; if (!wav_stream_read_u8(&pDec->stream, &b0) || !wav_stream_read_u8(&pDec->stream, &b1) || !wav_stream_read_u8(&pDec->stream, &b2)) break;
                    wav_int32 val = b0 | (b1 << 8) | (b2 << 16); if (val & 0x800000) val |= ~((wav_int32)0xFFFFFF); pOut[i] = val << 8;
                } else if (pDec->container.bitsPerSample == 32) {
                    wav_uint32 b; if (!wav_stream_read_u32(&pDec->stream, &b)) break; pOut[i] = (wav_int32)b;
                }
            }
            samplesRead++;
        }
    }
    return samplesRead / pDec->container.channels;
}
wav_result wav_seek_to_pcm_frame(wav_decoder* p, wav_uint64 frameIndex) {
    if (!p || !p->isInitialized) return WAV_INVALID_ARGS;
    wav_uint64 byteOffset = 0;
    if (p->container.formatTag == WAV_FORMAT_IMA_ADPCM || p->container.formatTag == WAV_FORMAT_MS_ADPCM) {
        if (p->container.samplesPerBlock == 0) return WAV_INVALID_ARGS;
        wav_uint64 blockIndex = frameIndex / p->container.samplesPerBlock;
        byteOffset = blockIndex * p->container.blockAlign;
        p->stream.intraBlockSampleOffset = frameIndex % p->container.samplesPerBlock;
    } else {
        byteOffset = frameIndex * p->container.blockAlign;
    }
    if (byteOffset > p->stream.dataChunkSize) return WAV_TRUNCATED;
    p->stream.currentBytePos = p->stream.dataChunkStart + byteOffset;
    p->stream.currentBlockBufferBytesValid = 0;
    return WAV_SUCCESS;
}
