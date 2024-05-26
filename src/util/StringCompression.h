#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

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
}