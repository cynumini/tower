// #pragma once

// #include "math.cpp"
// #include "types.cpp"
// #include <initializer_list>
// #include <stdlib.h>

// namespace os {
// extern "C" {
// using namespace skn;
// isize write(i32 fd, const void *, usize);
// }
// } // namespace os

// namespace skn {
// namespace {
// usize len(const char *str) {
//     usize i = 0;
//     while (str[i] != '\0') i++;
//     return i;
// }

// template <typename T> struct Slice {
//     T *ptr{};
//     usize len{};

//     template <usize N> constexpr Slice(const T (&ptr)[N]) : ptr(ptr), len(N - 1) {} // NOLINT

//     constexpr explicit Slice(const char *ptr) : ptr(ptr), len(skn::len(ptr)) {}

//     constexpr Slice(const char *ptr, usize len) : ptr(ptr), len(len) {}

//     T &operator[](usize i) { return ptr[i]; }

//     bool operator==(Slice slice) {
//         if (len != slice.len) return false;
//         for (usize i = 0; i < slice.len; i++) {
//             if (ptr[i] != slice[i]) return false;
//         }
//         return true;
//     }

//     T *begin() { return ptr; }

//     T *end() { return ptr + len; }
// };

// using StringView = Slice<const char>;

// [[noreturn]] void assertFail(StringView file, usize line, StringView condition);

// #ifdef NDEBUG
// #define ASSERT(condition) ((void)0)
// #else
// #define ASSERT(condition)                                                                        \
//     do {                                                                                         \
//         if (!(condition)) {                                                                      \
//             skn::assertFail(__FILE__, __LINE__, #condition);                                     \
//         }                                                                                        \
//     } while (false)
// #endif

// template <typename T, usize N> struct Array {
//     T data[N]; // NOLINT

//     constexpr Array() = default;

//     constexpr Array(std::initializer_list<T> values) : data{} {
//         ASSERT(values.size() == N);
//         usize i = 0;
//         for (const T &value : values) {
//             data[i++] = value;
//         }
//     }

//     T &operator[](const usize i) {
//         ASSERT(i < N);
//         return data[i]; // NOLINT
//     }

//     const T &operator[](const usize i) const {
//         ASSERT(i < N);
//         return data[i]; // NOLINT
//     }

//     T *begin() { return data; }

//     T *end() { return data + N; }
// };

// enum class FileDescriptor : i32 { // NOLINT
//     stdin = 0,
//     stdout = 1,
//     stderr = 2,
// };

// isize write(FileDescriptor fd, const void *ptr, usize len) {
//     return os::write(static_cast<i32>(fd), ptr, len);
// }

// void print(StringView value) { write(FileDescriptor::stdout, value.ptr, value.len); }

// void print(usize value) {
//     constexpr usize max = log10(USIZE_MAX) + 1;
//     constexpr usize end = max - 1;
//     Array<char, max> buffer = {};
//     usize index = 0;
//     if (value == 0) {
//         buffer[end] = '0', index++;
//     } else {
//         while (value != 0) {
//             buffer[end - index] = static_cast<char>('0' + (value % 10));
//             index++, value /= 10;
//         }
//     }
//     write(FileDescriptor::stdout, &buffer[max - index], index);
// }

// void print(const char *value) { write(FileDescriptor::stdout, value, len(value)); }

// void print(char c) { write(FileDescriptor::stdout, &c, 1); }

// template <typename T, typename... Args> void print(StringView fmt, T arg, Args... args) {
//     for (usize i = 0; i + 1 < fmt.len; ++i) {
//         if (fmt[i] == '{' && fmt[i + 1] == '}') {
//             print(StringView(fmt.ptr, i));
//             print(arg);
//             print(StringView(fmt.ptr + i + 2, fmt.len - i - 2), args...);
//             return;
//         }
//     }
//     print(fmt);
// }

// void println(StringView fmt) {
//     print(fmt);
//     print('\n');
// }

// template <typename T, typename... Args> void println(StringView fmt, T arg, Args... args) {
//     print(fmt, arg, args...);
//     print('\n');
// }

// void assertFail(StringView file, usize line, StringView condition) {
//     println("{}:{}: assert fail: {}", file, line, condition);
//     abort();
// }
// } // namespace
// } // namespace skn

