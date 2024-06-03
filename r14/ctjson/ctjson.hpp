#ifndef GKXX_CTJSON_HPP
#define GKXX_CTJSON_HPP

#include <concepts>
#include <string>
#include <tuple>
#include <utility>

#include "fixed_string.hpp"
#include "is_specialization_of.hpp"
#include "switch_case.hpp"

/*
Tokens:
  integer: -?(0|[1-9][0-9]*)
  string: "[alpha/num, punctuations, escapes '\\', '\n', '\r', '\t', '\"']*"
  true, false, null
  '{', '}', '[', ']', ',', ':'
 */

namespace gkxx::ctjson {

template <int N>
struct Integer {
  static constexpr int value = N;
  static constexpr auto to_string() {
    return std::to_string(value);
  }
};

template <fixed_string S>
struct String {
  static constexpr fixed_string value = S;
  static constexpr auto to_string() {
    return "\"" + value.to_string() + "\"";
  }
};

template <fixed_string S>
struct KeywordToken {
  static constexpr auto to_fixed_string() noexcept {
    return S;
  }
  static constexpr auto to_string() {
    return S.to_string();
  }
};

using True = KeywordToken<"true">;
using False = KeywordToken<"false">;
using Null = KeywordToken<"null">;

template <char C>
struct PunctToken {
  static constexpr auto to_fixed_string() noexcept {
    return fixed_string({C, 0});
  }
  static constexpr auto to_string() {
    return std::string(1, C);
  }
};

using LBrace = PunctToken<'{'>;
using RBrace = PunctToken<'}'>;
using LBracket = PunctToken<'['>;
using RBracket = PunctToken<']'>;
using Comma = PunctToken<','>;
using Colon = PunctToken<':'>;

template <fixed_string Msg, std::size_t Pos>
struct ErrorToken {
  static constexpr fixed_string message = Msg;
  static constexpr std::size_t position = Pos;
  static constexpr auto to_string() {
    return "<Error Token: " + message.to_string() + " at index " +
           std::to_string(position) + '>';
  }
};

namespace detect {

  template <typename T>
  inline constexpr auto is_integer_token = false;
  template <int N>
  inline constexpr auto is_integer_token<Integer<N>> = true;

  template <typename T>
  inline constexpr auto is_string_token = false;
  template <fixed_string S>
  inline constexpr auto is_string_token<String<S>> = true;

  template <typename T>
  inline constexpr auto is_keyword_token = false;
  template <fixed_string S>
  inline constexpr auto is_keyword_token<KeywordToken<S>> = true;

  template <typename T>
  inline constexpr auto is_punct_token = false;
  template <char C>
  inline constexpr auto is_punct_token<PunctToken<C>> = true;

  template <typename T>
  inline constexpr auto is_error_token = false;
  template <fixed_string Msg, std::size_t Pos>
  inline constexpr auto is_error_token<ErrorToken<Msg, Pos>> = true;

  template <typename T>
  inline constexpr auto is_value =
      is_integer_token<T> || is_string_token<T> || is_keyword_token<T>;

} // namespace detect

template <typename T>
concept CValue = detect::is_value<T>;

template <typename T>
concept CToken = detect::is_integer_token<T> || detect::is_string_token<T> ||
                 detect::is_keyword_token<T> || detect::is_punct_token<T> ||
                 detect::is_error_token<T>;

template <CToken... Tokens>
struct TokenSequence {
  static constexpr std::string reconstruct_string() {
    if constexpr (empty)
      return {};
    else
      return (... + Tokens::to_string());
  }
  static constexpr auto empty = (sizeof...(Tokens) == 0);
  static constexpr auto size = sizeof...(Tokens);
  template <std::size_t N>
  struct nth {
    using type = std::decay_t<decltype(std::get<N>(std::tuple<Tokens...>{}))>;
  };
};

inline constexpr bool is_whitespace(char c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}
inline constexpr bool is_digit(char c) {
  return c >= '0' && c <= '9';
}
inline constexpr bool is_supported_escape(char c) {
  return c == '\\' || c == 'n' || c == 'r' || c == 't' || c == '\"';
}

template <fixed_string Src>
struct Tokenizer {
  template <std::size_t Pos>
  struct next_nonwhitespace_pos;

  template <std::size_t Pos>
  struct token_getter;

