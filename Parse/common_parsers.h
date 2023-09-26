#pragma once

#include "dsl.h"

using namespace dsl;

constexpr auto single_char = cond > [](auto) {return true; } >>= [](auto&& result) { return result.get_range()[0]; };
constexpr auto char_range_parser = single_char >> ":"_lit >> single_char;

template<StringLiteralHelper str>
constexpr auto operator""_rp() {
	constexpr auto char_range = *char_range_parser(str.context());
	return ConditionalParser([](auto c) {
		return c >= char_range.get(IndexWrapper<0>{}) && c <= char_range.get(IndexWrapper<1>{});
	});
}

constexpr auto ws = *(cond > [](auto c) {
	return c == ' ' || c == '\n' || c == '\t';
});

constexpr auto wrap_ws = [](auto v) { return ws >> v >> ws; };

constexpr auto alpha = "a:z"_rp | "A:Z"_rp;
constexpr auto digit09 = "0:9"_rp;
constexpr auto digit19 = "1:9"_rp;

constexpr auto underscore = "_"_lit;

constexpr auto ident = underscore | alpha >> *(alpha | digit09 | underscore) >>= [](auto&& result) {
	return hash_str(std::string_view(result.get_range()));
};

constexpr auto separate_by = [](auto parser, auto separator) {
	using Main = decltype(parser);
	using Separator = decltype(separator);
	return Parser([](ContextConcept auto& context) {
		using ParserRet = decltype(Main{}(context))::value_type;
		DefaultVectorType<ParserRet> result;

		auto ret = Main{}(context);
		if (ret) {
			result.push_back(*ret);
			while (true) {
				auto saved = context.save();
				if (!Separator{}(context)) {
					context.restore(saved);
					break;
				}
				if (auto ret = Main{}(context); ret) {
					result.push_back(*ret);
				}
				else {
					context.restore(saved);
					break;
				}
			}
		}

		return ParseResult<decltype(result)>(result);
	});
};

constexpr auto int_parser = "0"_lit | (+"-"_lit >> digit19 >> *digit09) >>= [](auto&& arg) {
	int mult = 1;
	int result = 0;
	for (auto c : arg.get_range()) {
		if (c == '-') {
			mult = -1;
		}
		else {
			result *= 10;
			result += c - '0';
		}
	}

	return result * mult;
};

template<char... c>
constexpr auto operator""_ix() {
	constexpr char str[] = { c..., 0 };
	return IndexWrapper<*int_parser(StringLiteralHelper(str).context())>();
}