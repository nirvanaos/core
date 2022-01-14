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
