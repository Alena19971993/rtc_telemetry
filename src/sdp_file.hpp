#pragma once
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

inline void write_file(const std::string& path, const std::string& content) {
    try {
        std::ofstream f(path);
        f.exceptions(std::ios::failbit | std::ios::badbit);
        f << content;
    } catch (const std::ios_base::failure& e) {
        std::cout << "Failed to create file: " << path << " — " << e.what() << std::endl;
    }
}

inline std::optional<std::string> read_file(const std::string& path) {
    try {
        std::ifstream f(path);
        f.exceptions(std::ios::failbit | std::ios::badbit);
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    } catch (const std::ios_base::failure& e) {
        std::cout << "Failed to read file: " << path << " — " << e.what() << std::endl;
        return std::nullopt;
    }
}

inline bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}