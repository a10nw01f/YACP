#pragma once

#include "utils.h"
#include "context.h"
#include <type_traits>
#include <utility>

template<class F>
class Parser
{
public:
	using FuncType = F;

	constexpr explicit Parser(F&& func) {}
	Parser() = default;

	constexpr auto parse(ContextConcept auto& context) const -> decltype(F{}(context)) {
		return F{}(context);
	}

	constexpr decltype(auto) operator()(ContextConcept auto& context) const{
		return parse(context);
	}

	constexpr decltype(auto) operator()(auto context) const {
		auto copy = context;
		return parse(copy);
	}
};

TEMPLATE_CONCEPT(Parser)