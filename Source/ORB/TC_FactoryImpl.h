/// \file
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
#ifndef NIRVANA_ORB_CORE_TC_FACTORYIMPL_H_
#define NIRVANA_ORB_CORE_TC_FACTORYIMPL_H_
#pragma once

#include <CORBA/Server.h>
#include "TC_Factory_s.h"
#include <ORB/TC_Fixed.h>
#include <ORB/TC_Interface.h>
#include <ORB/TC_Struct.h>
#include <ORB/TC_Union.h>
#include <ORB/TC_Enum.h>
#include <ORB/TC_String.h>
#include <ORB/TC_Sequence.h>
#include <ORB/TC_Array.h>
#include <ORB/TC_Alias.h>
#include <ORB/TC_Value.h>
#include <ORB/TC_ValueBox.h>
#include <ORB/TC_Abstract.h>
#include <ORB/TC_Native.h>
#include <ORB/TC_Recursive.h>
#include "IndirectMapUnmarshal.h"
#include "Services.h"
#include "ProxyLocal.h"
#include "../Synchronized.h"

namespace CORBA {
namespace Core {

class StreamIn;

class TC_FactoryImpl : public servant_traits <TC_Factory>::Servant <TC_FactoryImpl>
{
	typedef Nirvana::Core::SetUnorderedUnstable <IDL::String> NameSet;

public:
	TypeCode::_ref_type create_struct_tc (RepositoryId& id, Identifier& name,
		StructMemberSeq& members)
	{
		if (members.empty ())
			throw BAD_PARAM ();
		if (!id.empty ())
			check_id (id);
		return create_struct_tc (TCKind::tk_struct, std::move (id), std::move (name), members);
	}

	TypeCode::_ref_type create_union_tc (RepositoryId& id, Identifier& name,
		TypeCode::_ptr_type discriminator_type, UnionMemberSeq& members)
	{
		if (!id.empty ())
			check_id (id);
		check_name (name);
		if (members.empty ())
			throw BAD_PARAM ();

		// The create_union_tc operation shall also check that the supplied discriminator type is
		// legitimate, and if the check fails, raise BAD_PARAM with standard minor code 20.
		switch (discriminator_type->kind ()) {
			case TCKind::tk_short:
			case TCKind::tk_long:
			case TCKind::tk_ushort:
			case TCKind::tk_ulong:
			case TCKind::tk_boolean:
			case TCKind::tk_char:
			case TCKind::tk_enum:
			case TCKind::tk_longlong:
			case TCKind::tk_ulonglong:
				break;
			default:
				throw BAD_PARAM (MAKE_OMG_MINOR (20));
		}
		NameSet names;
		TC_Union::Members smembers;
		smembers.construct (members.size ());
		TC_Union::Member* pm = smembers.begin ();
		Nirvana::Core::SetUnorderedUnstable <ULongLong> labels;
		Long default_index = -1;
		for (auto it = members.begin (); it != members.end (); ++it, ++pm) {
			check_member_name (names, it->name ());
			check_type (it->type ());

			// If the TypeCode of a label is not equivalent to the TypeCode of the discriminator (other than
			// the octet TypeCode to indicate the default label), the operation shall raise BAD_PARAM with
			// standard minor code 19.
			TypeCode::_ref_type label_type = it->label ().type ();
			if (label_type->kind () != TCKind::tk_octet) {
				if (!label_type->equivalent (discriminator_type))
					throw BAD_PARAM (MAKE_OMG_MINOR (19));

				// If a duplicate label is found, raise BAD_PARAM with standard minor code 18.
				ULongLong val = 0;
				discriminator_type->n_copy (&val, it->label ().data ());
				if (!labels.insert (val).second)
					throw BAD_PARAM (MAKE_OMG_MINOR (18));

			} else if (default_index >= 0) {
				throw BAD_PARAM (MAKE_OMG_MINOR (19));
			} else
				default_index = Long (it - members.begin ());

			pm->label = it->label ();
			pm->name = std::move (it->name ());
			pm->type = TC_Ref (it->type (), complex_base (it->type ()));
		}
		servant_reference <TC_Union> ref = make_reference <TC_Union> (std::move (id), std::move (name),
			discriminator_type, default_index, std::move (smembers));
		TypeCode::_ptr_type tc = ref->_get_ptr ();
		complex_objects_.emplace (&tc, ref);
		return tc;
	}

