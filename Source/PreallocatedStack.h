/// \file Stack.h
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
#ifndef NIRVANA_CORE_PREALLOCATEDSTACK_H_
#define NIRVANA_CORE_PREALLOCATEDSTACK_H_
#pragma once

#include "MemContextCore.h"
#include <type_traits>
#include <utility>

namespace Nirvana {
namespace Core {

/// Stack with preallocated initial storage.
/// 
/// \tparam El Element type.
/// \tparam PREALLOCATE Size of initially allocated storage (in elements).
/// \tparam MIN_BLOCK Minimal size of each additionally allocated block (in elements).
template <class El, size_t PREALLOCATE = 16, size_t MIN_BLOCK = 16>
class PreallocatedStack
{
	PreallocatedStack (const PreallocatedStack&) = delete;
	PreallocatedStack& operator = (const PreallocatedStack&) = delete;
public:
	/// Default constructor.
	PreallocatedStack () NIRVANA_NOEXCEPT :
		last_block_ (nullptr),
		top_ ((El*)&preallocated_)
	{}

	/// Constructor.
	///
	/// \param first First element to push.
	PreallocatedStack (const El& first) NIRVANA_NOEXCEPT :
		last_block_ (nullptr),
		top_ (((El*)&preallocated_) + 1)
	{
		::new (&preallocated_) El (first);
	}

	/// Constructor.
	/// 
	/// \param args Arguments to construct first element.
	template <class ... Args>
	PreallocatedStack (Args ... args) NIRVANA_NOEXCEPT :
		last_block_ (nullptr),
		top_ (((El*)&preallocated_) + 1)
	{
		::new (&preallocated_) El (std::forward <Args> (args)...);
	}

	~PreallocatedStack ()
	{
		clear ();
	}

	void clear () NIRVANA_NOEXCEPT
	{
		while (!empty ())
			pop ();
	}

	bool empty () const NIRVANA_NOEXCEPT
	{
		return top_ == (El*)&preallocated_;
	}

	El& top () NIRVANA_NOEXCEPT
	{
		assert (!empty ());
		return *(top_ - 1);
	}

	const El& top () const NIRVANA_NOEXCEPT
	{
		return const_cast <PreallocatedStack&> (*this).top ();
	}

	void push (const El& el)
	{
		allocate ();
		try {
			::new (top_) El (el);
		} catch (...) {
			release ();
			throw;
		}
		++top_;
	}

	void push (El&& el)
	{
		allocate ();
		::new (top_ ++) El (std::move (el));
	}

	template <class ... Args>
	void emplace (Args ... args)
	{
		allocate ();
		try {
			::new (top_) El (std::forward <Args> (args)...);
		} catch (...) {
			release ();
			throw;
		}
		++top_;
	}

	void pop () NIRVANA_NOEXCEPT
	{
		assert (!empty ());
		(--top_)->~El ();
		release ();
	}

private:
	struct Block;
	static const size_t BLOCK_ELEMENTS = (((size_t)1 << log2_ceil (MIN_BLOCK * sizeof (El) + sizeof (Block*))) - sizeof (Block*)) / sizeof (El);

	struct Block
	{
		typename std::aligned_storage <sizeof (El) * BLOCK_ELEMENTS, alignof (El)>::type storage;
		struct Block* prev;
	};

private:
	// Allocate space at the top of the stack
	void allocate ();

	// Release space at the top of the stack
	void release () NIRVANA_NOEXCEPT;

private:
	typename std::aligned_storage <sizeof (El)* PREALLOCATE, alignof (El)>::type preallocated_;
	Block* last_block_;
	El* top_;
};

template <class El, size_t PREALLOCATE, size_t MIN_BLOCK>
void PreallocatedStack <El, PREALLOCATE, MIN_BLOCK>::allocate ()
{
	El* end = last_block_ ? (El*)&(last_block_->storage) + BLOCK_ELEMENTS : (El*)&preallocated_ + PREALLOCATE;
	if (top_ == end) {
		// Allocate new block
		Block* block = (Block*)g_shared_mem_context->heap ().allocate (nullptr, sizeof (Block), 0);
		block->prev = last_block_;
		last_block_ = block;
		top_ = (El*)&(block->storage);
	}
}

template <class El, size_t PREALLOCATE, size_t MIN_BLOCK>
void PreallocatedStack <El, PREALLOCATE, MIN_BLOCK>::release () NIRVANA_NOEXCEPT
{
	if (top_ == ((El*)&(last_block_->storage))) {
		Block* block = last_block_;
		last_block_ = block->prev;
		top_ = last_block_ ? (El*)&(last_block_->storage) + BLOCK_ELEMENTS : (El*)&preallocated_ + PREALLOCATE;
		g_shared_mem_context->heap ().release (block, sizeof (Block));
	}
}

}
}

#endif
