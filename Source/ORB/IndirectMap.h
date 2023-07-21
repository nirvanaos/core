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
#ifndef NIRVANA_ORB_CORE_INDIRECTMAP_H_
#define NIRVANA_ORB_CORE_INDIRECTMAP_H_
#pragma once

#include <CORBA/CORBA.h>
#include "../MapUnorderedUnstable.h"
#include "../HeapAllocator.h"

namespace CORBA {
namespace Core {

static const ULong INDIRECTION_TAG = 0xFFFFFFFF;

typedef Nirvana::Core::MapUnorderedUnstable <uintptr_t, uintptr_t, std::hash <uintptr_t>,
	std::equal_to <uintptr_t>, Nirvana::Core::HeapAllocator>
	IndirectMapCont;

class IndirectMapMarshal : public IndirectMapCont
{
	typedef IndirectMapCont Base;

public:
	IndirectMapMarshal ();

	auto emplace (Internal::Interface::_ptr_type itf, size_t pos)
	{
		return emplace ((uintptr_t)&itf, (uintptr_t)pos);
	}

	auto emplace (Internal::Interface::_ptr_type itf, const Octet* pos)
	{
		return emplace ((uintptr_t)&itf, (uintptr_t)pos);
	}

	auto emplace (size_t pos, Internal::Interface::_ptr_type itf)
	{
		return emplace ((uintptr_t)pos, (uintptr_t)&itf);
	}

	auto emplace (const Octet* pos, Internal::Interface::_ptr_type itf)
	{
		return emplace ((uintptr_t)pos, (uintptr_t)&itf);
	}

	Internal::Interface* find (uintptr_t pos) const noexcept;

	Internal::Interface* find (const Octet* pos) const noexcept
	{
		return find ((uintptr_t)pos);
	}

private:
	std::pair <iterator, bool> emplace (uintptr_t key, uintptr_t val);
};

typedef IndirectMapMarshal IndirectMapUnmarshal;

template <class MarshalMap, class UnmarshalMap>
class IndirectMap
{
	enum class Discr
	{
		NONE,
		MARSHAL,
		UNMARSHAL
	};

public:
	IndirectMap () :
		d_ (Discr::NONE)
	{}

	~IndirectMap ()
	{
		clear ();
	}

	void clear () noexcept;

	MarshalMap& marshal_map ();
	UnmarshalMap& unmarshal_map ();

private:
	Discr d_;
	union U {
		U () {}
		~U () {}

		MarshalMap marshal_map;
		UnmarshalMap unmarshal_map;
	} u_;
};

template <class MarshalMap, class UnmarshalMap>
void IndirectMap <MarshalMap, UnmarshalMap>::clear () noexcept
{
	switch (d_) {
	case Discr::MARSHAL:
		u_.marshal_map.~MarshalMap ();
		break;

	case Discr::UNMARSHAL:
		u_.unmarshal_map.~UnmarshalMap ();
		break;
	}
	d_ = Discr::NONE;
}

template <class MarshalMap, class UnmarshalMap>
MarshalMap& IndirectMap <MarshalMap, UnmarshalMap>::marshal_map ()
{
	assert (d_ != Discr::UNMARSHAL);
	if (d_ == Discr::NONE) {
		d_ = Discr::MARSHAL;
		new (&u_.marshal_map) MarshalMap ();
	}
	return u_.marshal_map;
}

template <class MarshalMap, class UnmarshalMap>
UnmarshalMap& IndirectMap <MarshalMap, UnmarshalMap>::unmarshal_map ()
{
	assert (d_ != Discr::MARSHAL);
	if (d_ == Discr::NONE) {
		d_ = Discr::UNMARSHAL;
		new (&u_.unmarshal_map) UnmarshalMap ();
	}
	return u_.unmarshal_map;
}

typedef IndirectMap <IndirectMapMarshal, IndirectMapUnmarshal> IndirectMapItf;

}
}

#endif