	static TypeCode::_ref_type create_enum_tc (RepositoryId& id, Identifier& name,
		EnumMemberSeq& members)
	{
		if (!id.empty ())
			check_id (id);
		check_name (name);
		if (members.empty ())
			throw BAD_PARAM ();

		NameSet names;
		TC_Enum::Members smembers;
		smembers.construct (members.size ());
		IDL::String* pm = smembers.begin ();
		for (Identifier& m : members) {
			check_member_name (names, m);
			*pm = std::move (m);
			++pm;
		}
		return make_pseudo <TC_Enum> (std::move (id), std::move (name), std::move (smembers));
	}

	TypeCode::_ref_type create_alias_tc (RepositoryId& id, Identifier& name,
		TypeCode::_ptr_type original_type)
	{
		if (!id.empty ())
			check_id (id);
		check_name (name);
		check_type (original_type);
		servant_reference <TC_Alias> ref = make_reference <TC_Alias> (std::move (id), std::move (name), TC_Ref (original_type, complex_base (original_type)));
		TypeCode::_ptr_type tc = ref->_get_ptr ();
		complex_objects_.emplace (&tc, ref);
		return tc;
	}

	TypeCode::_ref_type create_exception_tc (RepositoryId& id, Identifier& name,
		StructMemberSeq& members)
	{
		check_id (id);
		return create_struct_tc (TCKind::tk_except, std::move (id), std::move (name), members);
	}

	static TypeCode::_ref_type create_interface_tc (RepositoryId& id, Identifier& name)
	{
		return create_interface_tc (TCKind::tk_objref, std::move (id), std::move (name));
	}

	static TypeCode::_ref_type create_string_tc (ULong bound)
	{
		if (0 == bound)
			return TypeCode::_ptr_type (_tc_string);
		else
			return make_pseudo <TC_String> (bound);
	}

	static TypeCode::_ref_type create_wstring_tc (ULong bound)
	{
		if (0 == bound)
			return TypeCode::_ptr_type (_tc_wstring);
		else
			return make_pseudo <TC_WString> (bound);
	}

	static TypeCode::_ref_type create_fixed_tc (UShort digits, Short scale)
	{
		if (digits > 31 || scale > digits || scale < 0)
			throw BAD_PARAM ();
		return make_pseudo <TC_Fixed> (digits, scale);
	}

	TypeCode::_ref_type create_sequence_tc (ULong bound, TypeCode::_ptr_type element_type)
	{
		check_type (element_type);
		servant_reference <TC_Sequence> ref = 
			make_reference <TC_Sequence> (TC_Ref (element_type, complex_base (element_type)), bound);
		TypeCode::_ptr_type tc = ref->_get_ptr ();
		complex_objects_.emplace (&tc, ref);
		return tc;
	}

	TypeCode::_ref_type create_array_tc (ULong length, TypeCode::_ptr_type element_type)
	{
		check_type (element_type);
		servant_reference <TC_Array> ref =
			make_reference <TC_Array> (TC_Ref (element_type, complex_base (element_type)), length);
		TypeCode::_ptr_type tc = ref->_get_ptr ();
		complex_objects_.emplace (&tc, ref);
		return tc;
	}

	TypeCode::_ref_type create_value_tc (RepositoryId& id, Identifier& name,
		ValueModifier type_modifier, TypeCode::_ptr_type concrete_base, ValueMemberSeq& members)
	{
		check_id (id);
		check_name (name);
		NameSet names;
		TC_Value::Members smembers;
		if (members.size ()) {
			smembers.construct (members.size ());
			TC_Value::Member* pm = smembers.begin ();
			for (ValueMember& m : members) {
				check_member_name (names, m.name ());
				check_type (m.type ());
				pm->name = std::move (m.name ());
				pm->type = TC_Ref (m.type (), complex_base (m.type ()));
				pm->visibility = m.access ();
				++pm;
			}
		}
		servant_reference <TC_Value> ref =
			make_reference <TC_Value> (std::move (id), std::move (name), type_modifier,
				TC_Ref (concrete_base, complex_base (concrete_base)), std::move (smembers));
		TypeCode::_ptr_type tc = ref->_get_ptr ();
		complex_objects_.emplace (&tc, ref);
		return tc;
	}

	TypeCode::_ref_type create_value_box_tc (RepositoryId& id, Identifier& name,
		TypeCode::_ptr_type boxed_type)
	{
		check_id (id);
		check_name (name);
		check_type (boxed_type);
		servant_reference <TC_ValueBox> ref =
			make_reference <TC_ValueBox> (std::move (id), std::move (name),
				TC_Ref (boxed_type, complex_base (boxed_type)));
		TypeCode::_ptr_type tc = ref->_get_ptr ();
		complex_objects_.emplace (&tc, ref);
		return tc;
	}

