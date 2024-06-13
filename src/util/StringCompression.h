#ifndef SRC_UTIL_STRINGCOMPRESSION_H
#define SRC_UTIL_STRINGCOMPRESSION_H

#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <lz4.h>
#include <math.h>

namespace StringCompression {
    const unsigned int GOLOMB_RICE_PARAM = 2;

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
                currentByte = currentByte | (r >> (GOLOMB_RICE_PARAM - (8 - offset)));
                output.push_back(currentByte);
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
        //set all unused bits in the last byte to 1
        uint8_t lastByte = currentByte | (0xFF >> offset);
        output.push_back(lastByte);
        return output;
    }

    std::string golombRiceDecompression(std::string input) {
        const char* inputArray = input.c_str();
        uint32_t head = 0;
        std::string output;

        bool calculateQ;
        uint32_t currentQ = 0;
        uint32_t currentR = 0;
        uint8_t offset = 0;
        uint8_t offsetR = 0; //is needed to count how much of the remainder r was already read

        while (head < input.length()) {
            uint8_t currentByte = inputArray[head];

            if (calculateQ) {
                //fill the already processed bits with 1s and invert the byte
                currentByte = ~(currentByte | (0xFF << (8 - offset)));
                //find the most left unset bit uising the inverted byte
                int position = 0;
                while (currentByte > 0) {
                    currentByte = currentByte >> 1;
                    position += 1;
                }
                //update the Q value and check if a new byte has to be loaded
                currentQ += 8 - position - offset;

                if (position <= 1) {
                    head += 1;
                    offset = 0;

                    if (position == 1) {
                        calculateQ = false;
                    }
                } else {
                    offset = 8 - position + 1;
                    calculateQ = false;
                }
                
            } else {
                uint32_t lengthR = GOLOMB_RICE_PARAM - offsetR;
                //fill the already processed bits with 0s
                currentByte = currentByte & (0xFF >> offset);
                //check if r lies completely in the current byte
                if ((offset + lengthR) <= 8) {
                    //shift to the right so that only the relevant bits remain
                    currentByte = currentByte >> (8 - (offset + lengthR));
                    //add the bits to the current r value
                    currentR = (currentR << lengthR) | currentByte;
                    //update the offsets
                    offsetR = 0;
                    calculateQ = true;
                    if((offset + lengthR) == 8) {
                        offset = 0;
                        head += 1;
                    } else {
                        offset += lengthR;
                    }
                    //calculate and push the final value
                    uint32_t finalValue = currentQ * std::pow(2, GOLOMB_RICE_PARAM) + currentR;
                    output.append(std::to_string(finalValue));
                    output.append(" ");
                    currentQ = 0;
                    currentR = 0;
                } else {
                    //r value has also to be read from the next byte
                    uint8_t lengthR = 8 - offset;
                    offsetR += lengthR;
                    //add the relevant bits to the current value
                    currentR = (currentR << lengthR) | currentByte;
                    //update offsets
                    offset = 0;
                    head += 1;
                }
            }
        }
        output.pop_back();
        return output;
    }
}

#endif