# Huffman Compressor (C++17)

A command-line lossless file compressor using canonical Huffman coding. It
compresses any file, decompresses to a byte-identical copy, and prints the
ratio.

```bash
huffman compress   input.txt  input.huf
huffman decompress input.huf  output.txt
```

## How it works

Count byte frequencies over the file. Build a Huffman tree by repeatedly merging
the two rarest nodes out of a min-heap, then read each byte's code length off its
depth, so frequent bytes end up with short codes.

Then assign canonical codes. Rather than storing the tree, I store just the 256
code lengths and both sides derive identical codes from them by a fixed rule.
The header stays 256 bytes and the decoder stays simple.

Finally bit-pack the codes MSB-first through a
[`BitWriter`](include/huffman/BitIO.h). The decoder walks bits through the
canonical tables to recover each byte.

### File format

```
"HUF1"            4 bytes   magic
originalLength    8 bytes   big-endian (so we stop exactly and ignore bit padding)
codeLengths[256]  256 bytes one length per byte value (0 = unused)
bitstream         ...       the packed codes
```

## Why canonical

A textbook Huffman implementation has to serialise the tree into the file.
Canonical Huffman stores only the code lengths and rebuilds the codes
deterministically, which gives you a smaller header, a simpler decoder, and a
natural table-driven decode. It's what DEFLATE and ZIP do.

## Results

Compressing this repo's own source (mixed C++ text):

```
original   : 10,425 bytes
compressed :  6,659 bytes   (1.57x, 36.1% saved)
round-trip : byte-identical
```

The ratio tracks entropy. Text, logs and anything with a skewed byte
distribution compress well; already-compressed or random data barely moves,
which is what information theory says has to happen.

## Build & run

```bash
cmake -S . -B build
cmake --build build

./build/huffman compress   README.md out.huf
./build/huffman decompress out.huf   back.md
ctest --test-dir build     # 8 tests
```

Needs CMake 3.14+ and a C++17 compiler. GoogleTest is fetched automatically.

## Layout

```
include/huffman/   BitIO.h (bit-level read/write), Huffman.h (public API)
src/               Huffman.cpp (codec + tree + file format), main.cpp (CLI)
tests/             test_huffman.cpp   (GoogleTest)
```

## Edge cases

- Empty file, single byte, and a file of one repeated symbol (which needs a
  forced 1-bit code, otherwise the tree has no depth to read lengths off)
- All 256 byte values present
- Truncated or non-`HUF1` input gets rejected instead of crashing

## Roadmap

- [ ] Run-length plus Huffman, roughly a DEFLATE-lite, for a better ratio
- [ ] Streaming API for files bigger than memory
- [ ] Adaptive Huffman (single pass, no header)
