#include "huffman/Huffman.h"

#include <cstdio>
#include <string>
#include <sys/stat.h>

static long fileSize(const std::string& path) {
    struct stat st{};
    return (stat(path.c_str(), &st) == 0) ? static_cast<long>(st.st_size) : -1;
}

static void usage() {
    std::printf("usage:\n"
                "  huffman compress   <input> <output>\n"
                "  huffman decompress <input> <output>\n");
}

int main(int argc, char** argv) {
    if (argc != 4) { usage(); return 2; }
    const std::string mode = argv[1], in = argv[2], out = argv[3];

    if (mode == "compress") {
        if (!huff::compressFile(in, out)) { std::printf("error: could not compress %s\n", in.c_str()); return 1; }
        const long a = fileSize(in), b = fileSize(out);
        std::printf("compressed %s (%ld B) -> %s (%ld B)   ratio %.2fx, saved %.1f%%\n",
                    in.c_str(), a, out.c_str(), b,
                    b > 0 ? static_cast<double>(a) / b : 0.0,
                    a > 0 ? 100.0 * (1.0 - static_cast<double>(b) / a) : 0.0);
    } else if (mode == "decompress") {
        if (!huff::decompressFile(in, out)) { std::printf("error: could not decompress %s\n", in.c_str()); return 1; }
        std::printf("decompressed %s -> %s (%ld B)\n", in.c_str(), out.c_str(), fileSize(out));
    } else {
        usage();
        return 2;
    }
    return 0;
}
