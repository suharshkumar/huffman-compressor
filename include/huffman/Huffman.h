#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace huff {

// Compress / decompress a byte buffer with canonical Huffman coding.
// decompress(compress(x)) == x for every input, including empty and single-byte.
std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
std::vector<uint8_t> decompress(const std::vector<uint8_t>& packed);

// File wrappers around the buffer functions. Return true on success.
bool compressFile(const std::string& inPath, const std::string& outPath);
bool decompressFile(const std::string& inPath, const std::string& outPath);

} // namespace huff
