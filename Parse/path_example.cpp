#include "dsl.h"
#include "common_parsers.h"
#include "reflect.h"
#include <vector>
#include <iostream>

namespace {
	constexpr auto at_leat_1 = [](auto parser) {
		return Parser([](ContextConcept auto& context) {
			auto saved = context.save();
			auto ret = decltype(parser){}(context);
			using T = decltype(ret);
			if (ret) {
				if (ret.value().size() == 0) {
					context.restore(saved);
					return T(std::unexpected(ParseError{}));
				}
			}

			return ret;
		});
	};

	constexpr auto path_parser = at_leat_1(separate_by(wrap_ws(ident), "."_lit));

	struct S {
		int x;
		REFLECT_MEMBERS(S,(x))
	};

	template<DefaultVectorType<Identifier> path>
	constexpr decltype(auto) GetPathImpl(auto&& v) {
		return [](this auto&& func, auto&& obj, auto index) -> decltype(auto) {
			if constexpr (index.get() == path.size()) {
				return (decltype(obj)&)obj;
			}
			else {
				return func(GetMember(obj, Identifier_t<path[index.get()]>()), index.Next());
			}
		}(FWD(v), 0_ix);
	}

	template<StringLiteralHelper str>
	constexpr auto operator""_in() {
		constexpr auto path = path_parser(str.context()).value();
		return [](auto&& v)->decltype(auto) {
			return GetPathImpl<path>(FWD(v));
		};
	}

	struct Pipe { Identifier m_Action = 0;};
	constexpr auto action = "|"_lit >> ident >>= [](auto&& value) { return Pipe{ value.get_value() }; };
	struct IterateTag { char c = 0; };
	constexpr auto iterate = ":"_lit >>= [](auto) { return IterateTag{}; };
	constexpr auto extended_path_parser = *(action | iterate | path_parser);

	template<StringLiteralHelper str>
	constexpr auto operator""_of() {
		static constexpr auto path = extended_path_parser(str.context()).value();
		return [](auto&& obj, auto scope) {
			using Scope = decltype(scope);
			[](this auto&& func, auto&& obj, auto index) {
				if constexpr (index.get() == path.size()) {
					return;
				}
				else {
					constexpr auto current = path[index.get()];
					constexpr auto iterate = current.get_if<IterateTag>();
					constexpr auto pipe = current.get_if<Pipe>();
					constexpr auto members = current.get_if<DefaultVectorType<Identifier>>();
					if constexpr (iterate) {
						for (auto&& v : obj) {
							func(v, index.Next());
						}
					}
					else if constexpr (pipe) {
						constexpr auto action = Scope{}(Identifier_t<pipe->m_Action>());
						func(action(FWD(obj)), index.Next());
					}
					else {
						func(GetPathImpl<*members>(FWD(obj)), index.Next());
					}
				}
			}(FWD(obj), 0_ix);
		};
	}

	struct Texture {
		int width, height;
		REFLECT_MEMBERS(Texture,(width)(height))
	};

	struct Item {
		Texture texture;
		REFLECT_MEMBERS(Item,(texture))
	};

	struct Character {
		std::vector<Item> items;
		REFLECT_MEMBERS(Character,(items))
	};

	struct Player {
		Character character;
		REFLECT_MEMBERS(Player,(character))
	};

	struct Game {
		std::vector<Player> players;
		REFLECT_MEMBERS(Game,(players))
	};

	constexpr auto sqr = [](auto v) {
		return v * v;
	};

	REFLECT_IDENTIFIER(sqr)

	constexpr auto print = [](auto v) {
		std::cout << v;
		return v;
	};

	REFLECT_IDENTIFIER(print)

	int Example() {
		std::cout << "\npath example:\n";
		Game game;
		game.players.emplace_back().character.items.emplace_back().texture.width = 10;
		"players:character.items:texture.width|sqr|print"_of(game, SCOPE);

		Texture texture{ 2, 2 };
		"width"_in(texture) = 42;
		
		std::cout << "\ntexture width: " << "width"_in(texture) << '\n';

		return 0;
	}

	auto result = Example();
}