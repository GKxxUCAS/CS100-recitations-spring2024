<!-- 以下内容建议归入“模板”一章 -->

### 正确使用 SFINAE 技术 {#sfinae-basic}

#### 适用版本

C++ 98 及以后

#### 解释

SFINAE (Substitution Failure Is Not An Error) ，即“替换失败不是错误”，是指在用显式指定或推导出的类型替换模板参数时如果发生失败，这种失败不被认为是一个错误，而是导致当前的模板特化被从重载集合中去除。这一特点被广泛运用于模板元编程 (template metaprogramming) 中，可以被用来判断一个类型或表达式是否是良定义的 (well-defined) ，用来在编译期选择不同的函数等。

然而，尽管 SFINAE 技术能有效地实现一些功能，但借助 SFINAE 技术写出的代码可读性较差，且涉及到较为复杂的模板技巧。在某些情况下，我们可以使用诸如标签派发 (\ref{#tag-dispatch})、 `if constexpr` (\ref{#if-constexpr}) 、 concepts 和 requires 子句 (\ref{#concepts}) 等方式来代替 SFINAE 。

#### 正例

标准库 `std::unique_ptr<T, D>` 的类型成员 `pointer` 在类型 `std::remove_reference_t<D>::pointer` 存在的情况下是这个类型的别名，否则是 `T *` 的别名。这可以使用 SFINAE 技术来实现。

```cpp
template <typename T, typename D>
class unique_ptr {
  template <typename U = D,
            typename V = typename std::remove_reference_t<U>::pointer>
  static V pointer_sfinae_helper(int);

  static T *pointer_sfinae_helper(...);

public:
  using pointer = decltype(pointer_sfinae_helper(0));
};
```

#### 正例

借助 `std::enable_if` 对模板函数的参数加以约束。（C++11 及以后）

```cpp
template <typename T,
          typename = std::enable_if_t<std::is_move_constructible_v<T>
              && std::is_move_assignable_v<T>>>
void swap(T &a, T &b) {
  // ...
}

template <typename T, std::size_t N,
          typename = std::enable_if_t<std::is_swappable_v<T>>>
void swap(T (&a)[N], T (&b)[N]) {
  // ...
}
```

#### 正例

借助 `std::void_t` 检测一个类型是否存在。（C++17 及以后）

```cpp
template<class, class = void>
struct has_pre_increment_member : std::false_type {};
 
template<class T>
struct has_pre_increment_member<T,
           std::void_t<decltype(++std::declval<T&>())>> : std::true_type {};
```

### 如果可以，使用标签派发技术代替 SFINAE {#tag-dispatch}

#### 适用版本

C++ 98 及以后

#### 解释

当我们需要基于某个类型的某个特征来选择不同版本的函数时，可以使用标签派发 (tag dispatch) 技术。

#### 正例

对于迭代器，可以使用 `std::iterator_traits<I>::iterator_category` 作为标签，基于此进行派发。

```cpp
template <typename Iterator>
typename std::iterator_traits<Iterator>::difference_type
distance_impl(Iterator begin, Iterator end, std::input_iterator_tag) {
  typename std::iterator_traits<Iterator>::difference_type dist = 0;
  for (; begin != end; ++begin)
    ++dist;
  return dist;
}

template <typename Iterator>
typename std::iterator_traits<Iterator>::difference_type
distance_impl(Iterator begin, Iterator end, std::random_access_iterator_tag) {
  return end - begin;
}

template <typename Iterator>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator begin, Iterator end) {
  using category = typename std::iterator_traits<Iterator>::iterator_category;
  return distance_impl(begin, end, category{});
}
```

#### 反例

这里使用 SFINAE 则显得繁琐，不够清晰。

```cpp
template <typename Iterator,
          std::enable_if_t<std::is_same_v<
              std::random_access_iterator_tag,
              typename std::iterator_traits<Iterator>::iterator_category>, bool> = true>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator begin, Iterator end) {
  return end - begin;
}

template <typename Iterator,
          std::enable_if_t<!std::is_same_v<
              std::random_access_iterator_tag,
              typename std::iterator_traits<Iterator>::iterator_category>, bool> = true>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator begin, Iterator end) {
  typename std::iterator_traits<Iterator>::difference_type dist = 0;
  for (; begin != end; ++begin)
    ++dist;
  return dist;
}
```

### 如果可以，使用 `if constexpr` 代替 SFINAE {#if-constexpr}

#### 适用版本

C++ 17 及以后

#### 解释

`if constexpr` 根据一个编译时 `bool` 常量的值来决定编译或不编译某一段代码，这使得某些原本需要借助 SFINAE 技术来实现的功能可以直接、清晰地被实现。

#### 正例

```cpp
template <typename Iterator>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator begin, Iterator end) {
  using category = typename std::iterator_traits<Iterator>::iterator_category;
  if constexpr (std::is_same_v<category, std::random_access_iterator_tag>)
    return end - begin;
  else {
    typename std::iterator_traits<Iterator>::difference_type dist = 0;
    for (; begin != end; ++begin)
      ++dist;
    return dist;
  }
}
```

### 如果可以，使用 concepts 和 requires 代替 SFINAE {#concepts}

#### 适用版本

C++ 20 及以后

#### 解释

Concepts 能更准确、清晰地表达对于模板参数的约束。

`requires` 子句可以直接检测一个类型或表达式是否是良定义的。

因此在有了 concepts 及相关语言特性之后，大多数时候都无需使用 SFINAE 。

#### 正例

```cpp
template <std::random_access_iterator Iterator>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator begin, Iterator end) {
  return end - begin;
}

template <std::input_iterator Iterator>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator begin, Iterator end) {
  typename std::iterator_traits<Iterator>::difference_type dist = 0;
  for (; begin != end; ++begin)
    ++dist;
  return dist;
}
```

#### 正例

使用 `requires` 判断一个类型是否存在。

```cpp
template <typename T, typename D>
class unique_ptr {
  static consteval auto pointer_helper() {
    if constexpr (requires { typename std::remove_reference_t<D>::pointer; })
      return typename std::remove_reference_t<D>::pointer{};
    else
      return static_cast<T *>(nullptr);
  }

public:
  using pointer = decltype(pointer_helper());
};
```

<!-- TODO: 编写 SFINAE-friendly 的代码？有必要吗？ -->
