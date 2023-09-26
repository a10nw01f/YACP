#pragma once

#include <stdint.h>
#include <utility>
#include "utils.h"

template<class T, uint32_t Capacity>
struct FixedVec {
    constexpr decltype(auto) operator[](this auto&& self, uint32_t index) {
        return std::forward_like<decltype(self)>(self.m_Data[index]);
    }

    constexpr auto size() const { return m_Size; }

    constexpr T& push_back(auto&& v) { 
		m_Data[m_Size] = FWD(v); 
		return m_Data[m_Size++];
	}

	constexpr auto begin(this auto&& self) {
		return self.m_Data;
	}

	constexpr auto end(this auto&& self) {
		return &self.m_Data[self.m_Size];
	}

	constexpr FixedVec() = default;

	template<int I>
	constexpr FixedVec(const T(&arr)[I]): m_Size(I){
		for (int i = 0; i < I; ++i) {
			m_Data[i] = arr[i];
		}
	}

	constexpr FixedVec(auto begin, auto end) {
		for (auto itr = begin; itr != end; ++itr) {
			push_back(*itr);
		}
	}


    T m_Data[Capacity] = {};
    int m_Size = 0;
};

template<class T, int I>
struct TupleLeaf{
	constexpr decltype(auto) get(this auto&& self, IndexWrapper<I>) {
		return std::forward_like<decltype(self)>(self.TupleLeaf::m_Value);
	}
	constexpr decltype(auto) get(this auto&& self, TypeWrapper<T>) {
		return std::forward_like<decltype(self)>(self.TupleLeaf::m_Value);
	}
    T m_Value;
};

template<class... Leafs>
struct TupleImpl : Leafs... {
    using Leafs::get...;
};

template<class... Ts, int... Is>
auto create_leafs(TypeList<Ts...>, std::index_sequence<Is...>) {
    return TupleImpl<TupleLeaf<Ts, Is>...>{};
}

template<class... Ts>
struct Tuple : decltype(create_leafs(TypeList<Ts...>{}, std::index_sequence_for<Ts...>{})) {};

namespace std
{
	template<class... Ts>
	struct tuple_size<::Tuple<Ts...>> : std::tuple_size<std::tuple<Ts...>> {};

	template<int I, class... Ts>
	struct tuple_element<I, ::Tuple<Ts...>> : std::tuple_element<I, std::tuple<Ts...>> {};

	template<int I, class... Ts>
	struct tuple_element<I, const ::Tuple<Ts...>> : std::tuple_element<I, const std::tuple<Ts...>> {};
}

template<std::size_t Index, class... Ts>
constexpr std::tuple_element_t<Index, ::Tuple<Ts...>>& get(::Tuple<Ts...>& tup)
{
	return tup.get(IndexWrapper<Index>{});
}

template<std::size_t Index, class... Ts>
constexpr const std::tuple_element_t<Index, ::Tuple<Ts...>>& get(const ::Tuple<Ts...>& tup)
{
	return tup.get(IndexWrapper<Index>{});
}

TEMPLATE_CONCEPT(Tuple)

struct VariantEnd {
	constexpr VariantEnd() = default;
	constexpr VariantEnd(VariantEnd, int) {}
};

template<class T, class... Ts>
struct VariantImpl {
	static constexpr auto get_rest_type() {
		if constexpr (sizeof...(Ts) == 0) {
			return TypeWrapper<VariantEnd>{};
		}
		else {
			return TypeWrapper<VariantImpl<Ts...>>{};
		}
	}

	constexpr auto get_index() const {
		return sizeof...(Ts);
	}

	using Rest = decltype(get_rest_type().get());

	union {
		T m_Value;
		Rest m_Rest;
	};

	constexpr VariantImpl() = default;

	constexpr VariantImpl(auto&& value, int& index) requires(std::is_same_v<T, std::remove_cvref_t<decltype(value)>>) : m_Value(FWD(value)) {
		index = sizeof...(Ts);
	}

	constexpr VariantImpl(auto&& value, int& index) : m_Rest(FWD(value), index) {}

	constexpr decltype(auto) visit(this auto&& self, auto&& func, int index){
		if (index == self.get_index()) {
			return FWD(func)(std::forward_like<decltype(self)>(self.m_Value));
		}
		else {
			if constexpr (!std::is_same_v<Rest, VariantEnd>) {
				return FWD(self).m_Rest.visit(FWD(func), index);
			}
		}
	}

	template<class U>
	constexpr auto* get_if(this auto&& self, int index) {
		if constexpr (std::is_same_v<T, U>) {
			if (index == self.get_index()) {
				return &self.m_Value;
			}
			else {
				return static_cast<decltype(&self.m_Value)>(nullptr);
			}
		}
		else {
			return self.m_Rest.get_if<U>(index);
		}
	}
};

template<class... Ts>
struct Variant {
	using Impl = VariantImpl<Ts...>;
	int m_Index = -1;
	Impl m_Impl;

	constexpr Variant() : m_Impl(VariantEnd{}, m_Index) {}
	constexpr Variant(auto&& value) : m_Index(-1), m_Impl(FWD(value), m_Index) {}

	constexpr ~Variant() = default;
	constexpr Variant(const Variant& other) = default;
	constexpr Variant(Variant&& other) noexcept = default;
	constexpr Variant(Variant& other) : Variant(static_cast<const Variant&>(other)) {}
	constexpr Variant& operator=(const Variant& other) = default;
	constexpr Variant& operator=(Variant&& other) noexcept = default;

	constexpr decltype(auto) visit(this auto&& self, auto&& func) {
		return FWD(self).m_Impl.visit(FWD(func), self.m_Index);
	}

	template<class T>
	constexpr auto* get_if(this auto&& self) {
		return self.m_Impl.get_if<T>(self.m_Index);
	}
};