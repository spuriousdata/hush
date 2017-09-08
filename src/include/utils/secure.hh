#ifndef SECURE_HH_
#define SECURE_HH_

#include <limits> // numeric_limits
#include <new> // bad_alloc, "placement" new
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include <sodium.h>

namespace hush {
	namespace secure {
		class SodiumInitException : public std::runtime_error
		{
			using std::runtime_error::runtime_error;
			using std::runtime_error::what;
		};

		template <class T> class SodiumAllocator
		{
		public:
			typedef T              value_type;
			typedef T*             pointer;
			typedef T const *      const_pointer;
			typedef T&             reference;
			typedef T const &      const_reference;
			typedef std::size_t    size_type;
			typedef std::ptrdiff_t difference_type;

			// constructors
			SodiumAllocator() throw() { init(); };
			SodiumAllocator(SodiumAllocator const &) throw() { init(); };
			template <class U> SodiumAllocator(SodiumAllocator<U> const &) throw() { init(); };

			// methods
			template<class U> struct rebind { typedef SodiumAllocator<U> other; };

			// addresses
			pointer address(reference value) const { return &value; };
			const_pointer address(const_reference value) const { return &value; };

			// max
			size_type max_size() const throw() 
			{ 
				return std::numeric_limits<std::size_t>::max() / sizeof(T);
			};

			pointer allocate(size_type num, void const * = 0)
			{
				pointer ret = (pointer)sodium_malloc(num * sizeof(T));
				if (ret == NULL)
					throw std::bad_alloc();
				return ret;
			};

			void construct(pointer p, T const & value)
			{
				new((void*)p)T(value);
			};

			void destroy(pointer p)
			{
				p->~T();
			};

			void deallocate(pointer p, size_type n)
			{
				sodium_free(p);
			}
		private:
			void init() { if (sodium_init() == -1) throw SodiumInitException("Couldn't initialize sodium"); }
		};

		// add comparators for allocator that evaluate as equal
		template <class T1, class T2>
		bool operator== (SodiumAllocator<T1> const &, SodiumAllocator<T2> const &) throw() {
			return true;
		};

		template <class T1, class T2>
		bool operator!= (SodiumAllocator<T1> const &, SodiumAllocator<T2> const &) throw() {
			return false;
		};

		// Secure Types
		using string = std::basic_string<char, std::char_traits<char>, SodiumAllocator<char>>;

		template<typename T> 
		using vector = std::vector<T, SodiumAllocator<T>>;
	};
};

#endif /* SECURE_HH_ */