  template <std::size_t Pos, CToken... CurrentTokens>
  struct lexer {
    // Src[Pos] is non-whitespace
    using token_getter = typename token_getter<Pos>::result;
    using new_token = typename token_getter::token;
    static constexpr auto next_pos =
        next_nonwhitespace_pos<token_getter::end_pos>::result;
    static consteval auto get_result() noexcept {
      if constexpr (detect::is_error_token<new_token>)
        return new_token{};
      else
        return typename lexer<next_pos, CurrentTokens..., new_token>::result{};
    }
    using result = decltype(get_result());
  };
  template <CToken... Tokens>
  struct lexer<Src.size(), Tokens...> {
    using result = TokenSequence<Tokens...>;
  };

  using result = lexer<next_nonwhitespace_pos<0>::result>::result;
};

template <fixed_string Src>
template <std::size_t Pos>
struct Tokenizer<Src>::next_nonwhitespace_pos {
  static consteval auto move() noexcept {
    auto i = Pos;
    while (i < Src.size() && is_whitespace(Src[i]))
      ++i;
    return i;
  }
  static constexpr auto result = move();
};

template <fixed_string Src>
template <std::size_t Pos>
struct Tokenizer<Src>::token_getter {
  static_assert(!is_whitespace(Src[Pos]),
                "token_getter encounters a whitespace");

  template <CToken Token, std::size_t EndPos = static_cast<std::size_t>(-1)>
  struct internal_result_t {
    static_assert(!(!detect::is_error_token<Token> &&
                    EndPos == static_cast<std::size_t>(-1)));
    using token = Token;
    static constexpr auto end_pos = EndPos;
  };

  template <fixed_string Msg, std::size_t ErrorPos>
  using error_result_t = internal_result_t<ErrorToken<Msg, ErrorPos>>;

  static consteval auto match_true() noexcept {
    if constexpr (Pos + 3 < Src.size() && Src[Pos + 1] == 'r' &&
                  Src[Pos + 2] == 'u' && Src[Pos + 3] == 'e')
      return internal_result_t<True, Pos + 4>{};
    else
      return error_result_t<"expects 'true'", Pos>{};
  }
  static consteval auto match_false() noexcept {
    if constexpr (Pos + 4 < Src.size() && Src[Pos + 1] == 'a' &&
                  Src[Pos + 2] == 'l' && Src[Pos + 3] == 's' &&
                  Src[Pos + 4] == 'e')
      return internal_result_t<False, Pos + 5>{};
    else
      return error_result_t<"expects 'false'", Pos>{};
  }
  static consteval auto match_null() noexcept {
    if constexpr (Pos + 3 < Src.size() && Src[Pos + 1] == 'u' &&
                  Src[Pos + 2] == 'l' && Src[Pos + 3] == 'l')
      return internal_result_t<Null, Pos + 4>{};
    else
      return error_result_t<"expects 'null'", Pos>{};
  }

  struct string_matcher {
    static consteval auto first_scan() noexcept {
      constexpr auto failure = Pos;
      if constexpr (Pos + 1 == Src.size())
        return std::pair{failure, 0};
      else {
        auto cur = Pos + 1;
        auto escape = 0;
        while (cur < Src.size() && Src[cur] != '"') {
          if (Src[cur] == '\\') {
            ++cur;
            if (cur < Src.size() && is_supported_escape(Src[cur])) {
              ++cur;
              ++escape;
            } else
              return std::pair{cur, -1};
          } else
            ++cur;
        }
        if (cur < Src.size() && Src[cur] == '"')
          return std::pair{cur, escape};
        else
          return std::pair{failure, 0};
      }
    }
    static consteval auto get_contents() noexcept {
      char contents[end_quote_pos - Pos - escape_cnt];
      std::size_t fill = 0;
      for (auto i = Pos + 1; i != end_quote_pos; ++i) {
        if (Src[i] == '\\') {
          ++i;
          if (Src[i] == '\\')
            contents[fill++] = '\\';
          else if (Src[i] == 'n')
            contents[fill++] = '\n';
          else if (Src[i] == 'r')
            contents[fill++] = '\r';
          else if (Src[i] == 't')
            contents[fill++] = '\t';
          else // Src[i] == '\"'
            contents[fill++] = '\"';
        } else
          contents[fill++] = Src[i];
      }
      contents[fill] = '\0';
      return fixed_string(contents);
    }
    static constexpr auto first_scan_result = first_scan();
    static constexpr auto end_quote_pos = first_scan_result.first;
    static constexpr auto escape_cnt = first_scan_result.second;
    static consteval auto get_result() noexcept {
      if constexpr (end_quote_pos == Pos)
        return error_result_t<"invalid string", Pos>{};
      else if constexpr (escape_cnt == -1)
        return error_result_t<"unsupported escape", end_quote_pos>{};
      else
        return internal_result_t<String<get_contents()>, end_quote_pos + 1>{};
    }

