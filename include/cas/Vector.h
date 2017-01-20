#pragma once

/*
#include <vector>
#include <string>

namespace cas
{
	template<typename T>
	struct dependent_false : std::false_type {};

	template<typename T>
	class array
	{
	public:
		static_assert(dependent_false<T>::value, "Only specializations of cas::array may be used.");
	};

	template<>
	class array<bool> : std::vector<bool>
	{
	public:
	};

	template<>
	class array<char> : std::vector<char>
	{
	public:
	};

	template<>
	class array<int> : std::vector<int>
	{
	public:
	};

	template<>
	class array<float> : std::vector<float>
	{
	public:
	};

	template<>
	class array<std::string>
	{
	public:
		string& at(uint pos);
		const string& at(uint pos) const;
	};
}
*/
