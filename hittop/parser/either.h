// Disjunction between zero or more grammars.
//
#ifndef HITTOP_PARSER_EITHER_H
#define HITTOP_PARSER_EITHER_H

#include <array>
#include <type_traits>
#include <vector>

#include "hittop/parser/char_class.h"
#include "hittop/parser/failure.h"
#include "hittop/parser/literal.h"
#include "hittop/parser/parser.h"

namespace hittop {
namespace parser {

template <typename... Grammars> struct Either {};

template <typename... Grammars>
struct IsSingleCharRule<Either<Grammars...>>
    : TrueForAll<IsSingleCharRule, Grammars...> {};

// Empty disjunction is just failure.
template <> class Parser<Either<>> : public Parser<Failure> {};

// Singleton disjunction is equivalent to the single grammar.
template <typename Grammar>
class Parser<Either<Grammar>> : public Parser<Grammar> {};

// Two alternatives; this is the interesting base case that does most of the
//  work here.
template <typename First, typename Second> class Parser<Either<First, Second>> {
public:
  template <typename Range, typename... Args>
  auto operator()(const Range &input, Args &&... args) const
      -> ParseResult<decltype(std::begin(input))> {
    auto first_result = Parse<First>(input, args...);
    if (first_result.ok() || first_result.error() == ParseError::INCOMPLETE) {
      return first_result;
    }
    return Parse<Second>(input, std::forward<Args>(args)...);
  }
};

// Recursive (general) case; left-associative.
template <typename First, typename... Rest>
class Parser<Either<First, Rest...>>
    : public Parser<Either<First, Either<Rest...>>> {};

// If an Either rule is a SingleCharRule, it can be rewritten as a CharClass.
template <typename First, typename... Rest>
class OptimizedParser<Either<First, Rest...>>
    : public Parser<std::conditional_t<
          IsSingleCharRule<Either<First, Rest...>>::value,
          CharClass<First, Rest...>, Either<First, Either<Rest...>>>> {};

// TODO(tonyastolfi) - implement optimized case for:
//  Either<Token<>, Token<>, Token<>, ...>
// Sort the list of tokens once and then binary-search them.

} // namespace parser
} // namespace hittop

#endif // HITTOP_PARSER_EITHER_H