    using result = decltype(get_result());
  };

  struct integer_matcher {
    template <std::size_t Cur>
    struct next_nondigit_pos {
      static consteval auto get_result() noexcept {
        if constexpr (Cur < Src.size() && is_digit(Src[Cur]))
          return next_nondigit_pos<Cur + 1>::result;
        else
          return Cur;
      }
      static constexpr auto result = get_result();
    };
    template <std::size_t start, std::size_t end>
    struct calc_value {
      static constexpr std::size_t
          result = end - start > 10 ? 0
                                    : calc_value<start, end - 1>::result * 10u +
                                          (Src[end - 1] - '0');
    };
    template <std::size_t pos>
    struct calc_value<pos, pos> {
      static constexpr std::size_t result = 0;
    };
    static constexpr auto neg = (Src[Pos] == '-');
    static constexpr auto start_pos = neg ? Pos + 1 : Pos;
    static constexpr auto end_pos = next_nondigit_pos<start_pos>::result;
    static constexpr auto digits_length = end_pos - start_pos;
    static constexpr auto too_many_leading_zeros =
        digits_length >= 1 && Src[start_pos] == '0' && digits_length >= 2;
    static constexpr std::size_t value = calc_value<start_pos, end_pos>::result;
    static constexpr auto overflow = value >
                                     (2147483647ul + static_cast<int>(neg));

    using result = typename meta::switch_<
        0,
        meta::case_if<[](...) { return digits_length == 0; },
                      error_result_t<"expects integer", start_pos>>,
        meta::case_if<[](...) { return (digits_length > 10); },
                      error_result_t<"integer too long", start_pos>>,
        meta::case_if<[](...) { return too_many_leading_zeros; },
                      error_result_t<"too many leading zeros", start_pos>>,
        meta::case_if<[](...) { return overflow; },
                      error_result_t<"integer value exceeding the range "
                                     "of 32-bit signed integers",
                                     start_pos>>,
        meta::default_<internal_result_t<
            Integer<neg ? -static_cast<int>(value) : static_cast<int>(value)>,
            end_pos>>>::type;
  };

  using result = typename meta::switch_<
      Src[Pos], meta::case_<'{', internal_result_t<LBrace, Pos + 1>>,
      meta::case_<'}', internal_result_t<RBrace, Pos + 1>>,
      meta::case_<'[', internal_result_t<LBracket, Pos + 1>>,
      meta::case_<']', internal_result_t<RBracket, Pos + 1>>,
      meta::case_<',', internal_result_t<Comma, Pos + 1>>,
      meta::case_<':', internal_result_t<Colon, Pos + 1>>,
      meta::case_<'t', decltype(match_true())>,
      meta::case_<'f', decltype(match_false())>,
      meta::case_<'n', decltype(match_null())>,
      meta::case_<'"', typename string_matcher::result>,
      meta::case_if<[](char c) { return c == '-' || is_digit(c); },
                    typename integer_matcher::result>,
      meta::default_<error_result_t<"Unrecognized token", Pos>>>::type;
};

/*
json    -> {value}
value   -> {object}
         | {array}
         | String
         | Integer
         | True
         | False
         | Null
object  -> LBrace RBrace
         | LBrace {members} RBrace
members -> {member}
         | {member} Comma {members}
member  -> String Colon {value}
array   -> LBracket RBracket
         | LBracket {values} RBracket
values  -> {value}
         | {value} Comma {values}
 */

namespace detail {

  template <fixed_string Sep>
  struct separated_string {
    std::string content;
  };

  template <fixed_string Sep>
  inline constexpr separated_string<Sep>
  operator+(const separated_string<Sep> &lhs,
            const separated_string<Sep> &rhs) {
    return {lhs.content + Sep.to_string() + rhs.content};
  }

} // namespace detail

namespace detect {

