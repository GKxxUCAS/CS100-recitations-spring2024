#ifndef GKXX_WHEEL_TYPE_NAME_HPP
#define GKXX_WHEEL_TYPE_NAME_HPP

#if __cplusplus >= 201703L

#include <string_view>

namespace gkxx {

template <typename T>
constexpr auto get_type_name() {
#if defined(__clang__)
  constexpr auto prefix = std::string_view{"[T = "};
  constexpr auto suffix = "]";
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
  constexpr auto prefix = std::string_view{"[with T = "};
  constexpr auto suffix = "]";
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__MSC_VER)
  constexpr auto prefix = std::string_view{"get_type_name<"};
  constexpr auto suffix = ">(void)";
  constexpr auto function = std::string_view{__FUNCSIG__};
#else
#error Unsupported compiler
#endif

  const auto start = function.find(prefix) + prefix.size();
  const auto end = function.find_last_of(suffix);
  const auto size = end - start;

  return function.substr(start, size);
}

} // namespace gkxx

#else

#if __has_include(<boost/type_index.hpp>)
#include <boost/type_index.hpp>
#include <string>

namespace gkxx {

template <typename T>
inline std::string get_type_name() {
  return boost::typeindex::type_id_with_cvr<T>().pretty_name();
}

} // namespace gkxx

#else
#error <boost/type_index.hpp> is required for standards before C++17.
#endif // boost

#endif // C++17

#endif // GKXX_WHEEL_TYPE_NAME_HPP