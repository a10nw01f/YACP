# YACP - Yet Another Compile Time Parser
YACP is a lightweight C++ compile-time parser combinator designed to work with C++23. It provides a DSL to create parsers that operate at compile time or runtime.

Currently this project is meant for educational purposes and should probably not be used in production.

## Features
**Compile-Time Parsing:** YACP enables you to define parsers that operate at compile time, allowing you to create functions, types and tree data structures from a custom syntax.

**Parser Combinators:** YACP provides a set of parser combinators that you can use to build complex parsers from simple ones. Create combinators like sequence, choice (variant), many (repeat/list), transform and others by using a domain specific language.

**Integration with C++ Reflection:** YACP includes examples demonstrating how compile-time parsers can be integrated with reflection. It includes a mechanism to reflect an identifier and shows how to generate reflection metadata with compile time parsers.

## Examples
### SQL
```cpp
auto cats = std::vector<Cat>{ Cat{"mizi", 1337, 17} };
auto result = "SELECT name, id WHERE age > 2"_FROM(cats);
```
### Derive
```cpp
DERIVE((ATTRIBS, MEMBERS), MyStruct,
struct 
[[custom::serialize]]
[[custom::debug_name(my object)]] 
MyStruct {
	int x;
	double y;
};)

auto members = GetMembers(TypeWrapper<MyStruct>()).value();
auto attributes = GetAttributes(TypeWrapper<MyStruct>{}).value();
```
### Path
```cpp
Game game;
"players:character.items:texture.width|sqr|print"_of(game, SCOPE);
```
### Struct 
```cpp
STRUCT(Person, {
  name: string
  age: int
  item: Item
})
```
### 
### JSON
```cpp
constexpr static auto json = JSON(R"({
	"value1": true,
	"value2": false,
	"test": null,
	"asd": {
		"arr": [1,2,45.32,[true,false],{"inner":33.45}]
	}
})");
```

## License
This project is licensed under the MIT License - see the LICENSE file for details.

Enjoy parsing at compile time with YACP, and feel free to reach out if you have any questions or feedback!
