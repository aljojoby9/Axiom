#pragma once

/**
 * @file io.hpp
 * @brief IO module for Axiom standard library
 * 
 * Provides file operations, console IO, and formatting.
 */

#include "core.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace axiom {
namespace stdlib {
namespace io {

/**
 * @brief Print to stdout
 */
template<typename T>
void print(const T& value) {
    std::cout << value << std::endl;
}

template<typename T, typename... Args>
void print(const T& first, const Args&... rest) {
    std::cout << first << " ";
    print(rest...);
}

/**
 * @brief Print without newline
 */
template<typename T>
void write(const T& value) {
    std::cout << value;
}

/**
 * @brief Print to stderr
 */
template<typename T>
void eprint(const T& value) {
    std::cerr << value << std::endl;
}

/**
 * @brief Read line from stdin
 */
inline String input(const String& prompt = "") {
    if (!prompt.empty()) {
        std::cout << prompt.to_std();
    }
    std::string line;
    std::getline(std::cin, line);
    return String(line);
}

/**
 * @brief File handle for reading and writing
 */
class File {
public:
    enum class Mode {
        Read,
        Write,
        Append,
        ReadWrite
    };
    
    File() = default;
    
    static Result<File, String> open(const String& path, Mode mode = Mode::Read) {
        File file;
        file.path_ = path;
        file.mode_ = mode;
        
        std::ios_base::openmode flags = std::ios_base::in;
        switch (mode) {
            case Mode::Read:
                flags = std::ios_base::in;
                break;
            case Mode::Write:
                flags = std::ios_base::out | std::ios_base::trunc;
                break;
            case Mode::Append:
                flags = std::ios_base::out | std::ios_base::app;
                break;
            case Mode::ReadWrite:
                flags = std::ios_base::in | std::ios_base::out;
                break;
        }
        
        file.stream_.open(path.to_std(), flags);
        if (!file.stream_.is_open()) {
            return Result<File, String>::err(String("Failed to open file: ") + path);
        }
        
        return Result<File, String>::ok(std::move(file));
    }
    
    bool is_open() const { return stream_.is_open(); }
    
    void close() { stream_.close(); }
    
    /**
     * @brief Read entire file content
     */
    Result<String, String> read() {
        if (!stream_.is_open()) {
            return Result<String, String>::err("File not open");
        }
        std::stringstream buffer;
        buffer << stream_.rdbuf();
        return Result<String, String>::ok(String(buffer.str()));
    }
    
    /**
     * @brief Read single line
     */
    Option<String> readline() {
        if (!stream_.is_open() || stream_.eof()) {
            return Option<String>::none();
        }
        std::string line;
        if (std::getline(stream_, line)) {
            return Option<String>::some(String(line));
        }
        return Option<String>::none();
    }
    
    /**
     * @brief Read all lines
     */
    List<String> readlines() {
        List<String> lines;
        std::string line;
        while (std::getline(stream_, line)) {
            lines.append(String(line));
        }
        return lines;
    }
    
    /**
     * @brief Write string to file
     */
    Result<Unit, String> write(const String& content) {
        if (!stream_.is_open()) {
            return Result<Unit, String>::err("File not open");
        }
        stream_ << content.to_std();
        return Result<Unit, String>::ok(Unit{});
    }
    
    /**
     * @brief Write line to file
     */
    Result<Unit, String> writeline(const String& line) {
        if (!stream_.is_open()) {
            return Result<Unit, String>::err("File not open");
        }
        stream_ << line.to_std() << "\n";
        return Result<Unit, String>::ok(Unit{});
    }

private:
    String path_;
    Mode mode_ = Mode::Read;
    std::fstream stream_;
};

/**
 * @brief Read entire file content
 */
inline Result<String, String> read_file(const String& path) {
    auto result = File::open(path, File::Mode::Read);
    if (result.is_err()) {
        return Result<String, String>::err(result.unwrap_err());
    }
    return result.unwrap().read();
}

/**
 * @brief Write content to file
 */
inline Result<Unit, String> write_file(const String& path, const String& content) {
    auto result = File::open(path, File::Mode::Write);
    if (result.is_err()) {
        return Result<Unit, String>::err(result.unwrap_err());
    }
    return result.unwrap().write(content);
}

/**
 * @brief Append content to file
 */
inline Result<Unit, String> append_file(const String& path, const String& content) {
    auto result = File::open(path, File::Mode::Append);
    if (result.is_err()) {
        return Result<Unit, String>::err(result.unwrap_err());
    }
    return result.unwrap().write(content);
}

/**
 * @brief Check if file exists
 */
inline bool exists(const String& path) {
    std::ifstream f(path.to_std());
    return f.good();
}

// Format implementation helpers (declared before format)
inline void format_impl(std::ostringstream& ss, const std::string& fmt) {
    ss << fmt;
}

template<typename T, typename... Args>
void format_impl(std::ostringstream& ss, const std::string& fmt, 
                 const T& value, const Args&... rest) {
    size_t pos = fmt.find("{}");
    if (pos != std::string::npos) {
        ss << fmt.substr(0, pos) << value;
        format_impl(ss, fmt.substr(pos + 2), rest...);
    } else {
        ss << fmt;
    }
}

/**
 * @brief Format string with arguments (simple printf-like)
 */
template<typename... Args>
String format(const String& fmt, const Args&... args) {
    std::ostringstream ss;
    format_impl(ss, fmt.to_std(), args...);
    return String(ss.str());
}

} // namespace io
} // namespace stdlib
} // namespace axiom
