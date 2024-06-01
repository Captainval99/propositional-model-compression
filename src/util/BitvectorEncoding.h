#ifndef SRC_UTIL_BITVECTORENCODING_H
#define SRC_UTIL_BITVECTORENCODING_H

#include <vector>
#include <string>

namespace BitvectorEncoding {

    std::string plainBitvector(std::vector<bool> bitvector) {
        std::string output;

        for (bool value: bitvector) {
            output.append(std::to_string(value));
        }

        return output;
    }

    std::string diffEncoding(std::vector<bool> bitvetor) {
        std::string output;
        uint64_t currentDistance = 0;

        for (bool value: bitvetor) {
            if (value) {
                currentDistance += 1;
            } else {
                output.append(std::to_string(currentDistance));
                output.append(" "); 
                currentDistance = 0;
            }
        }

        //remove last whitespace character
        if (output.length() > 0) {
            output.pop_back();
        }

        return output;
    }
}


#endif