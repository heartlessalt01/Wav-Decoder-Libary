# WAV Decoder Library

A lightweight, freestanding-friendly WAV decoder written in C.

Designed for:
- Bare-metal systems
- Operating system kernels
- Bootloaders
- Embedded projects
- Normal desktop applications

No dependency on libc is required. The decoder provides its own memory helpers and uses a custom I/O backend system.

---

## Features

- RIFF WAV parsing
- RIFX big-endian WAV support
- RF64 WAV support
- Memory loading support
- Custom streaming I/O support
- PCM decoding:
  - 8-bit
  - 16-bit
  - 24-bit
  - 32-bit
- IMA ADPCM decoding
- Microsoft ADPCM decoding
- WAV extensible format support
- Frame seeking
- 16-bit PCM output
- 32-bit PCM output

Supported WAV formats:

| Format | Support |
|-|-|
| PCM | ✅ |
| IMA ADPCM | ✅ |
| MS ADPCM | ✅ |
| WAVE_FORMAT_EXTENSIBLE PCM | ✅ |
| MP3 | ❌ |
| OGG | ❌ |

---

# Files
