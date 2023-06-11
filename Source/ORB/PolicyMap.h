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
#ifndef NIRVANA_ORB_CORE_POLICYMAP_H_
#define NIRVANA_ORB_CORE_POLICYMAP_H_
#pragma once

#include <CORBA/CORBA.h>
#include "../MapUnorderedUnstable.h"
#include "../UserAllocator.h"
#include "../UserObject.h"
#include "../CoreInterface.h"
#include "StreamInEncap.h"
#include "StreamOutEncap.h"

namespace CORBA {
namespace Core {

template <PolicyType type> class PolicyImpl;

class PolicyMap : public Nirvana::Core::UserObject
{
public:
	PolicyMap ();
	PolicyMap (const PolicyMap& src) = default;
	PolicyMap (PolicyMap&& src) = default;
	PolicyMap (const OctetSeq& src);

	// Does not write the policy size, only policies will be written
	void write (StreamOut& stm) const;

	bool insert (Policy::_ptr_type pol);

	template <PolicyType type>
	bool insert (const typename PolicyImpl <type>::ValueType& val);

	Policy::_ref_type get_policy (PolicyType type) const;

	template <PolicyType type>
	bool get_value (typename PolicyImpl <type>::ValueType& val) const;

	bool erase (PolicyType type) noexcept
	{
		return map_.erase (type);
	}

	bool empty () const noexcept
	{
		return map_.empty ();
	}

	size_t size () const noexcept
	{
		return map_.size ();
	}

private:
	const OctetSeq* get_data (PolicyType type) const noexcept;

private:
	typedef Nirvana::Core::MapUnorderedUnstable <PolicyType, OctetSeq,
		std::hash <PolicyType>, std::equal_to <PolicyType>,
		Nirvana::Core::UserAllocator> Map;

	Map map_;
};

template <PolicyType type>
bool PolicyMap::insert (const typename PolicyImpl <type>::ValueType& val)
{
	Nirvana::Core::ImplStatic <StreamOutEncap> stm;
	PolicyImpl <type>::write_value (val, stm);
	return map_.emplace (type, std::move (stm.data ())).second;
}

template <PolicyType type>
bool PolicyMap::get_value (typename PolicyImpl <type>::ValueType& val) const
{
	const OctetSeq* p = get_data (type);
	if (p) {
		Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (*p));
		PolicyImpl <type>::read_value (stm, val);
		return true;
	}
	return false;
}

typedef Nirvana::Core::ImplDynamicSyncVirt <PolicyMap> PolicyMapShared;

}
}

#endif
