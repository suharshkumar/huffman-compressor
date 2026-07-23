# Huffman Compressor (C++17)

A command-line **lossless file compressor** built on **canonical Huffman coding**.
Compresses any file, decompresses to a byte-identical copy, and reports the ratio.

```bash
huffman compress   input.txt  input.huf
huffman decompress input.huf  output.txt
```

## How it works

1. **Count** byte frequencies across the file.
2. **Build a Huffman tree** (a min-heap merges the two rarest nodes repeatedly), and
   read off each byte's **code length** from its depth — frequent bytes get short codes.
3. **Assign canonical codes.** Instead of storing the whole tree, we store just the
   256 code *lengths*; both sides derive the identical codes from them by a fixed rule.
   That keeps the header tiny (256 bytes) and decoding simple.
4. **Bit-pack** the codes MSB-first via a [`BitWriter`](include/huffman/BitIO.h);
   the decoder walks bits through the canonical tables to recover each byte.

### File format

```
"HUF1"            4 bytes   magic
originalLength    8 bytes   big-endian (lets us stop exactly, ignoring bit padding)
codeLengths[256]  256 bytes one length per possible byte value (0 = unused)
bitstream         ...       the packed codes
```

## Why canonical Huffman

A classic Huffman implementation must serialise the tree; canonical Huffman stores
only the code lengths and reconstructs the codes deterministically. Smaller header,
simpler decoder, and a natural table-driven decode — the same idea DEFLATE/ZIP use.

## Results

Compressing this repo's own source (mixed C++ text):

```
original   : 10,425 bytes
compressed :  6,659 bytes   (1.57x, 36.1% saved)
round-trip : byte-identical
```

Ratios depend on entropy: low-entropy data (logs, text, skewed distributions)
compresses well; already-random/compressed data barely moves — as information
theory says it must.

## Build & run

```bash
cmake -S . -B build
cmake --build build

./build/huffman compress   README.md out.huf
./build/huffman decompress out.huf   back.md
ctest --test-dir build     # 8 tests: round-trip (empty/single/random/all-bytes) + bit I/O
```

Requires CMake ≥ 3.14 and a C++17 compiler. GoogleTest is fetched automatically.

## Project layout

```
include/huffman/   BitIO.h (bit-level read/write), Huffman.h (public API)
src/               Huffman.cpp (codec + tree + file format), main.cpp (CLI)
tests/             test_huffman.cpp   (GoogleTest)
```

## Edge cases handled

- Empty file, single byte, a file of one repeated symbol (forced 1-bit code)
- All 256 byte values present
- Truncated / non-`HUF1` input is rejected rather than crashing

## Roadmap

- [ ] Run-length + Huffman (a DEFLATE-lite) for a better ratio
- [ ] Streaming API for files larger than memory
- [ ] Adaptive Huffman (single pass, no header)
