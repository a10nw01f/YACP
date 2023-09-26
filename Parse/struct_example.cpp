#include "dsl.h"
#include "reflect.h"
#include "common_parsers.h"
#include <string>
#include <iostream>

REFLECT_TYPE(int)
REFLECT_TYPE(bool)

namespace std {
	REFLECT_TYPE(string)
}

namespace {
	using namespace dsl;
	using namespace std;
	using std::ResolveIdentifier;

	template<StringLiteralHelper str>
	constexpr auto operator""_id() {
		return Identifier_t<hash_str(std::string_view(str))>{};
	}

	constexpr auto struct_parser = "{"_lit >> ::ws >> *(ident >> ::ws >> ":"_lit >> wrap_ws(ident)) >> "}"_lit;

	template<Identifier id, TypeWrapper type>
	struct StructMember {
		using Type = decltype(type.get());
		Type m_Value;

		static constexpr auto ResolveIdentifier(Identifier_t<id>) { return &StructMember::m_Value; }
		constexpr auto&& operator[](this auto&& self, Identifier_t<id> ident) {
			return GetMember(FWD(self), ident);
		}
	};

	template<class... Ts>
	struct CombineMembers : Ts... {
		using Ts::ResolveIdentifier...;
	};

	template<StringLiteralHelper str, class Scope>
	constexpr auto CreateStruct(Scope) {
		constexpr auto members = struct_parser(str.context()).value();
		constexpr auto types = Expand([](auto... indices) {
			return TypeWrapper<
				CombineMembers<
					StructMember<
						members[indices.get()].get(0_ix),
						Scope{}(Identifier_t<members[indices.get()].get(1_ix)>{})
					>...
				>
			>{};
		}, std::make_index_sequence<members.size()>());
		return types;
	}

#define QUOTE(...) #__VA_ARGS__
#define STRUCT(name, ...) using name = decltype(CreateStruct<QUOTE(__VA_ARGS__)>(SCOPE).get()); REFLECT_TYPE(name)

	STRUCT(Item, {
		name: string
		unique: bool
	})

	STRUCT(Person, {
		name: string
		age: int
		item: Item
	})

	int Example() {
		std::cout << "\nstruct example:\n";
		Person bilbo{ 
			"Bilbo", 
			111, 
			{"the one ring", true}
		};

		std::cout << bilbo["item"_id]["name"_id] << '\n';

		return 0;
	}
	int result = Example();


}
