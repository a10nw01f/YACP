#pragma once

#include "parser.h"
#include "context.h"
#include "static_string.h"
#include "containers.h"
#include <expected>
#include <variant>
#include <tuple>

struct ParseError{};

template<class T>
using ParseResult = std::expected<T, ParseError>;

struct Empty { char m_Dummy = {}; };

template<class F>
constexpr auto ConditionalParser(F&&){
	return Parser([](ContextConcept auto& context)->ParseResult<Empty>{
		if (context.eof() != context.current() && F {}(context.current())){
			context.next();
			return Empty{};
		}
		return std::unexpected(ParseError{});
	});
}

template<StringLiteralHelper str>
constexpr auto LiteralParser(){
	return Parser([](ContextConcept auto& context)->ParseResult<Empty>{
			auto saved = context.save();
			for (auto c : str) {
				if (context.current() != c){
					context.restore(saved);
					return std::unexpected(ParseError{});
				}
				context.next();
			}
			return Empty{};
		});
}

template<ParserConcept TParser>
constexpr auto OptionalParser(TParser){
	constexpr auto helper = [](ContextConcept auto& context) {
		auto result = TParser{}(context).value();
		if constexpr (std::is_same_v<decltype(result), Empty>) {
			return Empty{};
		}
		else {
			return Variant<Empty, decltype(result)>(Empty{});
		}
	};

	return Parser([](ContextConcept auto& context)->ParseResult<decltype(helper(context))>{
		auto parser_result = TParser()(context);
		if (parser_result) {
			return *parser_result;
		}
		return Empty{};
	});
}

template<class TContext, class TValue, class TSaved>
struct ConvertParsedValueHelper {
	TContext& m_Context;
	TSaved m_Saved;
	TValue& m_Value;

	constexpr ConvertParsedValueHelper(TContext& context, TSaved& saved, TValue& value):
		m_Context(context),
		m_Saved(saved),
		m_Value(value) {
	}

	constexpr auto get_range() {
		return m_Context.get_range(m_Saved);
	}

	constexpr auto& get_value() {
		return m_Value;
	}

	constexpr auto& get_allocator() {
		return m_Context.get_allocator();
	}
};

template<ParserConcept TParser, class F>
constexpr auto ConvertParsedValue(TParser, F)
{
	return Parser([](ContextConcept auto& context){
		auto saved = context.save();
		auto parse_result = TParser{}(context);
		using T = decltype(F{}(ConvertParsedValueHelper(context, saved, parse_result.value())));
		using Ret = ParseResult<T>;
		if (!parse_result)
		{
			return Ret(std::unexpected(ParseError{}));
		}
		return Ret(F{}(ConvertParsedValueHelper(context, saved, parse_result.value())));
	});
}

template<template<class> class Container, ParserConcept TParser>
constexpr auto RepeatParser(TParser){
	constexpr auto helper = [](auto context) {
		auto parse_result = TParser{}(context).value();
		using T = decltype(parse_result);
		if constexpr (std::is_same_v<Empty, T>) {
			return Empty{};
		}
		else {
			return Container<T>();
		}
	};
	return Parser([](ContextConcept auto& context)->ParseResult<decltype(helper(context))> {
		using Ret = decltype(helper(context));
		Ret values;
		while (true)
		{
			auto saved = context.save();
			auto parse_result = TParser{}(context);
			if (!parse_result){
				context.restore(saved);
				break;
			}
			if constexpr (!std::is_same_v<Ret, Empty>){
				values.push_back(*parse_result);
			}
		}
		return values;
	});
}

template<ParserConcept... TParsers>
struct SequenceParserFunc{
private:
	static constexpr auto apply_filter(auto context) {
		constexpr auto ret_types = TypeList<decltype(TParsers{}(context))::value_type... > {};
		constexpr auto filtered = Filter(ret_types, [](auto v) { return !std::is_same_v<Empty, decltype(v.get())>; });
		return filtered;
	}

