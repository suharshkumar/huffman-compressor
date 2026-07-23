#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace huff {

// Packs individual bits MSB-first into a growing byte buffer. The final partial
// byte is zero-padded on flush().
class BitWriter {
public:
    void writeBit(int bit) {
        current_ = static_cast<uint8_t>((current_ << 1) | (bit & 1));
        if (++nbits_ == 8) {
            out_.push_back(current_);
            current_ = 0;
            nbits_ = 0;
        }
    }

    // Write the low `len` bits of `code`, most-significant bit first.
    void writeBits(uint32_t code, int len) {
        for (int i = len - 1; i >= 0; --i)
            writeBit(static_cast<int>((code >> i) & 1));
    }

    void flush() {
        if (nbits_ > 0) {
            current_ = static_cast<uint8_t>(current_ << (8 - nbits_));  // pad with zeros
            out_.push_back(current_);
            current_ = 0;
            nbits_ = 0;
        }
    }

    const std::vector<uint8_t>& bytes() const { return out_; }

private:
    std::vector<uint8_t> out_;
    uint8_t current_ = 0;
    int     nbits_   = 0;
};

// Reads bits MSB-first from a byte buffer. readBit() returns -1 past the end.
class BitReader {
public:
    BitReader(const uint8_t* data, std::size_t size) : data_(data), size_(size) {}

    int readBit() {
        if (bytePos_ >= size_) return -1;
        const int bit = (data_[bytePos_] >> (7 - bitPos_)) & 1;
        if (++bitPos_ == 8) { bitPos_ = 0; ++bytePos_; }
        return bit;
    }

private:
    const uint8_t* data_;
    std::size_t    size_;
    std::size_t    bytePos_ = 0;
    int            bitPos_  = 0;
};

} // namespace huff
