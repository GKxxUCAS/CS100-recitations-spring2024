#ifndef GKXX_SWITCH_CASE_HPP
#define GKXX_SWITCH_CASE_HPP

#include <concepts>

namespace gkxx::meta {

template <auto Label, typename Result>
struct case_ {
  template <auto Expr>
  static constexpr auto match = (Expr == Label);
  using result = Result;
};

template <auto Pred, typename Result>
struct case_if {
  template <auto Expr>
  static constexpr auto match = Pred(Expr);
  using result = Result;
};

namespace detail {

  template <typename T>
  inline constexpr auto is_case_ = false;
  template <auto L, typename R>
  inline constexpr auto is_case_<case_<L, R>> = true;
  template <auto Pred, typename R>
  inline constexpr auto is_case_<case_if<Pred, R>> = true;

  struct default_label_t {
    explicit default_label_t() = default;
    template <typename T>
    constexpr bool operator==(T &&) const volatile noexcept {
      return true;
    }
  };

  inline constexpr default_label_t default_label{};

} // namespace detail

template <typename Result>
using default_ = case_<detail::default_label, Result>;

template <typename T>
concept CCase = detail::is_case_<T>;

template <auto Expr, CCase... Cases>
struct switch_;

template <auto Expr>
struct switch_<Expr> {};

template <auto Expr, CCase First, CCase... Rest>
struct switch_<Expr, First, Rest...> {
 private:
  template <typename T>
  struct wrapper_type {
    using type = T;
  };

 public:
  using type = decltype([] {
    if constexpr (First::template match<Expr>)
      return wrapper_type<typename First::result>{};
    else
      return wrapper_type<typename switch_<Expr, Rest...>::type>{};
  }())::type;
};

} // namespace gkxx::meta

#endif // GKXX_SWITCH_CASE_HPP