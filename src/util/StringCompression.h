#ifndef SRC_UTIL_STRINGCOMPRESSION_H
#define SRC_UTIL_STRINGCOMPRESSION_H

#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <lz4.h>

namespace StringCompression {

    std::string compressString(std::string input) {
        std::stringstream original;
        std::stringstream compressed;
        original << input;
        boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
        out.push(boost::iostreams::zlib_compressor());
        out.push(original);
        boost::iostreams::copy(out, compressed);
        return compressed.str();
    }

    std::string decompressString(std::string input) {
        std::stringstream compressed;
        std::stringstream decompressed;
        compressed << input;
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::zlib_decompressor());
        in.push(compressed);
        boost::iostreams::copy(in, decompressed);
        return decompressed.str();
    }

    std::string lz4Compression(std::string input) {
        int maxCompressionSize = LZ4_compressBound(input.size());
        char* compressed = new char[maxCompressionSize];
        
        int compressedSize = LZ4_compress_default(input.c_str(), compressed, input.size(), maxCompressionSize);

        std::string compressedString(compressed, compressed + compressedSize);
        delete compressed;

        return compressedString;
    }

    std::string lz4Decompression(std::string input, int uncompressedSize) {
        char* decompressed = new char[uncompressedSize];

        int decompressionSize = LZ4_decompress_safe(input.c_str(), decompressed, input.size(), uncompressedSize);

        std::string decompressedString(decompressed, decompressed + decompressionSize);
        delete decompressed;

        return decompressedString;
    }
}

#endif