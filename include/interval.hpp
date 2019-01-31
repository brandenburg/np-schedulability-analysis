#ifndef INTERVAL_HPP
#define INTERVAL_HPP

#include <cassert>
#include <ostream>
#include <memory>
#include <vector>

template<class T> class Interval {
	T a, b;

	public:

	Interval(const T &a, const T &b)
	{
		if (a > b) {
			this->a = b;
			this->b = a;
		} else {
			this->a = a;
			this->b = b;
		}
	}

	Interval(const std::pair<T, T> p)
	: Interval(p.first, p.second)
	{
	}

	Interval(const Interval<T>& orig)
	: a(orig.a)
	, b(orig.b)
	{
	}

	const T& from() const
	{
		return a;
	}

	const T& min() const
	{
		return a;
	}

	const T& starting_at() const
	{
		return a;
	}

	const T& until() const
	{
		return b;
	}

	const T& upto() const
	{
		return b;
	}

	const T& max() const
	{
		return b;
	}

	bool contains(const Interval<T>& other) const
	{
		return from() <= other.from() && other.until() <= until();
	}

	bool contains(const T& point) const
	{
		return from() <= point && point <= until();
	}

	bool disjoint(const Interval<T>& other) const
	{
		return other.until() < from() || until() < other.from();
	}

	bool intersects(const Interval<T>& other) const
	{
		return not disjoint(other);
	}

	bool operator==(const Interval<T>& other) const
	{
		return other.from() == from() && other.until() == until();
	}

	void operator +=(T offset)
	{
		a += offset;
		b += offset;
	}

	Interval<T> operator+(const Interval<T>& other) const
	{
		return {a + other.a, b + other.b};
	}

	Interval<T> operator+(const std::pair<T, T>& other) const
	{
		return {a + other.first, b + other.second};
	}

	Interval<T> merge(const Interval<T>& other) const
	{
		return Interval<T>{std::min(from(), other.from()),
		                   std::max(until(), other.until())};
	}

	void widen(const Interval<T>& other)
	{
		a = std::min(from(), other.from());
		b = std::max(until(), other.until());
	}

	Interval<T> operator|(const Interval<T>& other) const
	{
		return merge(other);
	}

	void operator|=(const Interval<T>& other)
	{
		widen(other);
	}

	void lower_bound(T lb)
	{
		a = std::max(lb, a);
	}

	void extend_to(T b_at_least)
	{
		b = std::max(b_at_least, b);
	}

	T length() const
	{
		return until() - from();
	}
};

template<class T> std::ostream& operator<< (std::ostream& stream, const Interval<T>& i)
{
	stream << "I(" << i.from() << ", " << i.until() << ")";
	return stream;
}



template<class T, class X, Interval<T> (*map)(const X&)> class Interval_lookup_table {
	typedef std::vector<std::reference_wrapper<const X>> Bucket;

	private:

	std::unique_ptr<Bucket[]> buckets;
	const Interval<T> range;
	const T width;
	const unsigned int num_buckets;


	public:

	std::size_t bucket_of(const T& point) const
	{
		if (range.contains(point)) {
			return static_cast<std::size_t>((point - range.from()) / width);
		} else if (point < range.from()) {
			return 0;
		} else
			return num_buckets - 1;
	}

	Interval_lookup_table(const Interval<T>& range, T bucket_width)
	: range(range)
	, width(std::max(bucket_width, static_cast<T>(1)))
	, num_buckets(1 + std::max(
	                  static_cast<std::size_t>(range.length() / this->width),
	                  static_cast<std::size_t>(1)))
	{
		buckets = std::make_unique<Bucket[]>(num_buckets);
	}

	void insert(const X& x)
	{
		Interval<T> w = map(x);
		auto a = bucket_of(w.from()), b = bucket_of(w.until());
		assert(a < num_buckets);
		assert(b < num_buckets);
		for (auto i = a; i <= b; i++)
			buckets[i].push_back(x);
	}

	const Bucket& lookup(T point) const
	{
		return buckets[bucket_of(point)];
	}

	const Bucket& bucket(std::size_t i) const
	{
		return buckets[i];
	}

};

#endif