	static constexpr auto get_ret_type(auto filtered) {
		constexpr auto size = TupleSize(filtered);
		if constexpr (size == 0) {
			return TypeWrapper<Empty>{};
		}
		else if constexpr(size == 1){
			return First(filtered);
		}
		else {
			using Ret = decltype(Rename<Tuple>(filtered));
			return TypeWrapper<Ret>();
		}
	}

public:
	constexpr auto operator()(ContextConcept auto& context) const {
		constexpr auto filtered = decltype(apply_filter(context)){};
		using Ret = decltype(get_ret_type(filtered).get());
		constexpr auto is_tup = TupleSize(filtered) > 1;
		Ret result{};

		auto saved = context.save();
		auto success = [&](this auto&& func, auto parser_index, auto result_idx) {
			if constexpr (sizeof...(TParsers) == parser_index.get()) {
				return true;
			}
			else {
				auto parse_result = std::get<parser_index.get()>(std::tuple(TParsers{}...))(context);
				if (!parse_result) {
					return false;
				}
				if constexpr (std::is_same_v<Empty, decltype(parse_result)::value_type>) {
					return func(parser_index.inc(), result_idx);
				}
				else {
					if constexpr (is_tup) {
						result.get(result_idx) = *parse_result;
					}
					else {
						result = *parse_result;
					}

					return func(parser_index.inc(), result_idx.inc());
				}
			}
		}(IndexWrapper<0>(), IndexWrapper<0>());

		if (!success) {
			context.restore(saved);
			return ParseResult<Ret>(std::unexpected(ParseError{}));
		}

		return ParseResult<Ret>(result);
	}
};

TEMPLATE_CONCEPT(SequenceParserFunc)

template<class T>
concept SequenceParserConcept = ParserConcept<T> && SequenceParserFuncConcept<typename T::FuncType>;

template<ParserConcept... TParsers>
constexpr auto SequenceParser(TParsers...) {
	return Parser(SequenceParserFunc<TParsers...>{});
}

template<ParserConcept... TParsers>
struct VariantParserFunc {
	static constexpr auto get_ret_type(auto& context) {
		constexpr auto ret_types = TypeList<decltype(TParsers{}(context))::value_type... > {};
		constexpr auto filtered = Unique(ret_types);
		constexpr auto size = TupleSize(filtered);
		if constexpr (size == 0) {
			return TypeWrapper<Empty>{};
		}
		else if constexpr (size == 1) {
			return First(filtered);
		}
		else {
			using Ret = decltype(Rename<Variant>(filtered));
			return TypeWrapper<Ret>();
		}
	}

	constexpr auto operator()(ContextConcept auto& context) const {
		using Ret = ParseResult<decltype(VariantParserFunc::get_ret_type(context).get())>;
		auto saved = context.save();
		return [&](this auto&& func, auto index) {
			if constexpr (sizeof...(TParsers) == index.get()) {
				context.restore(saved);
				return Ret(std::unexpected(ParseError{}));
			}
			else {
				auto parse_result = std::get<index.get()>(std::tuple(TParsers{}...))(context);
				if (parse_result) {
					return Ret(*parse_result);
				}
				else {
					return func(index.inc());
				}
			}
		}(IndexWrapper<0>());
	}
};

TEMPLATE_CONCEPT(VariantParserFunc)

template<class T>
concept VariantParserConcept = ParserConcept<T> && VariantParserFuncConcept<typename T::FuncType>;

template<ParserConcept... TParsers>
constexpr auto VariantParser(TParsers...) {
	return Parser(VariantParserFunc<TParsers...>{});
}

template<class T, class RetType>
constexpr auto RecursiveParser() {
	return Parser([](ContextConcept auto& context)-> ParseResult<RetType> {
		return T{}(context);
	});
}

template<class P>
constexpr auto NotParser(P) {
	return Parser([](ContextConcept auto& ctx)->ParseResult<Empty> {
		auto result = P{}(ctx);
		if (!result) {
			ctx.next();
			if (!ctx.ended()) {
				return Empty{};
			}
		}
		return std::unexpected(ParseError{});
	});
}