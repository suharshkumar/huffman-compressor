#include "huffman/Huffman.h"

#include "huffman/BitIO.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <queue>

namespace huff {

namespace {

constexpr char        kMagic[4] = {'H', 'U', 'F', '1'};
constexpr std::size_t kHeader   = 4 + 8 + 256;   // magic + origLen + 256 code lengths

// --- Huffman tree -> per-symbol code LENGTHS --------------------------------
// We only need the code *lengths*; canonical coding derives the actual bits.
std::array<uint8_t, 256> codeLengths(const std::array<uint64_t, 256>& freq) {
    struct Node { uint64_t freq; int order; int sym; int left; int right; };
    std::vector<Node> nodes;

    int distinct = 0;
    for (int s = 0; s < 256; ++s)
        if (freq[s] > 0) { nodes.push_back({freq[s], distinct++, s, -1, -1}); }

    std::array<uint8_t, 256> lengths{};
    if (distinct == 0) return lengths;                 // empty input
    if (distinct == 1) { lengths[nodes[0].sym] = 1; return lengths; }  // one symbol -> 1 bit

    // Min-heap on (freq, order); `order` breaks ties deterministically.
    auto cmp = [&](int a, int b) {
        if (nodes[a].freq != nodes[b].freq) return nodes[a].freq > nodes[b].freq;
        return nodes[a].order > nodes[b].order;
    };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> pq(cmp);
    for (int i = 0; i < distinct; ++i) pq.push(i);

    int nextOrder = distinct;
    while (pq.size() > 1) {
        const int a = pq.top(); pq.pop();
        const int b = pq.top(); pq.pop();
        nodes.push_back({nodes[a].freq + nodes[b].freq, nextOrder++, -1, a, b});
        pq.push(static_cast<int>(nodes.size()) - 1);
    }

    // Walk the tree with an explicit stack to record each leaf's depth.
    std::vector<std::pair<int, int>> stack{{pq.top(), 0}};
    while (!stack.empty()) {
        const auto [idx, depth] = stack.back();
        stack.pop_back();
        const Node& n = nodes[idx];
        if (n.sym >= 0) lengths[n.sym] = static_cast<uint8_t>(depth);
        else {
            stack.push_back({n.left, depth + 1});
            stack.push_back({n.right, depth + 1});
        }
    }
    return lengths;
}

// --- Canonical codes from lengths (encoder side) ----------------------------
void canonicalCodes(const std::array<uint8_t, 256>& len,
                    std::array<uint32_t, 256>& code) {
    std::vector<int> syms;
    for (int s = 0; s < 256; ++s)
        if (len[s] > 0) syms.push_back(s);
    std::sort(syms.begin(), syms.end(),
              [&](int a, int b) { return len[a] != len[b] ? len[a] < len[b] : a < b; });

    uint32_t c = 0;
    int prevLen = len[syms.front()];
    for (int s : syms) {
        c <<= (len[s] - prevLen);   // shift when moving to a longer length
        prevLen = len[s];
        code[s] = c++;
    }
}

void putU64(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 7; i >= 0; --i) out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
}
uint64_t getU64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    return v;
}

} // namespace

std::vector<uint8_t> compress(const std::vector<uint8_t>& input) {
    std::array<uint64_t, 256> freq{};
    for (uint8_t b : input) ++freq[b];

    const std::array<uint8_t, 256> len = codeLengths(freq);
    std::array<uint32_t, 256> code{};
    if (!input.empty()) canonicalCodes(len, code);

    std::vector<uint8_t> out;
    out.reserve(kHeader + input.size());
    out.insert(out.end(), kMagic, kMagic + 4);
    putU64(out, input.size());
    out.insert(out.end(), len.begin(), len.end());

    BitWriter bw;
    for (uint8_t b : input) bw.writeBits(code[b], len[b]);
    bw.flush();
    out.insert(out.end(), bw.bytes().begin(), bw.bytes().end());
    return out;
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& packed) {
    if (packed.size() < kHeader || std::memcmp(packed.data(), kMagic, 4) != 0)
        return {};                                     // not our format

    const uint64_t origLen = getU64(packed.data() + 4);
    std::array<uint8_t, 256> len{};
    std::memcpy(len.data(), packed.data() + 12, 256);

    std::vector<uint8_t> out;
    out.reserve(origLen);
    if (origLen == 0) return out;

    // Rebuild the canonical decoding tables from the code lengths alone.
    int maxLen = 0;
    for (int s = 0; s < 256; ++s) maxLen = std::max(maxLen, static_cast<int>(len[s]));

    std::vector<int> countByLen(maxLen + 1, 0);
    for (int s = 0; s < 256; ++s)
        if (len[s] > 0) ++countByLen[len[s]];

    std::vector<int> syms;
    for (int s = 0; s < 256; ++s)
        if (len[s] > 0) syms.push_back(s);
    std::sort(syms.begin(), syms.end(),
              [&](int a, int b) { return len[a] != len[b] ? len[a] < len[b] : a < b; });

    std::vector<uint32_t> firstCode(maxLen + 2, 0);
    std::vector<int>      firstIndex(maxLen + 2, 0);
    uint32_t c = 0;
    int idx = 0;
    for (int l = 1; l <= maxLen; ++l) {
        firstCode[l]  = c;
        firstIndex[l] = idx;
        idx += countByLen[l];
        c = (c + countByLen[l]) << 1;
    }

    BitReader br(packed.data() + kHeader, packed.size() - kHeader);
    uint32_t acc = 0;
    int curLen = 0;
    while (out.size() < origLen) {
        const int bit = br.readBit();
        if (bit < 0) break;                            // truncated input
        acc = (acc << 1) | static_cast<uint32_t>(bit);
        ++curLen;
        if (curLen <= maxLen && countByLen[curLen] > 0) {
            const uint32_t offset = acc - firstCode[curLen];
            if (offset < static_cast<uint32_t>(countByLen[curLen])) {
                out.push_back(static_cast<uint8_t>(syms[firstIndex[curLen] + offset]));
                acc = 0;
                curLen = 0;
            }
        }
    }
    return out;
}

// --- File wrappers ----------------------------------------------------------
namespace {
bool readFile(const std::string& path, std::vector<uint8_t>& buf) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    buf.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}
bool writeFile(const std::string& path, const std::vector<uint8_t>& buf) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    return static_cast<bool>(out);
}
} // namespace

bool compressFile(const std::string& inPath, const std::string& outPath) {
    std::vector<uint8_t> in;
    return readFile(inPath, in) && writeFile(outPath, compress(in));
}

bool decompressFile(const std::string& inPath, const std::string& outPath) {
    std::vector<uint8_t> in;
    return readFile(inPath, in) && writeFile(outPath, decompress(in));
}

} // namespace huff
