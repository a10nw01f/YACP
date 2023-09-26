#include "reflect.h"
#include "dsl.h"
#include "common_parsers.h"
#include <iostream>

#define EVAL(...)  EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__

#define COUNT_ARGS_IMPL(_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
#define COUNT_ARGS(...) EVAL(COUNT_ARGS_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define M_FOR_EACH_0(A1, ACTN, E) E
#define M_FOR_EACH_1(A1, ACTN, E) ACTN(A1, E)
#define M_FOR_EACH_2(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_1(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_3(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_2(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_4(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_3(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_5(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_4(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_6(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_5(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_7(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_6(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_8(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_7(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_9(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_8(A1, ACTN, __VA_ARGS__)
#define M_FOR_EACH_10(A1, ACTN, E, ...) ACTN(A1, E) M_FOR_EACH_9(A1, ACTN, __VA_ARGS__)

#define M_FOR_EACH(A1, ACTN, ...) EVAL(CONCAT(M_FOR_EACH_, COUNT_ARGS(__VA_ARGS__)) (A1, ACTN, __VA_ARGS__))

#define CALL_MACRO(args,m) m args

#define DERIVE(macros, type, ...)\
__VA_ARGS__ \
M_FOR_EACH((type, #__VA_ARGS__), CALL_MACRO, EVAL macros)

using namespace dsl;

constexpr auto attrib_parser = "[["_lit >> *(!"]]"_lit) >> "]]"_lit >>= [](auto&& value) {
	auto range = value.get_range();
	auto end = range.end() - 2;
	FixedVec<char, 32> str;
	for(auto itr = range.begin() + 2; itr != end; ++itr) {
		str.push_back(*itr);
	}

	return str;
};

constexpr auto attribs_parser = wrap_ws("struct"_lit | "class"_lit) >> *wrap_ws(attrib_parser);

#define ATTRIBS(type, str) \
constexpr auto GetAttributes(TypeWrapper<type>){\
return attribs_parser(StringLiteralHelper(str).context()); \
}

constexpr auto member_parser = ident >> ws >> ident >> ws >> ";"_lit >> ws;
constexpr auto members_parser = *(!"{"_lit) >> "{"_lit >> ws >> (*member_parser) >> "}"_lit;

#define MEMBERS(type, str) \
constexpr auto GetMembers(TypeWrapper<type>){\
return members_parser(StringLiteralHelper(str).context());; \
}

namespace {

	DERIVE((ATTRIBS, MEMBERS), MyStruct,
	struct 
	[[custom::serialize]]
	[[custom::debug_name(my object)]] 
	MyStruct {
		int x;
		double y;
	};)

	int Example() {
		auto members = GetMembers(TypeWrapper<MyStruct>()).value();
		auto atributes = GetAttributes(TypeWrapper<MyStruct>{}).value();
		std::cout << "\nderive example:\n";
		std::cout << "struct attributes:\n";
		for (auto& attribute : atributes) {
			std::cout << attribute.m_Data << '\n';
		}
		return 0;
	}

	int x = Example();
}