  template <typename T>
  inline constexpr auto is_member = false;

} // namespace detect

template <typename T>
concept CMember = detect::is_member<T>;

namespace detail {

  template <auto...>
  inline constexpr auto has_duplicate = false;

  template <auto Only>
  inline constexpr auto has_duplicate<Only> = false;

  template <auto First, auto... Rest>
  inline constexpr auto has_duplicate<First, Rest...> =
      ((First == Rest) || ...) || has_duplicate<Rest...>;

} // namespace detail

template <CMember... Members>
  requires(!detail::has_duplicate<Members::key...>)
struct Object {
  static constexpr auto to_string() {
    if constexpr (sizeof...(Members) == 0)
      return "{}";
    else
      return "{" +
             (detail::separated_string<", ">{Members::to_string()} + ...)
                 .content +
             "}";
  }

 private:
  template <typename... Ms>
  static constexpr auto required_key_exists = (sizeof...(Ms) > 0);

  template <fixed_string K, typename... Ms>
    requires required_key_exists<Ms...>
  struct get_impl;

  template <fixed_string K>
  struct get_impl<K> {
    using result = void;
  };

  template <fixed_string K, typename M, typename... Ms>
  struct get_impl<K, M, Ms...> {
    static consteval auto helper() noexcept {
      if constexpr (K == M::key)
        return typename M::value{};
      else
        return typename get_impl<K, Ms...>::result{};
    }
    using result = decltype(helper());
  };

 public:
  template <fixed_string Key>
  using get = typename get_impl<Key, Members...>::result;
};

template <CValue... Values>
struct Array {
  static constexpr auto to_string() {
    if constexpr (sizeof...(Values) == 0)
      return "[]";
    else
      return "[" +
             (detail::separated_string<", ">{Values::to_string()} + ...)
                 .content +
             "]";
  }

 private:
  template <std::size_t N>
    requires (N < sizeof...(Values))
  struct get_impl {
    using result = std::decay_t<decltype(std::get<N>(std::tuple<Values...>{}))>;
  };

 public:
  template <std::size_t N>
  using get = typename get_impl<N>::result;
};

namespace detect {

  template <CMember... Members>
  inline constexpr auto is_value<Object<Members...>> = true;

  template <CValue... Values>
  inline constexpr auto is_value<Array<Values...>> = true;

} // namespace detect

template <fixed_string... Ss>
using ArrayStr = Array<String<Ss>...>;

template <int... Ns>
using ArrayInt = Array<Integer<Ns>...>;

template <fixed_string Key, CValue Value>
struct Member {
  static constexpr fixed_string key = Key;
  using value = Value;
  static constexpr auto to_string() {
    return "\"" + Key.to_string() + "\": " + Value::to_string();
  }
};

struct EndOfTokens {};

template <fixed_string Msg, std::size_t Pos>
struct SyntaxError {
  static constexpr fixed_string message = Msg;
  static constexpr std::size_t position = Pos;
};

namespace detect {

  template <fixed_string Key, CValue Value>
  inline constexpr auto is_member<Member<Key, Value>> = true;

  template <typename T>
  inline constexpr auto is_syntax_error = false;
  template <fixed_string Msg, std::size_t Pos>
  inline constexpr auto is_syntax_error<SyntaxError<Msg, Pos>> = true;

  template <typename T>
  inline constexpr auto is_terminal =
      is_string_token<T> || is_integer_token<T> || is_keyword_token<T>;

} // namespace detect

template <typename T>
concept CNode = CValue<T> || detect::is_member<T> || detect::is_syntax_error<T>;

template <meta::specialization_of<TokenSequence> Tokens>
struct ParseTokens {
  template <std::size_t N>
  struct nth_token_impl {
    static constexpr auto get_result() noexcept {
      if constexpr (N < Tokens::size)
        return typename Tokens::template nth<N>::type{};
      else
        return EndOfTokens{};
    }
    using result = decltype(get_result());
  };

  template <std::size_t N>
  using nth_token = typename nth_token_impl<N>::result;

  template <CNode ParseResult,
            std::size_t NextPos = static_cast<std::size_t>(-1)>
  struct internal_result_t {
    static_assert(!(!detect::is_syntax_error<ParseResult> &&
                    NextPos == static_cast<std::size_t>(-1)));
    using node = ParseResult;
    static constexpr auto next_pos = NextPos;
  };

