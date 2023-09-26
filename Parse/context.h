#pragma once

#include <ranges>
#include "utils.h"

struct EmptyAllocator {};
constexpr auto empty_allocator = EmptyAllocator{};

template<class T, class U = int, class Allocator = const EmptyAllocator>
class Context {
public:
	using Iter = decltype(std::declval<T>().begin());

	Context(const Context&) = default;

	explicit constexpr Context(const T& input, U eof = 0, Allocator& allocator = empty_allocator) :
		m_Input(input),
		m_Eof(eof),
		m_Current(m_Input.begin()),
		m_Allocator(allocator)
	{}

	constexpr auto current() const {
		using Ret = decltype(*m_Current);
		if (m_Current != m_Input.end()) {
			return *m_Current;
		}
		return eof();
	}

	constexpr auto save() const { return m_Current; }
	constexpr void restore(Iter saved) { m_Current = saved; }
	constexpr auto get_range(Iter start) const
	{
		return std::ranges::subrange(start, m_Current);
	}

	constexpr void next() { 
		if (m_Current != m_Input.end()) {
			++m_Current;
		}
	}

	constexpr auto eof() const { 
		using Ret = decltype(*m_Current); 
		return static_cast<Ret>(m_Eof); 
	}

	constexpr auto ended() const {
		return eof() == current();
	}

	constexpr auto& get_allocator() {
		return m_Allocator;
	}

	T m_Input;
	U m_Eof;
	Iter m_Current;
	Allocator& m_Allocator;
};

TEMPLATE_CONCEPT(Context)
