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
#include "PolicyMap.h"
#include "PolicyFactory.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

PolicyMap::PolicyMap ()
{}

PolicyMap::PolicyMap (const OctetSeq& src)
{
	ImplStatic <StreamInEncap> stm (std::ref (src));
	for (size_t cnt = stm.read32 (); cnt; --cnt) {
		PolicyType type = stm.read32 ();
		OctetSeq data;
		stm.read_seq (data);
		map_.emplace (type, std::move (data));
	}
}

void PolicyMap::write (StreamOut& stm) const
{
	for (Map::const_iterator it = map_.begin (); it != map_.end (); ++it) {
		stm.write32 (it->first);
		stm.write_seq (it->second);
	}
}

bool PolicyMap::insert (Policy::_ptr_type pol)
{
	ImplStatic <StreamOutEncap> stm;
	PolicyFactory::write (pol, stm);
	return map_.emplace (pol->policy_type (), std::move (stm.data ())).second;
}

const OctetSeq* PolicyMap::get_data (PolicyType type) const NIRVANA_NOEXCEPT
{
	auto f = map_.find (type);
	if (f != map_.end ())
		return &f->second;
	else
		return nullptr;
}

Policy::_ref_type PolicyMap::get_policy (PolicyType type) const
{
	Policy::_ref_type policy;
	const OctetSeq* p = get_data (type);
	if (p)
		policy = PolicyFactory::create (type, *p);
	return policy;
}

}
}

