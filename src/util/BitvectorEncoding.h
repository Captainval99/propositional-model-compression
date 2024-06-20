#ifndef SRC_UTIL_BITVECTORENCODING_H
#define SRC_UTIL_BITVECTORENCODING_H

#include <vector>
#include <string>

namespace BitvectorEncoding {

    std::vector<uint32_t> plainBitvector(std::vector<bool> bitvector) {
        std::string output;

        for (bool value: bitvector) {
            output.append(std::to_string(value));
        }

        return output;
    }

    std::vector<uint32_t> diffEncoding(std::vector<bool> bitvetor) {
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