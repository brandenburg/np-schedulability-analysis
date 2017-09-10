#ifndef CACHE_HPP
#define CACHE_HPP

#include <unordered_map>
#include <utility>

// simple cache for memoization (A,B) -> V

template<typename A, typename B, typename V>
class Cache
{
	public:

	typedef std::pair<A, B> Key;

	bool has(A a, B b) const
	{
		Key k{a, b};

		return map.find(k) != map.end();
	}

	bool lookup(A a, B b, V& v)
	{
		Key k{a, b};
		auto it = map.find(k);
		if (it != map.end()) {
			v = it->second;
			return true;
		}
		else
			return false;
	}

	void memoize(A a, B b, const V& v)
	{
		Key k{a, b};
		map.insert({k, v});
	}

	private:

	typedef std::unordered_map<Key, V> Map;

	Map map;
};

namespace std {
	template<typename A, typename B> struct hash< std::pair<A, B> >
    {
		std::size_t operator()(const std::pair<A, B>& p) const
        {
            return std::hash<A>()(p.first) ^ std::hash<B>()(p.second);
        }
    };
}


#endif
