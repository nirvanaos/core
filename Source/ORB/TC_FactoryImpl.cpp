/*
* Nirvana IDL support library.
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
#include "TC_FactoryImpl.h"
#include "Services.h"
#include "ProxyLocal.h"

namespace CORBA {
namespace Core {

std::atomic_flag TC_FactoryImpl::GC_scheduled_ = ATOMIC_FLAG_INIT;

inline void TC_FactoryImpl::schedule_GC () NIRVANA_NOEXCEPT
{
	if (!GC_scheduled_.test_and_set ()) {
		try {
			TC_Factory::_narrow (Services::bind (Services::TC_Factory))->collect_garbage ();
		} catch (...) {
		}
	}
}

void TC_ComplexBase::collect_garbage () NIRVANA_NOEXCEPT
{
	TC_FactoryImpl::schedule_GC ();
}

void TC_FactoryImpl::check_name (const IDL::String& name, ULong minor)
{
	if (!name.empty ()) {
		Char c = name.front ();
		if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
			const Char* p = name.data () + 1, * end = p + name.size () - 1;
			for (; p != end; ++p) {
				c = *p;
				if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || '_' == c))
					break;
			}
			if (p == end)
				return;
		}
	}
	throw BAD_PARAM (minor);
}

void TC_FactoryImpl::check_member_name (NameSet& names, const IDL::String& name)
{
	// Operations that take members shall check that the member names are valid IDL names
	// and that they are unique within the member list, and if the name is found to be incorrect,
	// they shall raise a BAD_PARAM with standard minor code 17.
	check_name (name, MAKE_OMG_MINOR (17));
	if (!names.insert (name).second)
		throw BAD_PARAM (MAKE_OMG_MINOR (17));
}

void TC_FactoryImpl::check_id (const IDL::String& id)
{
	// Operations that take a RepositoryId argument shall check that the argument passed in is
	// a string of the form <format> : <string>and if not, then raise a BAD_PARAM exception with
	// standard minor code 16.
	size_t col = id.find (':');
	if (col == IDL::String::npos || col == 0 || col == id.size () - 1)
		throw BAD_PARAM (MAKE_OMG_MINOR (16));
}

void TC_FactoryImpl::check_type (TypeCode::_ptr_type tc)
{
	// Operations that take content or member types as arguments shall check that they are legitimate
	// (i.e., that they don’t have kinds tk_null, tk_void, or tk_exception). If not, they shall raise
	// the BAD_TYPECODE exception with standard minor code 2.
	if (tc)
		switch (tc->kind ()) {
			case TCKind::tk_null:
			case TCKind::tk_void:
			case TCKind::tk_except:
				break;
			default:
				return;
		}
	throw BAD_TYPECODE (MAKE_OMG_MINOR (2));
}

TypeCode::_ref_type TC_FactoryImpl::create_struct_tc (TCKind kind, RepositoryId&& id,
	Identifier&& name, StructMemberSeq& members)
{
	check_name (name);
	NameSet names;
	TC_Struct::Members smembers;
	if (members.size ()) {
		smembers.construct (members.size ());
		TC_Struct::Member* pm = smembers.begin ();
		for (StructMember& m : members) {
			check_member_name (names, m.name ());
			check_type (m.type ());
			pm->name = std::move (m.name ());
			pm->type = TC_Ref (m.type (), complex_base (m.type ()));
			++pm;
		}
	}
	servant_reference <TC_Struct> ref =
		make_reference <TC_Struct> (kind, std::move (id), std::move (name), std::move (smembers));
	TypeCode::_ptr_type tc = ref->_get_ptr ();
	complex_objects_.emplace (&tc, ref);
	return tc;
}

TypeCode::_ref_type TC_FactoryImpl::create_interface_tc (TCKind kind, const RepositoryId& id,
	const Identifier& name)
{
	check_id (id);
	check_name (name);
	return make_pseudo <TC_Interface> (kind, id, name);
}

TC_ComplexBase* TC_FactoryImpl::complex_base (TypeCode::_ptr_type p) const NIRVANA_NOEXCEPT
{
	auto it = complex_objects_.find (&p);
	if (it == complex_objects_.end ())
		return nullptr;
	else
		return it->second;
}

}
}
