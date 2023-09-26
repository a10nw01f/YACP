#pragma once

#include "utils.h"
#include "parser.h"
#include "context.h"
#include "containers.h"
#include "create_parsers.h"

template<class T>
using DefaultVectorType = FixedVec<T, 16>;

namespace dsl
{
    template<StringLiteralHelper helper>
    constexpr auto operator ""_ctx()
    {
        return Context(helper, 0);
    }

    template<StringLiteralHelper helper>
    constexpr auto operator ""_lit()
    {
        return LiteralParser<helper>();
    }

    template<SequenceParserConcept T, SequenceParserConcept U>
    constexpr auto operator>>(T lhs, U rhs) {
        using Type = Concat_t<typename T::FuncType, typename U::FuncType>;
        return Parser(Type());
    }

    template<SequenceParserConcept T, ParserConcept U>
    constexpr auto operator>>(T lhs, U rhs) {
        using Type = Concat_t<typename T::FuncType, SequenceParserFunc<U>>;
        return Parser(Type());
    }

    template<ParserConcept T, SequenceParserConcept U>
    constexpr auto operator>>(T lhs, U rhs) {
        using Type = Concat_t<SequenceParserFunc<T>, typename U::FuncType>;
        return Parser(Type());
    }

    template<ParserConcept T, ParserConcept U>
    constexpr auto operator>>(T lhs, U rhs) {
        return SequenceParser(lhs, rhs);
    }

    template<ParserConcept T, class F>
    constexpr auto operator>>=(T parser, F func) {
        return ConvertParsedValue(parser, func);
    }

    template<ParserConcept T>
    constexpr auto operator*(T parser) {
        return RepeatParser<DefaultVectorType>(parser);
    }

    template<VariantParserConcept T, VariantParserConcept U>
    constexpr auto operator|(T lhs, U rhs) {
        using Type = Concat_t<typename T::FuncType, typename U::FuncType>;
        return Parser(Type());
    }

    template<VariantParserConcept T, ParserConcept U>
    constexpr auto operator|(T lhs, U rhs) {
        using Type = Concat_t<typename T::FuncType, VariantParserFunc<U>>;
        return Parser(Type());
    }

    template<ParserConcept T, VariantParserConcept U>
    constexpr auto operator|(T lhs, U rhs) {
        using Type = Concat_t<VariantParserFunc<T>, typename U::FuncType>;
        return Parser(Type());
    }

    template<ParserConcept T, ParserConcept U>
    constexpr auto operator|(T lhs, U rhs) {
        return VariantParser(lhs, rhs);
    }

    template<ParserConcept T>
    constexpr auto operator+(T arg) {
        return OptionalParser(arg);
    }

    template<ParserConcept T>
    constexpr auto operator!(T arg) {
        return NotParser(arg);
    }

    struct ConditionalParserTag {};

    constexpr auto cond = ConditionalParserTag{};

    template<class F>
    constexpr auto operator>(ConditionalParserTag, F) {
        return ConditionalParser(F{});
    }

    constexpr auto always = Parser([](ContextConcept auto& context)->ParseResult<Empty> {
        return Empty{};
    });
}
