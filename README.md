# WAV Decoder Library

A lightweight WAV decoder written in C.

Designed for:

- Bare-metal systems
- Operating system kernels
- Bootloaders
- Embedded systems
- Desktop applications

The library does not require libc and includes its own basic memory functions.

It supports both memory-based decoding and custom streaming I/O.

---

# Features

- RIFF WAV support
- RIFX big-endian WAV support
- RF64 WAV support
- Memory WAV loading
- Custom file/device streaming
- PCM decoding
  - 8-bit
  - 16-bit
  - 24-bit
  - 32-bit
- IMA ADPCM decoding
- Microsoft ADPCM decoding
- WAVE_FORMAT_EXTENSIBLE PCM
- PCM frame reading
- PCM frame seeking
- 16-bit output
- 32-bit output

---

# Supported Formats

| Format | Support |
|---|---|
| PCM | ✅ |
| IMA ADPCM | ✅ |
| MS ADPCM | ✅ |
| WAVE_FORMAT_EXTENSIBLE | ✅ |
| MP3 | ❌ |
| OGG | ❌ |

---

# Files

```
bootsound.wav
wav_decoder.h
wav_decoder.c
play_wav.exe
README.md
LICENSE
```

---

# Building

Example:

```bash
gcc wav_decoder.c play_wav.c -o play_wav.exe
```

For kernel or bare-metal projects:

Add:

```
wav_decoder.c
wav_decoder.h
```

to your build system.

No standard library is required.

---

#Note when compiling use MSYS UCRT64

# Basic Usage

Include the header:

```c
#include "wav_decoder.h"
```

---

# Opening WAV From Memory

Use `wav_open_memory()` when the entire WAV file is already loaded.

Example:

```c
#include "wav_decoder.h"


wav_decoder decoder;


unsigned char* wavData;
unsigned long wavSize;


/*
    Load WAV file into memory here

    wavData = file buffer
    wavSize = file size
*/


wav_result result =
wav_open_memory(
    &decoder,
    wavData,
    wavSize
);


if (result != WAV_SUCCESS)
{
    // Failed to open WAV
    return;
}
```

---

# Reading PCM Audio

## 16-bit Samples

```c
wav_int16 samples[4096];


wav_uint64 framesRead =
wav_read_pcm_frames_s16(
    &decoder,
    1024,
    samples
);
```

The output is interleaved.

Example stereo:

```
Left
Right
Left
Right
...
```

---

## 32-bit Samples

```c
wav_int32 samples[4096];


wav_uint64 framesRead =
wav_read_pcm_frames_s32(
    &decoder,
    1024,
    samples
);
```

---

# Custom Streaming I/O

The decoder supports reading from custom storage.

Useful for:

- FAT32 drivers
- HDD/SSD drivers
- ROM storage
- Embedded devices
- Kernel file systems


Example:

```c
wav_bool my_reader(
    void* userData,
    wav_uint64 offset,
    void* buffer,
    wav_size_t bytesToRead
)
{

    /*
        Read bytes here
        from your storage device
    */


    return WAV_TRUE;
}



wav_io io;


io.userData = myDevice;
io.read = my_reader;



wav_decoder decoder;


wav_result result =
wav_init_io(
    &decoder,
    io,
    totalStorageSize
);
```

---

# Seeking

Move to a specific PCM frame:

```c
wav_result result =
wav_seek_to_pcm_frame(
    &decoder,
    44100
);
```

Example:

```
44100 frames = approximately 1 second at 44100Hz
```

---

# Cleanup

When finished:

```c
wav_uninit(
    &decoder
);
```

---

# Complete Example

```c
#include "wav_decoder.h"


void decode_audio(
    const void* wav,
    wav_uint64 size
)
{

    wav_decoder decoder;


    if (wav_open_memory(
            &decoder,
            wav,
            size
        ) != WAV_SUCCESS)
    {
        return;
    }



    wav_int16 buffer[4096];



    while (1)
    {

        wav_uint64 frames =
        wav_read_pcm_frames_s16(
            &decoder,
            1024,
            buffer
        );


        if (frames == 0)
            break;



        /*
            Send PCM samples
            to your audio device here
        */
    }



    wav_uninit(&decoder);
}
```

---

# Example Player

This project includes:

```
play_wav.exe
```

Usage:

```bash
play_wav.exe music.wav
```

The player:

1. Opens the WAV file
2. Initializes the decoder
3. Decodes PCM samples
4. Sends audio to the output device

---

# Kernel / Bare Metal Usage

The decoder is designed to work without an operating system.

Example uses:

- Kernel audio drivers
- UEFI applications
- Custom file systems
- Embedded firmware


A storage backend can be connected using:

```c
wav_io
```

allowing the decoder to read from any device.

---

# API Reference

## Initialization

```c
wav_result wav_open_memory(
    wav_decoder* pDecoder,
    const void* data,
    wav_uint64 size
);
```

Open WAV data stored in memory.


---

```c
wav_result wav_init_io(
    wav_decoder* pDecoder,
    wav_io ioBackend,
    wav_uint64 totalStorageSize
);
```

Initialize decoder with custom streaming input.

---

## Reading

```c
wav_uint64 wav_read_pcm_frames_s16(
    wav_decoder* pDecoder,
    wav_uint64 framesToRead,
    wav_int16* pOut
);
```

Reads signed 16-bit PCM frames.


---

```c
wav_uint64 wav_read_pcm_frames_s32(
    wav_decoder* pDecoder,
    wav_uint64 framesToRead,
    wav_int32* pOut
);
```

Reads signed 32-bit PCM frames.

---

## Seeking

```c
wav_result wav_seek_to_pcm_frame(
    wav_decoder* pDecoder,
    wav_uint64 frameIndex
);
```

Moves playback position.

---

## Shutdown

```c
void wav_uninit(
    wav_decoder* pDecoder
);
```

Releases decoder state.

---

# License

Copyright © 2026 Heartless

All rights reserved.

Permission is granted to use, copy, and modify this software for:

- Personal projects
- Educational projects
- Research projects
- Private non-commercial use


Commercial use requires explicit written permission from the author.

Commercial use includes:

- Selling software containing this library
- Including this library in paid products
- Using this library in commercial services
- Redistributing this library commercially


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

Author:

Heartless

Year:

2026
