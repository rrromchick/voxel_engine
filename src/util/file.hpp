#pragma once

#include "std.hpp"
#include "typedefs.hpp"

namespace file {
    bool write_binary_file(
        std::string filename, const char *data, usize size) {
        std::ofstream output(filename, std::ios::binary);
        if (!output.is_open()) {
            return false;
        }

        output.write(data, size);
        output.close();
        return true;
    }

    bool read_binary_file(std::string filename, char *data, usize size) {
        std::ifstream output(filename, std::ios::binary);
        if (!output.is_open()) {
            return false;
        }
       
        output.read(data, size);
        output.close();
        return true;
    }
}