	static TypeCode::_ref_type create_native_tc (RepositoryId& id, Identifier& name)
	{
		check_id (id);
		check_name (name);

		return make_pseudo <TC_Native> (std::move (id), std::move (name));
	}

	TypeCode::_ref_type create_recursive_tc (RepositoryId& id)
	{
		if (!id.empty ())
			check_id (id);
		servant_reference <TC_Recursive> ref = make_reference <TC_Recursive> (std::move (id));
		TypeCode::_ptr_type tc = ref->_get_ptr ();
		complex_objects_.emplace (&tc, ref);
		return tc;
	}

	static TypeCode::_ref_type create_abstract_interface_tc (RepositoryId& id, Identifier& name)
	{
		check_id (id);
		check_name (name);

		return make_pseudo <TC_Abstract> (std::move (id), std::move (name));
	}

	static TypeCode::_ref_type create_local_interface_tc (RepositoryId& id, Identifier& name)
	{
		return create_interface_tc (TCKind::tk_local_interface, std::move (id), std::move (name));
	}

	static TypeCode::_ref_type create_component_tc (const RepositoryId& id,
		const Identifier& name)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_home_tc (const RepositoryId& id,
		const Identifier& name)
	{
		throw NO_IMPLEMENT ();
	}

	TypeCode::_ref_type create_event_tc (const RepositoryId& id,
		const Identifier& name, ValueModifier type_modifier,
		TypeCode::_ptr_type concrete_base, const ValueMemberSeq& members)
	{
		throw NO_IMPLEMENT ();
	}

	void collect_garbage () NIRVANA_NOEXCEPT
	{
		GC_scheduled_.clear ();
		for (auto it = complex_objects_.begin (); it != complex_objects_.end (); ++it) {
			if (it->second->has_extern_ref ())
				it->second->mark ();
		}
		for (auto it = complex_objects_.begin (); it != complex_objects_.end ();) {
			if (it->second->sweep ())
				it = complex_objects_.erase (it);
			else
				++it;
		}
	}

	static void schedule_GC () NIRVANA_NOEXCEPT;

	static TypeCode::_ref_type unmarshal_type_code (ULong kind, StreamIn& stream)
	{
		Object::_ref_type factory = Services::bind (Services::TC_Factory);
		const ProxyLocal* proxy = local2proxy (factory);
		TC_FactoryImpl* impl = static_cast <TC_FactoryImpl*> (
			static_cast <Bridge <CORBA::LocalObject>*> (&proxy->servant ()));
		SYNC_BEGIN (proxy->sync_context (), nullptr);
		IndirectMapUnmarshal indirect_map;
		return impl->unmarshal_type_code (kind, stream, indirect_map, 0);
		SYNC_END ();
	}

private:
	static void check_name (const IDL::String& name, ULong minor);

	static void check_name (const IDL::String& name)
	{
		// Typecode creation operations that take name as an argument shall check that the name is
		// a valid IDL name or is an empty string. If not, they shall raise the BAD_PARAM exception
		// with standard minor code 15.
		if (!name.empty ())
			check_name (name, MAKE_OMG_MINOR (15));
	}

	static void check_member_name (NameSet& names, const IDL::String& name);

	static void check_id (const IDL::String& id);

	static void check_type (TypeCode::_ptr_type tc);

	TypeCode::_ref_type create_struct_tc (TCKind kind, RepositoryId&& id,
		Identifier&& name, StructMemberSeq& members);

	static TypeCode::_ref_type create_interface_tc (TCKind kind, const RepositoryId& id,
		const Identifier& name);

	TC_ComplexBase* complex_base (TypeCode::_ptr_type p) const NIRVANA_NOEXCEPT;

	TypeCode::_ref_type unmarshal_type_code (StreamIn& stream, IndirectMapUnmarshal& indirect_map, size_t parent_offset);
	TypeCode::_ref_type unmarshal_type_code (ULong kind, StreamIn& stream, IndirectMapUnmarshal& indirect_map, size_t parent_offset);

private:
	Nirvana::Core::MapUnorderedUnstable <void*, TC_ComplexBase*,
		std::hash <void*>, std::equal_to <void*>,
		Nirvana::Core::UserAllocator <std::pair <void*, TC_ComplexBase*> > > complex_objects_;

	static std::atomic_flag GC_scheduled_;
};

}
}

#endif
