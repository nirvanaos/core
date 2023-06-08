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

namespace CORBA {
namespace Core {

class StreamOut;

template <PolicyType type> class PolicyImpl;

class PolicyMap : public Nirvana::Core::MapUnorderedUnstable <PolicyType, OctetSeq,
	std::hash <PolicyType>, std::equal_to <PolicyType>,
	Nirvana::Core::UserAllocator>,
	public Nirvana::Core::UserObject
{
public:
	PolicyMap ();
	PolicyMap (const OctetSeq& src);

	// Does not write the policy size, only policies will be written
	void write (StreamOut& stm) const;

	bool insert (Policy::_ptr_type pol);

	Policy::_ref_type get_policy (PolicyType type) const;

	template <PolicyType type, typename ValueType>
	bool get_value (ValueType& val) const;

private:
	const OctetSeq* get_data (PolicyType type) const;
};

template <PolicyType type, typename ValueType>
bool PolicyMap::get_value (ValueType& val) const
{
	const OctetSeq* p = get_data (type);
	if (p) {
		Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (*p));
		val = PolicyImpl <type>::read_value (stm);
		return true;
	}
	return false;
}

typedef Nirvana::Core::ImplDynamicSync <PolicyMap> PolicyMapShared;

}
}

#endif