  template <fixed_string Msg, std::size_t Pos>
  using error_result_t = internal_result_t<SyntaxError<Msg, Pos>>;

  template <std::size_t Pos>
  struct value_parser;

  template <std::size_t Pos>
  struct member_parser;

  template <std::size_t Pos, template <std::size_t> typename ElementParser,
            template <typename...> typename ListType>
  struct comma_list_parser;

  template <std::size_t Pos, typename Empty, typename Left, typename Right,
            template <std::size_t> typename ListParser>
  struct bracket_pair_list_parser;

  template <std::size_t Pos>
  using members_parser = comma_list_parser<Pos, member_parser, Object>;

  template <std::size_t Pos>
  using values_parser = comma_list_parser<Pos, value_parser, Array>;

  template <std::size_t Pos>
  using array_parser =
      bracket_pair_list_parser<Pos, Array<>, LBracket, RBracket, values_parser>;

  template <std::size_t Pos>
  using object_parser =
      bracket_pair_list_parser<Pos, Object<>, LBrace, RBrace, members_parser>;

  using result = typename value_parser<0>::result;
};

template <meta::specialization_of<TokenSequence> Tokens>
template <std::size_t Pos>
struct ParseTokens<Tokens>::value_parser {
  static consteval auto do_parse() noexcept {
    using lookahead = nth_token<Pos>;
    if constexpr (std::is_same_v<lookahead, EndOfTokens>)
      return error_result_t<"expects Value", Pos>{};
    else if constexpr (std::is_same_v<lookahead, LBrace>)
      return typename object_parser<Pos>::result{};
    else if constexpr (std::is_same_v<lookahead, LBracket>)
      return typename array_parser<Pos>::result{};
    else {
      static_assert(detect::is_terminal<lookahead>);
      return internal_result_t<lookahead, Pos + 1>{};
    }
  }
  using result = decltype(do_parse());
};

template <meta::specialization_of<TokenSequence> Tokens>
template <std::size_t Pos, typename Empty, typename Left, typename Right,
          template <std::size_t> typename ListParser>
struct ParseTokens<Tokens>::bracket_pair_list_parser {
  static consteval auto do_parse() noexcept {
    using lookahead = nth_token<Pos>;
    if constexpr (!std::is_same_v<lookahead, Left>)
      return error_result_t<"expects '" + Left::to_fixed_string() + "'", Pos>{};
    else
      return match_after_left();
  }
  static consteval auto match_after_left() noexcept {
    using lookahead = nth_token<Pos + 1>;
    if constexpr (std::is_same_v<lookahead, Right>)
      return internal_result_t<Empty, Pos + 2>{};
    else {
      using elements_result = typename ListParser<Pos + 1>::result;
      using elements_node = typename elements_result::node;
      if constexpr (detect::is_syntax_error<elements_node>)
        return elements_result{};
      else {
        constexpr auto next_pos = elements_result::next_pos;
        using next_token = nth_token<next_pos>;
        if constexpr (std::is_same_v<next_token, Right>)
          return internal_result_t<elements_node, next_pos + 1>{};
        else
          return error_result_t<"expects '" + Right::to_fixed_string() + "'",
                                next_pos>{};
      }
    }
  }
  using result = decltype(do_parse());
};

template <meta::specialization_of<TokenSequence> Tokens>
template <std::size_t Pos>
struct ParseTokens<Tokens>::member_parser {
  static consteval auto do_parse() noexcept {
    using lookahead = nth_token<Pos>;
    if constexpr (!detect::is_string_token<lookahead>)
      return error_result_t<"expects String", Pos>{};
    else
      return match_colon();
  }
  static consteval auto match_colon() noexcept {
    using lookahead = nth_token<Pos + 1>;
    if constexpr (!std::is_same_v<lookahead, Colon>)
      return error_result_t<"expects ':'", Pos + 1>{};
    else
      return match_value();
  }
  static consteval auto match_value() noexcept {
    using value_result = typename value_parser<Pos + 2>::result;
    using value_node = typename value_result::node;
    if constexpr (detect::is_syntax_error<value_node>)
      return value_result{};
    else
      return internal_result_t<Member<nth_token<Pos>::value, value_node>,
                               value_result::next_pos>{};
  }
  using result = decltype(do_parse());
};

template <meta::specialization_of<TokenSequence> Tokens>
template <std::size_t Pos, template <std::size_t> typename ElementParser,
          template <typename...> typename ListType>
struct ParseTokens<Tokens>::comma_list_parser {
  template <std::size_t CurPos, typename... CurElems>
  struct parse {
    template <typename MaybeMember>
    static consteval bool duplicate_object_key() noexcept {
      if constexpr (std::is_same_v<ListType<>, Object<>>)
        return (... || (MaybeMember::key == CurElems::key));
      else
        return false;
    }
    static consteval auto get_result() noexcept {
      using new_elem_result = typename ElementParser<CurPos>::result;
      using new_elem_node = typename new_elem_result::node;
      if constexpr (detect::is_syntax_error<new_elem_node>)
        return new_elem_result{};
      else {
        if constexpr (duplicate_object_key<new_elem_node>())
          return error_result_t<"duplicate object key", CurPos>{};
        else {
          constexpr auto next_pos = new_elem_result::next_pos;
          using next_token = nth_token<next_pos>;
          if constexpr (std::is_same_v<next_token, Comma>)
            return typename parse<next_pos + 1, CurElems...,
                                  new_elem_node>::result{};
          else
            return internal_result_t<ListType<CurElems..., new_elem_node>,
                                     next_pos>{};
        }
      }
    }
    using result = decltype(get_result());
  };
  using result = typename parse<Pos>::result;
};

template <fixed_string JsonCode>
struct parse {
  static consteval auto get_result() noexcept {
    using tokenize_result = Tokenizer<JsonCode>::result;
    if constexpr (detect::is_error_token<tokenize_result>)
      return tokenize_result{};
    else {
      using parse_result = typename ParseTokens<tokenize_result>::result;
      using root_node = typename parse_result::node;
      constexpr auto next_pos = parse_result::next_pos;
      if constexpr (detect::is_syntax_error<root_node>)
        return root_node{};
      else if constexpr (next_pos < tokenize_result::size)
        return SyntaxError<"expects end of string", next_pos>{};
      else
        return root_node{};
    }
  }
  using result = decltype(get_result());
};

namespace pretty {

