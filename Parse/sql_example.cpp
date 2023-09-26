#include "create_parsers.h"
#include "dsl.h"
#include "reflect.h"
#include "common_parsers.h"
#include <string>
#include <vector>
#include <iostream>

namespace {
	using namespace dsl;

	constexpr auto ws_indet = wrap_ws(ident);
	constexpr auto ws_int = wrap_ws(int_parser);
	enum class ECompOp {
		Greater,
		Smaller,
		Equal
	};

	struct CompOp {
		ECompOp m_Value = ECompOp::Equal;
		constexpr CompOp() = default;
		constexpr CompOp(ECompOp v) :m_Value(v) {}
	};

	template<StringLiteralHelper str, CompOp op>
	constexpr auto CreateCompOpParser() {
		return LiteralParser<str>() >>= [](auto) {return op; };
	};

	constexpr auto comp_func =
		CreateCompOpParser<">", ECompOp::Greater>() |
		CreateCompOpParser<"=", ECompOp::Equal>() |
		CreateCompOpParser<"<", ECompOp::Smaller>();

	constexpr auto comp_operand = wrap_ws(ident | int_parser);

	constexpr auto sql = "SELECT"_lit >> separate_by(ws_indet, ","_lit) >> "WHERE"_lit >> comp_operand >> comp_func >> comp_operand;


	struct Cat {
		std::string name;
		int id;
		int age;

		REFLECT_MEMBERS(Cat,(name)(id)(age))
	};

	template<ECompOp op>
	constexpr auto GetBinayrOpFunc() {
		if constexpr (op == ECompOp::Greater) {
			return [](auto&& a, auto&& b) {
				return a > b;
			};
		}
		else if constexpr (op == ECompOp::Equal) {
			return [](auto&& a, auto&& b) {
				return a == b;
			};
		}
		else if constexpr (op == ECompOp::Smaller) {
			return [](auto&& a, auto&& b) {
				return a < b;
			};
		}
	}

	template<Variant<int, Identifier> op>
	constexpr auto GetOperandFunc() {
		if constexpr (op.get_if<int>()) {
			return [](auto&&) {
				return *op.get_if<int>();
			};
		}
		else if(op.get_if<Identifier>()){
			return [](auto&& obj)->decltype(auto) {
				return GetMember(obj, Identifier_t<*op.get_if<Identifier>()>());
			};
		}
	}

	template<StringLiteralHelper str>
	constexpr auto operator""_FROM() {
		constexpr auto parse_result = sql(str.context()).value();
		constexpr auto idents = parse_result.get(0_ix);

		using Op1 = decltype(GetOperandFunc<parse_result.get(1_ix)>());
		using BinaryFunc = decltype(GetBinayrOpFunc<parse_result.get(2_ix).m_Value>());
		using Op2 = decltype(GetOperandFunc<parse_result.get(3_ix)>());

		return [](auto& values) {
			using T = std::decay_t<decltype(values)>::value_type;
			
			constexpr auto filter = std::views::filter([](auto& obj) {
				return BinaryFunc{}(Op1{}(obj), Op2{}(obj));
			});
			
			constexpr auto select_tuple = std::views::transform([](auto& obj) {
				return Expand([&](auto... indices) {
					return std::tuple(GetMember(obj, ValueWrapper<idents[indices.get()]>{})...);
				}, std::make_index_sequence<idents.size()>());
			});
			constexpr auto pipe = filter | select_tuple | std::ranges::to<std::vector>();
			return pipe(values);
		};
	}

	int Example() {
		std::cout << "\nsql example:\n";
		auto cats = std::vector<Cat>{ Cat{"mizi", 1337, 17} };
		auto result = "SELECT name, id WHERE age > 2"_FROM(cats);

		std::cout << std::get<0>(result[0]) << '\n';

		return 0;
	}

	int result = Example();

}