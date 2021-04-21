// This file is generated when compiling by CMake from the template
// 'cmake_vars.h.in'.
#pragma once
#include <sstream>
#include <string>
namespace DR {
namespace impl {
inline std::string ver_string(int a, int b, int c) {
  std::ostringstream ss;
  ss << a << '.' << b << '.' << c;
  return ss.str();
}
} // namespace impl

inline std::string ROOT_DIR = "/mnt/SSD/QTC++/DiLab";
inline std::string BUILD_TYPE = "Debug";
inline std::string BUILD_COMPILER =
#ifdef __clang__
    "clang++";
#elif __GNUC__
    "g++";
#elif _MSC_VER
    "MSVC";
#else
    "Unknown Compiler";
#endif
inline std::string CXX_VER =
#ifdef __clang__
    impl::ver_string(__clang_major__, __clang_minor__, __clang_patchlevel__);
#elif __GNUC__
    impl::ver_string(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif _MSC_VER
    std::to_string(_MSC_VER);
#else
    "Unknown Compiler Version";
#endif

inline std::string BUILD_SYSTEM_VERSION = "5.4.112-1-MANJARO";
inline std::string BUILD_SYSTEM_NAME = "Linux";
inline std::string BUILD_UTC_TIMESTAMP = "2021-04-20T03:40:27Z";


} // namespace DR
