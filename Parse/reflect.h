#pragma once

#include <ranges>
#include "utils.h"

using Identifier = uint32_t;

template<Identifier V>
using Identifier_t = ValueWrapper<V>;

constexpr auto ResolveIdentifier(auto v) {
	static_assert(std::is_same_v<decltype(v), struct Err>, "could not resolve identifier");
}

#define	CONCAT_IMPL(x,y) x##y
#define CONCAT(x,y) CONCAT_IMPL(x,y)

#define REFLECT_MEMBER(member) static constexpr auto ResolveIdentifier(Identifier_t<hash_str(#member)>) { return &Self::member; }
#define REPEAT_MEMBERS1(member) REFLECT_MEMBER(member) REPEAT_MEMBERS0 
#define REPEAT_MEMBERS0(member) REFLECT_MEMBER(member) REPEAT_MEMBERS1
#define REPEAT_MEMBERS0_END
#define REPEAT_MEMBERS1_END

#define REFLECT_MEMBERS(self, members) using Self = self; CONCAT(REPEAT_MEMBERS0 members, _END)

#define REFLECT_TYPE(ident) constexpr auto ResolveIdentifier(Identifier_t<hash_str(#ident)>){ return TypeWrapper<ident>(); }

template<class T>
constexpr decltype(auto) GetMember(T&& obj, auto ident) {
	constexpr auto member = std::decay_t<T>::ResolveIdentifier(decltype(ident){});
	return FWD(obj).*member;
}

using GlobalScope = decltype([](auto v) { return ResolveIdentifier(v); });

#define SCOPE [](auto v) {return ResolveIdentifier(v);}
#define REFLECT_IDENTIFIER(name) 	constexpr auto ResolveIdentifier(Identifier_t<hash_str(#name)>) {return name;}

