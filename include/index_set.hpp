#ifndef INDEX_SET_H
#define INDEX_SET_H

namespace NP {

		class Index_set
		{
			public:

			typedef std::vector<bool> Set_type;

			// new empty job set
			Index_set() : the_set() {}

			// derive a new set by "cloning" an existing set and adding an index
			Index_set(const Index_set& from, std::size_t idx)
			: the_set(std::max(from.the_set.size(), idx + 1))
			{
				std::copy(from.the_set.begin(), from.the_set.end(), the_set.begin());
				the_set[idx] = true;
			}

			// create the diff of two job sets (intended for debugging only)
			Index_set(const Index_set &a, const Index_set &b)
			: the_set(std::max(a.the_set.size(), b.the_set.size()), true)
			{
				auto limit = std::min(a.the_set.size(), b.the_set.size());
				for (unsigned int i = 0; i < limit; i++)
					the_set[i] = a.contains(i) ^ b.contains(i);
			}

			bool operator==(const Index_set &other) const
			{
				return the_set == other.the_set;
			}

			bool operator!=(const Index_set &other) const
			{
				return the_set != other.the_set;
			}

			bool contains(std::size_t idx) const
			{
				return the_set.size() > idx && the_set[idx];
			}

			bool includes(std::vector<std::size_t> indices) const
			{
				for (auto i : indices)
					if (!contains(i))
						return false;
				return true;
			}

			bool is_subset_of(const Index_set& other) const
			{
				for (unsigned int i = 0; i < the_set.size(); i++)
					if (contains(i) && !other.contains(i))
						return false;
				return true;
			}

			std::size_t size() const
			{
				std::size_t count = 0;
				for (auto x : the_set)
					if (x)
						count++;
				return count;
			}

			void add(std::size_t idx)
			{
				if (idx >= the_set.size())
					the_set.resize(idx + 1);
				the_set[idx] = true;
			}

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Index_set& s)
			{
				bool first = true;
				stream << "{";
				for (auto i = 0; i < s.the_set.size(); i++)
					if (s.the_set[i]) {
						if (!first)
							stream << ", ";
						first = false;
						stream << i;
					}
				stream << "}";

				return stream;
			}

			private:

			Set_type the_set;

			// no accidental copies
			Index_set(const Index_set& origin) = delete;
		};
}

#endif
