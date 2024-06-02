#ifndef SRC_UTIL_STRINGCOMPRESSION_H
#define SRC_UTIL_STRINGCOMPRESSION_H

#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <lz4.h>

namespace StringCompression {
    const unsigned int GOLOMB_RICE_PARAM = 4;

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

    std::vector<char> golombRiceCompression(std::vector<uint32_t> input) {
        uint32_t moduloBitMask = UINT32_MAX >> (32 - GOLOMB_RICE_PARAM);
        std::vector<char> output;
        uint32_t offset = 0;
        uint8_t currentByte = 0;

        for (uint32_t i: input) {
            uint32_t q = i >> GOLOMB_RICE_PARAM;
            uint32_t r = i & moduloBitMask;

            // write q
            if ((offset + q + 1) < 8) {
                currentByte = currentByte | ((0xFF >> (8 - q)) << (8 - (offset + q)));
                offset += q + 1;

            } else if ((offset + q + 1) == 8) {
                currentByte = currentByte | ((0xFF >> (offset + 1)) << 1);
                output.push_back(currentByte);
                //std::cout << "pushed byte: " << unsigned(currentByte) << std::endl;
                currentByte = 0;
                offset = 0;
            } else {
                currentByte = currentByte | (0xFF >> offset);
                output.push_back(currentByte);
                q -= (8 - offset);

                for (uint32_t j = 0; j < (q >> 3); j++) {
                    output.push_back(0xFF);
                }

                currentByte = 0xFF << (8 - (q % 8));
                offset = (q % 8) + 1;
            }
            
            //write r
            if (offset + GOLOMB_RICE_PARAM < 8) {
                currentByte = currentByte | (r << (8 - offset - GOLOMB_RICE_PARAM));
                offset += GOLOMB_RICE_PARAM;
            } else {
                currentByte = currentByte | (r >> (GOLOMB_RICE_PARAM - offset));
                output.push_back(currentByte);
                //std::cout << "pushed byte: " << unsigned(currentByte) << std::endl;
                uint32_t remainingLength = GOLOMB_RICE_PARAM - (8 - offset);

                uint32_t fullBytes = remainingLength >> 3; // remainingLenth / 8
                uint32_t remainder = remainingLength % 8;

                for (uint32_t j = fullBytes; j > 0; j--) {
                    uint8_t nextByte = r >> (remainder + ((j - 1) * 8));
                    output.push_back(nextByte);
                }

                currentByte = r << (8 - remainder);
                offset = remainder;
            }
        }
        output.push_back(currentByte);
        return output;
    }
}

#endif