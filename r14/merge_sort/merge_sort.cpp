#include <type_traits>

template <int...>
struct int_list;

template <>
struct int_list<> {};

using empty_list = int_list<>;

template <int first, int... rest>
struct int_list<first, rest...> {
  static constexpr int head = first;
  using tail = int_list<rest...>;
};

template <int, typename>
struct head_and_tail;

template <int head, int... tail>
struct head_and_tail<head, int_list<tail...>> {
  using result = int_list<head, tail...>;
};

template <typename, typename>
struct merge;

template <>
struct merge<empty_list, empty_list> {
  using result = empty_list;
};

template <int... first>
struct merge<int_list<first...>, empty_list> {
  using result = int_list<first...>;
};

template <int... second>
struct merge<empty_list, int_list<second...>> {
  using result = int_list<second...>;
};

template <int... first, int... second>
struct merge<int_list<first...>, int_list<second...>> {
 private:
  using x = int_list<first...>;
  using y = int_list<second...>;
  static constexpr int xh = x::head, yh = y::head;

 public:
  using result = std::conditional_t<
      (xh < yh),
      typename head_and_tail<xh, typename merge<typename x::tail, y>::result>::result,
      typename head_and_tail<yh, typename merge<x, typename y::tail>::result>::result>;
};

template <typename, unsigned>
struct split;

template <int... content>
struct split<int_list<content...>, 0u> {
  using left_result = empty_list;
  using right_result = int_list<content...>;
};

template <int... content, unsigned N>
struct split<int_list<content...>, N> {
 private:
  using l = int_list<content...>;
  using split_next = split<typename l::tail, N - 1>;

 public:
  using left_result = head_and_tail<l::head, typename split_next::left_result>::result;
  using right_result = split_next::right_result;
};

template <typename>
struct merge_sort;

template <>
struct merge_sort<empty_list> {
  using result = empty_list;
};

template <int x>
struct merge_sort<int_list<x>> {
  using result = int_list<x>;
};

template <int... content>
struct merge_sort<int_list<content...>> {
 private:
  using spliter = split<int_list<content...>, sizeof...(content) / 2>;
  using left_merged = merge_sort<typename spliter::left_result>::result;
  using right_merged = merge_sort<typename spliter::right_result>::result;

 public:
  using result = merge<left_merged, right_merged>::result;
};

template <typename>
struct is_sorted;

template <>
struct is_sorted<int_list<>> {
  static constexpr auto result = true;
};

template <int x>
struct is_sorted<int_list<x>> {
  static constexpr auto result = true;
};

template <int first, int second, int... rest>
struct is_sorted<int_list<first, second, rest...>> {
  static constexpr auto result =
      (first < second) && is_sorted<int_list<second, rest...>>::result;
};

int main() {
  using l = int_list<95554, 67802, 72486, 31920, 84531, 61547, 42905, 26623,
                     42834, 2156>;
  using sorted = merge_sort<l>::result;
  static_assert(is_sorted<sorted>::result);
  return 0;
}
