#pragma once

#include <utility>
#include <string_view>

#define TEMPLATE_CONCEPT_EX(name, tname) template<class T>concept name##Concept = requires(T arg) {{ tname{ arg } } -> std::same_as<T>;};
#define TEMPLATE_CONCEPT(name) TEMPLATE_CONCEPT_EX(name, name)

#define FWD(v) std::forward<decltype(v)>(v)

template<class... Ts> struct TypeList{};

template<class T> struct TypeWrapper {
	T get() const;
	using Type = T;
};

template<template<class...> class L, class... Ts>
constexpr auto TupleSize(const L<Ts...>&) {
	return sizeof...(Ts);
}

template<class T, class... Ts>
constexpr auto First(TypeList<T, Ts...>) {
	return TypeWrapper<T>{};
}

template<class T, class... Ts>
constexpr auto Push(TypeWrapper<T>, TypeList<Ts...>) {
	return TypeList<T, Ts...>{};
}

template<class T, class... Ts>
constexpr auto Push(TypeList<Ts...>, TypeWrapper<T>) {
	return TypeList<Ts..., T>{};
}

template<class T, class... Ts>
constexpr auto Pop(TypeList<T, Ts...>) {
	return TypeList<Ts...>{};
}

template<class... Ts>
constexpr auto Filter(auto list, auto filter, TypeList<Ts...> acc = TypeList{}) {
	if constexpr (TupleSize(list) == 0) {
		return acc;
	}
	else {
		constexpr auto first = First(list);
		if constexpr (filter(first)) {
			return Filter(Pop(list), filter, Push(acc, first));
		}
		else {
			return Filter(Pop(list), filter, acc);
		}
	}
}

template<template<class...> class Out, template<class...> class In, class... Ts>
auto Rename(In<Ts...>) -> Out<Ts...>;

template<auto V>
struct ValueWrapper {
	constexpr auto get() const { return V; }
};

template<int I>
struct IndexWrapper : ValueWrapper<I> {
	constexpr auto Next() const { return IndexWrapper<I + 1>(); }
};

template<class T, class... Ts>
constexpr auto Contains(TypeWrapper<T>, TypeList<Ts...>) {
	return (... || std::is_same_v<T, Ts>);
}

template <class... Ts, class... Us>
constexpr auto Unique(TypeList<Ts...> input, TypeList<Us...> output = TypeList{}) {
	if constexpr (sizeof...(Ts) == 0) {
		return output;
	}
	else {
		auto first = First(input);
		if constexpr (Contains(first, output)) {
			return Unique(Pop(input), output);
		}
		else {
			return Unique(Pop(input), Push(first, output));
		}
	}
}

template<template<class...> class L, class... Ts, class... Us>
constexpr auto Concat(L<Ts...>, L<Us...>) -> L<Ts..., Us...> {
	return L<Ts..., Us...>();
}

template<class T, class U>
using Concat_t = decltype(Concat(std::declval<T>(), std::declval<U>()));

constexpr auto hash_str(std::string_view s) -> uint32_t {
	uint32_t hash = 0;
	for (auto i : s) {
		hash *= 0x811C9DC5;
		hash ^= static_cast<uint32_t>(i);
	}
	return hash;
}

template<class F, int... Is>
constexpr decltype(auto) Expand(F&& func, std::index_sequence<Is...>) {
	return func(IndexWrapper<Is>{}...);
}
