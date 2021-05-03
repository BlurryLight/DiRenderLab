//
// Created by zhong on 2021/4/23.
//

#ifndef DIRENDERLAB_GLOBAL_HH
#define DIRENDERLAB_GLOBAL_HH

#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>

namespace DRL {
template <typename String, typename... Args>
inline void AssertLog(bool condition, const String &fmt, Args &&... args) {
  if (!condition) {
    std::string fmtstring =
        std::string("Assertion Failed: ") + std::string(fmt);
    spdlog::error(fmtstring, std::forward<Args>(args)...);
    spdlog::shutdown();
    std::quick_exit(-1);
  }
}

template <typename String, typename... Args>
inline void AssertWarning(bool condition, const String &fmt, Args &&... args) {
  if (!condition) {
    std::string fmtstring =
        std::string("Assertion Failed: ") + std::string(fmt);
    spdlog::warn(fmtstring, std::forward<Args>(args)...);
  }
}

namespace details {
template <class T, class = std::void_t<>>
struct bind_check : std::false_type {};

template <class T>
struct bind_check<T, std::void_t<decltype(std::declval<T>().bind())>>
    : std::true_type {};

template <class T, class = std::void_t<>>
struct unbind_check : std::false_type {};

template <class T>
struct unbind_check<T, std::void_t<decltype(std::declval<T>().unbind())>>
    : std::true_type {};

template <class...> constexpr std::false_type always_false{};
} // namespace details

template <typename... T> struct bind_guard {
  std::tuple<T &...> objs_;
  explicit bind_guard(T &... bindable_obj) : objs_(std::tie(bindable_obj...)) {
    std::apply([](auto &... obj) { (obj.bind(), ...); }, objs_);
  }
  ~bind_guard() {
    std::apply([](auto &... obj) { (obj.unbind(), ...); }, objs_);
  }
  bind_guard(const bind_guard &) = delete;
  bind_guard &operator=(const bind_guard &) = delete;
};

template <> struct bind_guard<> {
  bind_guard() = default;
  ~bind_guard() = default;
  bind_guard(const bind_guard &) = delete;
  bind_guard &operator=(const bind_guard &) = delete;
};
} // namespace DRL

////fwd declaration
// namespace fs {
//    class path;
//}

#endif // DIRENDERLAB_GLOBAL_HH
