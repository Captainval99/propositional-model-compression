#ifndef SRC_UTIL_BITVECTORENCODING_H
#define SRC_UTIL_BITVECTORENCODING_H

#include <vector>
#include <string>

namespace BitvectorEncoding {

    std::vector<uint32_t> plainBitvector(std::vector<bool> bitvector) {
        std::vector<uint32_t> output;

        for (bool value: bitvector) {
            output.push_back(value);
        }

        return output;
    }

    std::vector<uint32_t> diffEncoding(std::vector<bool> bitvetor) {
        //add a false to the bitvector to make it possible to differntiate between the different sectors of the compressed file
        bitvetor.push_back(false);

        std::vector<uint32_t> output;
        uint32_t currentDistance = 0;

        for (bool value: bitvetor) {
            if (value) {
                currentDistance += 1;
            } else {
                output.push_back(currentDistance);
                currentDistance = 0;
            }
        }

        return output;
    }
}


#endif