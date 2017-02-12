// Defines the generic Parser interface for the Casper parser generator library.
//
#ifndef HITTOP_PARSER_PARSER_H
#define HITTOP_PARSER_PARSER_H

#include <iostream>

#include "boost/range/as_literal.hpp"

#include "hittop/parser/parse_error.h"

#include "hittop/util/fallible.h"
#include "hittop/util/is_callable.h"

namespace hittop {
namespace parser {

using util::Fallible;

// The form of a Parser class.  There is no generic implementation of Parser,
// only partial and full specializations that define how to parse specific
// grammars.
template <typename Grammar> class Parser;

template <typename Grammar, typename Range> struct ParserRunner {
  using iterator = decltype(std::begin(std::declval<Range>()));
  using output_range_type = boost::iterator_range<iterator>;
  using result_type = Fallible<output_range_type>;

public:
  explicit ParserRunner(const Range &input) : input_(input) {}

  template <typename... Args> result_type operator()(Args &&... args);

  Fallible<iterator> &&consume() {
    if (!called_) {
      operator()();
    }
    return std::move(result_);
  }

private:
  const Range &input_;
  bool called_ = false;
  Fallible<iterator> result_;
};

// Convenience wrapper around defining a new Parser object and invoking it on
// the given input range.  Allows the invocation operator defined on
// Parser<Grammer> to be non-const, as the parser instance created within this
// function is itself non-const.
template <typename Grammar, typename Range, typename Visitor,
          typename = std::enable_if_t<util::IsCallable<
              Visitor, Grammar, ParserRunner<Grammar, const Range &>>::value>,
          typename... Args>
auto Parse(const Range &input, Visitor &&visitor, Args &&... args)
    -> decltype(std::declval<Parser<Grammar>>()(input)) {
  ParserRunner<Grammar, Range> run_parser{input};
  std::forward<Visitor>(visitor)(Grammar{}, run_parser);
  return run_parser.consume();
}

template <typename Grammar, typename Range, typename... Args>
auto Parse(const Range &input, Args &&... args)
    -> decltype(std::declval<Parser<Grammar>>()(input)) {
  Parser<Grammar> parser;
  return parser(input, std::forward<Args>(args)...);
}

// Convenience function that parses a C string as a literal character range.
template <typename Grammar>
auto Parse(const char *input)
    -> decltype(Parse<Grammar>(boost::as_literal(input))) {
  return Parse<Grammar>(boost::as_literal(input));
}

// Implementation of ParserRunner::operator() - invokes the parser on behalf of
// a visitor.
template <typename Grammar, typename Range>
template <typename... Args>
inline typename ParserRunner<Grammar, Range>::result_type
ParserRunner<Grammar, Range>::operator()(Args &&... args) {
  called_ = true;
  result_ = Parse<Grammar>(input_, std::forward<Args>(args)...);
  return {boost::make_iterator_range(std::begin(input_), result_.get()),
          result_.error()};
}

} // namespace parser
} // namespace hittop

#define REGISTER_PARSE_RULE(name)                                              \
  names.emplace(std::make_pair(&typeid(name), #name))

#endif // HITTOP_PARSER_PARSER_H
