#ifndef GKXX_STRING_LITERAL_HPP
#define GKXX_STRING_LITERAL_HPP

#include <algorithm>
#include <compare>
#include <string>
#include <string_view>

namespace gkxx {

/// @brief Compile-time fixed string, enabling the use of string literals as
/// template parameters and user-literals
/// @tparam N The length of the string
template <std::size_t N>
struct fixed_string {
  consteval fixed_string(const char (&str)[N + 1]) {
    std::copy_n(str, N + 1, data);
  }
  auto operator<=>(const fixed_string &) const = default;
  bool operator==(const fixed_string &) const = default;
  constexpr std::string_view to_string_view() const noexcept {
    return {data, N};
  }
  constexpr std::string to_string() const noexcept {
    return std::string(to_string_view());
  }
  consteval auto operator[](std::size_t i) const noexcept {
    return data[i];
  }
  template <std::size_t Begin, std::size_t End>
  consteval fixed_string<End - Begin> slice() const noexcept {
    char copy[End - Begin + 1];
    std::copy(data + Begin, data + End, copy);
    copy[End - Begin] = '\0';
    return copy;
  }
  static consteval auto size() noexcept {
    return N;
  }
  char data[N + 1]{};
};

template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N - 1>;

template <std::size_t N, std::size_t M>
  requires(N != M)
inline consteval bool operator==(const fixed_string<N> &,
                                 const fixed_string<M> &) noexcept {
  return false;
}

template <std::size_t N, std::size_t M>
inline consteval auto operator+(const fixed_string<N> &lhs,
                                const fixed_string<M> &rhs) noexcept {
  char data[N + M + 1];
  std::copy_n(lhs.data, lhs.size(), data);
  std::copy_n(rhs.data, rhs.size(), data + lhs.size());
  data[N + M] = '\0';
  return fixed_string(data);
}

template <std::size_t N, std::size_t M>
inline consteval auto operator+(const char (&lhs)[N],
                                const fixed_string<M> &rhs) noexcept {
  return fixed_string(lhs) + rhs;
}

template <std::size_t N, std::size_t M>
inline consteval auto operator+(const fixed_string<N> &lhs, const char (&rhs)[M]) noexcept {
  return lhs + fixed_string(rhs);
}

} // namespace gkxx

#endif // GKXX_STRING_LITERAL_HPP