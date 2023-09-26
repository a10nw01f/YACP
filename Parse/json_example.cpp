#include <vector>
#include <array>
#include <iostream>
#include "dsl.h"
#include "common_parsers.h"

namespace {
	using namespace dsl;

	template<int Capacity, class... Ts>
	struct Allocator
	{
		template<class T>
		using Inner = FixedVec<T, Capacity>;

		template<class T>
		constexpr auto&& add(const T& value) {
			return m_Data.get(TypeWrapper<Inner<T>>{}).push_back(value);
		}

		template<class T>
		constexpr auto&& get() {
			return m_Data.get(TypeWrapper<Inner<T>>{});
		}

		Tuple<Inner<Ts>...> m_Data;
	};

	using String = FixedVec<char, 256>;

	struct Var;
	using Arr = DefaultVectorType<Var>;
	using Member = Tuple<String*, Var>;
	using Obj = DefaultVectorType<Member>;

	using VarBase = Variant<Obj*, Arr*, String*, double, bool, std::nullptr_t>;
	struct Var : VarBase {
		using VarBase::Variant;
	};

	struct JsonParseFunc;
	static constexpr auto rescursive_var_p = RecursiveParser<JsonParseFunc, Var>();

	static constexpr auto null_p = "null"_lit >>= [](auto) {return nullptr; };
	static constexpr auto true_p = "true"_lit >>= [](auto) {return true; };
	static constexpr auto false_p = "false"_lit >>= [](auto) {return false; };

	static constexpr auto escaped = R"(\")"_lit;
	static constexpr auto char_p = escaped | ConditionalParser([](auto c) {return c != '"'; });
	static constexpr auto string_p = ("\""_lit >> *char_p >> "\""_lit) >>= [](auto input) {
		auto range = input.get_range();
		return &input.get_allocator().add(String(range.begin() + 1, range.end() - 1));
	};

	constexpr auto frac_p = ("."_lit >> *digit09) >>= [](auto value) {
		double frac = 0.0;
		double mult = 0.1;
		for (auto c : value.get_range() | std::views::drop(1)) {
			frac += (c - '0') * mult;
			mult *= 0.1;
		}

		return frac;
	};

	constexpr auto double_p = int_parser >> (frac_p | (always >>= [](auto) { return 0.0; })) >>= [](auto v)-> double {
		auto&& [n, frac] = v.get_value();
		return n + (n < 0 ? -frac : frac);
	};

	constexpr auto arr_p = wrap_ws("["_lit) >> separate_by(wrap_ws(rescursive_var_p), ","_lit) >> wrap_ws("]"_lit) >>= [](auto v) {
		return &v.get_allocator().get<Arr>().push_back(v.get_value());
	};

	constexpr auto obj_p = wrap_ws("{"_lit) >> separate_by(wrap_ws(string_p) >> ":"_lit >> wrap_ws(rescursive_var_p), ","_lit) >> wrap_ws("}"_lit) >>= [](auto v) {
		return &v.get_allocator().get<Obj>().push_back(v.get_value());
	};

	constexpr auto var_p = arr_p | obj_p | null_p | true_p | false_p | double_p | string_p>>= [](auto result) {
		return result.get_value().visit([&](auto&& v) {
			return Var(v);
		});
	};

	using VarParserType = decltype(var_p);
	struct JsonParseFunc : VarParserType {
		using VarParserType::operator();
	};

	struct JSON {
		constexpr JSON(StringLiteralHelper str) {
			m_Root = var_p(Context(str, 0, m_Allocator)).value();
		}

		Var m_Root;
		Allocator<42, String, Obj, Arr> m_Allocator;
	};

	constexpr static auto json = JSON(R"({
	"value1": true,
	"value2": false,
	"test": null,
	"asd": {
		"arr": [1,2,45.32,[true,false],{"inner":33.45}]
	}
})");

	int Example() {
		std::cout << "\njson example:\n";

		constexpr auto use_as_nttp = ValueWrapper<json>();
		
		int ident = 0;
		auto print = [](auto... ts) {
			(std::cout << ... << ts);
		};
		auto newline = [&]() {
			std::cout << "\n";
			for (int i = 0; i < ident; i++) {
				std::cout << '\t';
			}
		};
		auto print_list = [&](char begin, char end, auto&& list, auto&& print_item) {
			print(begin);
			ident++;
			newline();
			for (int i = 0; i < list.size(); ++i) {
				if (i != 0) {
					print(",");
					newline();
				}
				print_item(list[i]);
			}
			ident--;
			newline();
			print(end);
		};

		json.m_Root.visit([&](this auto&& self, auto v)->void {
			using T = decltype(v);
			if constexpr (std::is_same_v<T, bool>) {
				print(v ? "true" : "false");
			}
			if constexpr (std::is_same_v<T, double>) {
				print(v);
			}
			if constexpr (std::is_same_v<T, Arr*>) {
				print_list('[', ']', *v, [&](auto&& item) {
					item.visit([&](auto v)->void {
						self(v);
					});
				});
			}
			if constexpr (std::is_same_v<T, Obj*>) {
				print_list('{', '}', *v, [&](auto&& item) {
					auto&& [name, value] = item;
					self(name);
					std::cout << ":";
					value.visit([&](auto v)->void {
						self(v);
					});
				});
			}
			if constexpr (std::is_same_v<T, std::nullptr_t>) {
				print("null");
			}
			if constexpr (std::is_same_v<T, String*>) {
				print('"',v->begin(),'"');
			}
		});
		std::cout << '\n';
		return 0;
	}

	auto result = Example();
}