#pragma once

#include "context.h"

struct StringLiteralHelper {
	int m_Size;
	const char* m_Data;

	template<int I>
	constexpr StringLiteralHelper(const char(&arr)[I]) :
		m_Size(I - 1),
		m_Data(arr)
	{
	}

	constexpr auto begin() const {
		return m_Data;
	}

	constexpr auto end() const {
		return m_Data + m_Size;
	}

	constexpr auto context() const {
		return Context(*this, 0);
	}

	constexpr auto size() const { return m_Size; }
};

