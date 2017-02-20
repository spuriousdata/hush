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
			typedef const T*       const_pointer;
			typedef T&             reference;
			typedef const T&       const_reference;
			typedef std::size_t    size_type;
			typedef std::ptrdiff_t difference_type;

			// constructors
			SodiumAllocator() throw() { init(); };
			SodiumAllocator(const SodiumAllocator&) throw() { init(); };
			template <class U> SodiumAllocator(const SodiumAllocator<U>&) throw() { init(); };

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

			pointer allocate(size_type num, const void* = 0)
			{
				pointer ret = (pointer)sodium_malloc(num * sizeof(T));
				if (ret == NULL)
					throw std::bad_alloc();
				return ret;
			};

			void construct(pointer p, const T& value)
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
		bool operator== (const SodiumAllocator<T1>&, const SodiumAllocator<T2>&) throw() {
			return true;
		};

		template <class T1, class T2>
		bool operator!= (const SodiumAllocator<T1>&, const SodiumAllocator<T2>&) throw() {
			return false;
		};

		// Secure Types
		typedef std::basic_string<char, std::char_traits<char>, SodiumAllocator<char>> string;
		template<typename T> using vector = std::vector<T, SodiumAllocator<T>>;
	};
};

#endif /* SECURE_HH_ */
