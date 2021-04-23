//
// Created by zhong on 2021/4/23.
//

#ifndef DIRENDERLAB_GLOBAL_HH
#define DIRENDERLAB_GLOBAL_HH

#include <spdlog/spdlog.h>
#include <string>
namespace DRL {
    template<typename String, typename... Args>
    inline void AssertLog(bool condition, const String &fmt, Args &&...args) {
        if (!condition) {
            std::string fmtstring = std::string("Assertion Failed: ") + std::string(fmt);
            spdlog::error(fmtstring, std::forward<Args>(args)...);
            spdlog::shutdown();
            std::abort();
        }
    }
}// namespace DRL

#endif//DIRENDERLAB_GLOBAL_HH
