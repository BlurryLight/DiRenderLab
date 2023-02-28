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
inline void AssertLog(bool condition, const String &fmt, Args &&...args) {
  if (!condition) {
    std::string fmtstring =
        std::string("Assertion Failed: ") + std::string(fmt);
    spdlog::error(fmtstring, std::forward<Args>(args)...);
    spdlog::shutdown();
    std::quick_exit(-1);
  }
}

template <typename String, typename... Args>
inline void AssertWarning(bool condition, const String &fmt, Args &&...args) {
  if (!condition) {
    std::string fmtstring =
        std::string("Assertion Failed: ") + std::string(fmt);
    spdlog::warn(fmtstring, std::forward<Args>(args)...);
  }
}
template <typename T, typename = void>
struct is_pointer_like_arrow_dereferencable : std::false_type {};

// 检测是否有->函数
template <typename T>
struct is_pointer_like_arrow_dereferencable<
    T, std::void_t<decltype(std::declval<T>().operator->())>> : std::true_type {
};

//如果有，再检测是否是int*这种类型
template <typename T>
struct is_pointer_like_arrow_dereferencable<T *, void>
    : std::disjunction<std::is_class<T>, std::is_union<T>> {};

template <typename... T> struct bind_guard {
  // https://godbolt.org/z/q8TE4Maed SFINAE
  // https://godbolt.org/z/TPE3197ax
  // 这里不要写很复杂，std::apply碰见没有实现bind和unbind的类型会给出很友好的提示
  std::tuple<T &...> objs_;
  template <typename... Args>

  explicit bind_guard(T &...bindable_obj) : objs_(std::tie(bindable_obj...)) {
    std::apply([](auto &&...args) { bind(args...); }, objs_);
  }
  ~bind_guard() {
    std::apply([](auto &&...args) { unbind(args...); }, objs_);
  }
  bind_guard(const bind_guard &) = delete;
  bind_guard &operator=(const bind_guard &) = delete;

private:
  template <typename U> static void bind(U &&t) {
    if constexpr (is_pointer_like_arrow_dereferencable<U>::value || std::is_pointer<U>::value )
      t->bind();
    else
      t.bind();
  }
  template <typename U, typename... Args>
  static void bind(U &&t, Args &&...rest) {
    bind(t);
    bind(rest...);
  }
  template <typename U> static void unbind(U &&t) {
    if constexpr (is_pointer_like_arrow_dereferencable<U>::value || std::is_pointer_v<U>)
      t->unbind();
    else
      t.unbind();
  }
  template <typename U, typename... Args>
  static void unbind(U &&t, Args &&...rest) {
    unbind(t);
    unbind(rest...);
  }
};

} // namespace DRL

////fwd declaration
// namespace fs {
//    class path;
//}

#endif // DIRENDERLAB_GLOBAL_HH
