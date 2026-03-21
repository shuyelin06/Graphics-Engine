#pragma once

#include <array>
#include <cstring>

namespace Engine {
// String Class that is allocated on the stack.
template <size_t MAX_SIZE> class StackString {
    std::array<char, MAX_SIZE> buffer;

  public:
    StackString() { memset(buffer.data(), 0, buffer.size()); }
    template <std::size_t N> StackString(const char (&str)[N]) {
        static_assert(N <= MAX_SIZE, "String literal too large");
        std::memcpy(buffer.data(), str, N); // includes null terminator
    }

    template <std::size_t N> StackString& operator=(const char (&str)[N]) {
        static_assert(N <= MAX_SIZE, "String literal too large");
        std::memcpy(buffer.data(), str, N); // includes null terminator
        return *this;
    }
    template <std::size_t N> bool operator==(const char (&str)[N]) const {
        constexpr size_t MIN_SIZE = N < MAX_SIZE ? N : MAX_SIZE;
        return strncmp(buffer.data(), str, MIN_SIZE);
    }

    const char* c_str() const { return buffer.data(); }
};
} // namespace Engine