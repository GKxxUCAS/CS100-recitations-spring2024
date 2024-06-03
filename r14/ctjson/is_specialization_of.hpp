#ifndef GKXX_IS_SPECIALIZATION_OF_HPP
#define GKXX_IS_SPECIALIZATION_OF_HPP

#include <type_traits>

namespace gkxx::meta {

template <typename T, template <typename...> typename Template>
struct is_specialization_of : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

template <typename T, template <typename...> typename Template>
inline constexpr bool is_specialization_of_v =
    is_specialization_of<T, Template>::value;

#if __cplusplus > 201703L
template <typename T, template <typename...> typename Template>
concept specialization_of = is_specialization_of_v<T, Template>;
#endif

} // namespace gkxx::meta

#endif // GKXX_IS_SPECIALIZATION_OF_HPP