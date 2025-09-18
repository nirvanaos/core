/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#ifndef NIRVANA_CORE_BINARYMAP_H_
#define NIRVANA_CORE_BINARYMAP_H_
#pragma once

#include "Binary.h"
#include "SkipList.h"
#include "BinderMemory.h"

namespace Nirvana {
namespace Core {

/// Address of a loaded Binary
class BinaryAddress
{
public:
	BinaryAddress (Binary& binary) noexcept :
		begin_ ((const uint8_t*)binary.base_address ()),
		end_ ((const uint8_t*)binary.base_address () + binary.size ()),
		binary_ (&binary)
	{}

	BinaryAddress (const void* begin) noexcept :
		begin_ ((const uint8_t*)begin)
	{}

	bool operator < (const BinaryAddress& rhs) const noexcept
	{
		return begin_ > rhs.begin_; // Descending order
	}

	Binary* binary () const noexcept
	{
		return binary_;
	}

	const uint8_t* begin () const noexcept
	{
		return begin_;
	}

	const uint8_t* end () const noexcept
	{
		return end_;
	}

private:
	const uint8_t* begin_, * end_;
	Binary* binary_;
};

/// Map addresses to binaries.
/// Used to translate signals to exceptions.
class BinaryMap : public SkipList <BinaryAddress>
{
	typedef SkipList <BinaryAddress> Base;

public:
	BinaryMap () :
		Base (BinderMemory::heap ())
	{}

	void add (Binary& binary)
	{
		auto ins = Base::insert (std::ref (binary));
		NIRVANA_ASSERT (ins.second);
		release_node (ins.first);
	}

	void remove (Binary& binary) noexcept
	{
		Base::erase (binary.base_address ());
	}

	Binary* find (const void* addr) noexcept
	{
		Binary* binary = nullptr;
		NodeVal* pf = Base::lower_bound (addr);
		if (pf) {
			const BinaryAddress& v = pf->value ();
			if (addr < v.end ())
				binary = v.binary ();
			release_node (pf);
		}
		return binary;
	}
};

}
}

#endif