  using namespace std::string_literals;

  inline constexpr auto indents(std::size_t n) {
    return std::string(n, ' ');
  }

  template <CNode T>
  struct type_name;

  template <int N>
  struct type_name<Integer<N>> {
    static constexpr auto get([[maybe_unused]] std::size_t indent) {
      return "Integer<" + std::to_string(N) + ">";
    }
  };

  template <fixed_string S>
  struct type_name<String<S>> {
    static constexpr auto get([[maybe_unused]] std::size_t indent) {
      return "String<\"" + S.to_string() + "\">";
    }
  };

  template <fixed_string S>
  struct type_name<KeywordToken<S>> {
    static constexpr auto get([[maybe_unused]] std::size_t indent) {
      auto keyword = KeywordToken<S>::to_string();
      keyword.front() -= 'a' - 'A';
      return keyword;
    }
  };

  template <CMember... Members>
  struct type_name<Object<Members...>> {
    static constexpr auto get([[maybe_unused]] std::size_t indent) {
      if constexpr (sizeof...(Members) == 0)
        return "Object<>";
      else
        return "Object<\n" +
               (detail::separated_string<",\n">{
                    type_name<Members>::get(indent + 2)} +
                ...)
                   .content +
               '\n' + indents(indent) + '>';
    }
  };

  template <CValue... Values>
  struct type_name<Array<Values...>> {
    static constexpr auto get([[maybe_unused]] std::size_t indent) {
      if constexpr (sizeof...(Values) == 0)
        return "Array<>";
      else
        return "Array<\n" +
               (detail::separated_string<",\n">{
                    indents(indent + 2) + type_name<Values>::get(indent + 2)} +
                ...)
                   .content +
               '\n' + indents(indent) + '>';
    }
  };

  template <fixed_string Key, CValue Value>
  struct type_name<Member<Key, Value>> {
    static constexpr auto get([[maybe_unused]] std::size_t indent) {
      return indents(indent) + "Member<\"" + Key.to_string() + "\", " +
             type_name<Value>::get(indent) + '>';
    }
  };

} // namespace pretty

template <CNode T>
static constexpr auto pretty_type_name() {
  return pretty::type_name<T>::get(0);
}

} // namespace gkxx::ctjson

#endif // GKXX_CTJSON_HPP