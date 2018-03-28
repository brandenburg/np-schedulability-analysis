#ifndef UTIL_HPP
#define UTIL_HPP

#include <iostream>
#include <fstream>
#include <algorithm>

namespace NP {

	template<class InputIt, class T>
	bool contains(InputIt first, InputIt last, const T& val)
	{
		return std::find(first, last, val) != last;
	}

	template<class Cont, class T>
	bool contains(const Cont& container, const T& val)
	{
		return contains(container.begin(), container.end(), val);
	}

	template<class T>
	void fdump(const std::string& fname, const T& obj)
	{
		auto f = std::ofstream(fname,  std::ios::out);
		f << obj;
		f.close();
	}
}

#endif
