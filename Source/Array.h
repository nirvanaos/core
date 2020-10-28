/// \file Array.h
#ifndef NIRVANA_CORE_ARRAY_H_
#define NIRVANA_CORE_ARRAY_H_

#include "core.h"

namespace Nirvana {
namespace Core {

/// \brief Fixed size array.
template <class T, template <class> class Al>
class Array
{
public:
	Array () :
		begin_ (nullptr),
		end_ (nullptr)
	{}

	Array (Array&& src) :
		begin_ (src.begin_),
		end_ (src.end_)
	{
		src.begin_ = src.end_ = nullptr;
	}

	/// \brief Allocate memory and initialize it with zeroes.
	void allocate (size_t size)
	{
		assert (!begin_ && !end_);
		begin_ = Al <T>::allocate (size, nullptr, Memory::ZERO_INIT);
		end_ = begin_ + size;
	}

	/// \brief Allocate and call default constructors. Rarely used.
	void construct (size_t size)
	{
		assert (!begin_ && !end_);
		if (size) {
			begin_ = Al <T>::allocate (size);
			end_ = begin_ + size;
			T* p = begin_;
			do {
				new (p) T ();
			} while (end_ != ++p);
		}
	}

	~Array ()
	{
		if (begin_ != end_) {
			T* p = begin_;
			do {
				p->~T ();
			} while (end_ != ++p);
			Al <T>::deallocate (begin_, (end_ - begin_));
		}
	}

	size_t size () const
	{
		return end_ - begin_;
	}

	const T* begin () const
	{
		return begin_;
	}

	T* begin ()
	{
		return begin_;
	}

	const T* end () const
	{
		return end_;
	}

	T* end ()
	{
		return end_;
	}

	const T* cbegin () const
	{
		return begin ();
	}

	const T* cend () const
	{
		return end ();
	}

	const T& operator [] (size_t i) const
	{
		assert (i < (size_t)(end_ - begin_));
		return begin_ [i];
	}

	T& operator [] (size_t i)
	{
		assert (i < (size_t)(end_ - begin_));
		return begin_ [i];
	}

private:
	T* begin_;
	T* end_;
};

}
}

#endif
