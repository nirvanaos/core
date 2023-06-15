/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_ARRAY_H_
#define NIRVANA_CORE_ARRAY_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/Hash.h>
#include <functional>

namespace Nirvana {
namespace Core {

/// \brief Fixed size array.
template <class T, template <class> class Al>
class Array : private Al <T>
{
public:
	Array (const Al <T>& al = Al <T> ()) noexcept :
		Al <T> (al),
		begin_ (nullptr),
		end_ (nullptr)
	{}

	Array (Al <T>&& al) noexcept :
		Al <T> (std::move (al)),
		begin_ (nullptr),
		end_ (nullptr)
	{}

	Array (const Array& src, const Al <T>& al = Al <T> ()) :
		Al <T> (al),
		begin_ (nullptr),
		end_ (nullptr)
	{
		copy (src);
	}

	Array (const Array& src, Al <T>&& al) :
		Al <T> (std::move (al)),
		begin_ (nullptr),
		end_ (nullptr)
	{
		copy (src);
	}

	Array (Array&& src) noexcept :
		Al <T> (src), // don't move
		begin_ (src.begin_),
		end_ (src.end_)
	{
		src.begin_ = src.end_ = nullptr;
	}

	void copy (const Array& src)
	{
		assert (!begin_ && !end_);
		size_t size = src.size ();
		if (size) {
			begin_ = Al <T>::allocate (size);
			end_ = begin_ + size;
			T* pd = begin_;
			const T* ps = src.begin_;
			try {
				do {
					new (pd) T (*(ps++));
				} while (end_ != ++pd);
			} catch (...) {
				while (pd > begin_) {
					(--pd)->~T ();
				}
				throw;
			}
		}
	}

	Array& operator = (Array&& src) noexcept
	{
		Al <T>::operator = (src); // don't move
		assert (!begin_);
		begin_ = src.begin_;
		end_ = src.end_;
		src.begin_ = nullptr;
		src.end_ = nullptr;
		return *this;
	}

	/// \brief Allocate memory and initialize it with zeroes.
	void allocate (size_t size)
	{
		assert (!begin_ && !end_);
		if (size) {
			begin_ = Al <T>::allocate (size);
			end_ = begin_ + size;
			if (0 == sizeof (T) % sizeof (size_t))
				zero ((size_t*)begin_, (size_t*)end_);
			else if (0 == sizeof (T) % sizeof (int))
				zero ((int*)begin_, (int*)end_);
			else
				zero ((uint8_t*)begin_, (uint8_t*)end_);
		}
	}

	/// \brief Allocate and call default constructors.
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

	size_t size () const noexcept
	{
		return end_ - begin_;
	}

	const T* begin () const noexcept
	{
		return begin_;
	}

	T* begin () noexcept
	{
		return begin_;
	}

	const T* end () const noexcept
	{
		return end_;
	}

	T* end () noexcept
	{
		return end_;
	}

	const T* cbegin () const noexcept
	{
		return begin ();
	}

	const T* cend () const noexcept
	{
		return end ();
	}

	const T& front () const noexcept
	{
		assert (size ());
		return *begin_;
	}

	const T& back () const noexcept
	{
		assert (size ());
		return *(end_ - 1);
	}

	const T& operator [] (size_t i) const noexcept
	{
		assert (i < (size_t)(end_ - begin_));
		return begin_ [i];
	}

	T& operator [] (size_t i) noexcept
	{
		assert (i < (size_t)(end_ - begin_));
		return begin_ [i];
	}

	void swap (Array& rhs) noexcept
	{
		T* tmp = rhs.begin_;
		rhs.begin_ = begin_;
		begin_ = tmp;
		tmp = rhs.end_;
		rhs.end_ = end_;
		end_ = tmp;
	}

	const Al <T>& get_allocator () const noexcept
	{
		return static_cast <const Al <T>&> (*this);
	}

	bool operator == (const Array& rhs) const noexcept
	{
		return size () == rhs.size () && std::equal (begin_, end_, rhs.begin_);
	}

private:
	T* begin_;
	T* end_;
};

}
}

namespace std {

/// Byte array hash
template <template <class> class Al>
struct hash <Nirvana::Core::Array <uint8_t, Al> >
{
	size_t operator () (const Nirvana::Core::Array <uint8_t, Al>& ar) const noexcept
	{
		return Nirvana::Hash::hash_bytes (ar.begin (), ar.size ());
	}
};

}

#endif
