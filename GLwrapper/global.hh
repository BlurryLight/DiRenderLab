//
// Created by zhong on 2021/4/23.
//

#ifndef DIRENDERLAB_GLOBAL_HH
#define DIRENDERLAB_GLOBAL_HH

#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>

namespace DRL {
    template<typename String, typename... Args>
    inline void AssertLog(bool condition, const String &fmt, Args &&...args) {
        if (!condition) {
            std::string fmtstring = std::string("Assertion Failed: ") + std::string(fmt);
            spdlog::error(fmtstring, std::forward<Args>(args)...);
            spdlog::shutdown();
            std::quick_exit(-1);
        }
    }

    template<typename String, typename... Args>
    inline void AssertWarning(bool condition, const String &fmt, Args &&...args) {
        if (!condition) {
            std::string fmtstring = std::string("Assertion Failed: ") + std::string(fmt);
            spdlog::warn(fmtstring, std::forward<Args>(args)...);
        }
    }

    namespace details {
        template<class T, class = std::void_t<>>
        struct bind_check : std::false_type {};

        template<class T>
        struct bind_check<T, std::void_t<decltype(std::declval<T>().bind())>> : std::true_type {};

        template<class T, class = std::void_t<>>
        struct unbind_check : std::false_type {};

        template<class T>
        struct unbind_check<T, std::void_t<decltype(std::declval<T>().unbind())>> : std::true_type {};

        template<class...>
        constexpr std::false_type always_false{};
    }// namespace details
    template<typename T>
    struct bind_guard {
        T *obj_;
        explicit bind_guard(T &bindable_obj) : obj_(&bindable_obj) {
            if constexpr (
                    (decltype(details::bind_check<T>())::value) &&
                    (decltype(details::unbind_check<T>())::value)) {
                bindable_obj.bind();
            } else {
                static_assert(details::always_false<T>, "Bind guard needs an obj with bind() and unbind() functions!");
            }
        }
        ~bind_guard() {
            obj_->unbind();
        }
    };
}// namespace DRL

////fwd declaration
//namespace fs {
//    class path;
//}


#endif//DIRENDERLAB_GLOBAL_HH
