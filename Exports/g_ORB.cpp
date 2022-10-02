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
#include <CORBA/Server.h>
#include <CORBA/ORB_s.h>
#include <Binder.h>
#include <ORB/Services.h>
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
#include <ORB/PolicyFactory.h>

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

/// Implementation of the CORBA::ORB interface.
class ORB :
	public CORBA::servant_traits <CORBA::ORB>::ServantStatic <ORB>,
	public PolicyFactory
{
	typedef Nirvana::Core::SetUnorderedUnstable <IDL::String> NameSet;

public:
	static Type <ORBid>::ABI_ret _s_id (Bridge <CORBA::ORB>*, Interface*)
	{
		return Type <ORBid>::ret ();
	}

	static IDL::String object_to_string (Object::_ptr_type obj)
	{
		throw NO_IMPLEMENT ();
	}

	static Object::_ref_type string_to_object (const IDL::String& str)
	{
		throw NO_IMPLEMENT ();
	}

	void create_list (Long count, NVList::_ref_type& new_list)
	{
		throw NO_IMPLEMENT ();
	}

	void create_operation_list (OperationDef::_ptr_type oper, NVList::_ref_type& new_list)
	{
		throw NO_IMPLEMENT ();
	}

	void get_default_context (Context::_ref_type& ctx)
	{
		throw NO_IMPLEMENT ();
	}

	void send_multiple_requests_oneway (const RequestSeq& req)
	{
		throw NO_IMPLEMENT ();
	}

	void send_multiple_requests_deferred (const RequestSeq& req)
	{
		throw NO_IMPLEMENT ();
	}

	bool poll_next_response ()
	{
		throw NO_IMPLEMENT ();
	}

	void get_next_response (Request::_ref_type& req)
	{
		throw NO_IMPLEMENT ();
	}

	bool get_service_information (ServiceType service_type, ServiceInformation& service_information)
	{
		throw NO_IMPLEMENT ();
	}

	static ObjectIdList list_initial_services ()
	{
		return Services::list_initial_services ();
	}

	static Object::_ref_type resolve_initial_references (const ObjectId& identifier)
	{
		return Services::bind (identifier);
	}

	static TypeCode::_ref_type create_struct_tc (const RepositoryId& id,
		const Identifier& name, const StructMemberSeq& members)
	{
		if (members.empty ())
			throw BAD_PARAM ();
		return create_struct_tc (TCKind::tk_struct, id, name, members);
	}

	static TypeCode::_ref_type create_union_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type discriminator_type, const UnionMemberSeq& members)
	{
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
				default_index = it - members.begin ();

			pm->label = it->label ();
			pm->name = it->name ();
			pm->type = it->type ();
		}
		return make_pseudo <TC_Union> (id, name, TC_Ref (discriminator_type, true), default_index,
			std::move (smembers));
	}

	static TypeCode::_ref_type create_enum_tc (const RepositoryId& id,
		const Identifier& name, const EnumMemberSeq& members)
	{
		check_id (id);
		check_name (name);
		if (members.empty ())
			throw BAD_PARAM ();

		NameSet names;
		TC_Enum::Members smembers;
		smembers.construct (members.size ());
		TC_Base::String* pm = smembers.begin ();
		for (const Identifier& m : members) {
			check_member_name (names, m);
			*pm = m;
			++pm;
		}
		return make_pseudo <TC_Enum> (id, name, std::move (smembers));
	}

	static TypeCode::_ref_type create_alias_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type original_type)
	{
		check_id (id);
		check_name (name);
		check_type (original_type);

		return make_pseudo <TC_Alias> (id, name, TC_Ref (original_type, true));
	}

	static TypeCode::_ref_type create_exception_tc (const RepositoryId& id,
		const Identifier& name, const StructMemberSeq& members)
	{
		return create_struct_tc (TCKind::tk_except, id, name, members);
	}

	static TypeCode::_ref_type create_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return create_interface_tc (TCKind::tk_objref, id, name);
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

	static TypeCode::_ref_type create_sequence_tc (ULong bound,
		TypeCode::_ptr_type element_type)
	{
		check_type (element_type);
		return make_pseudo <TC_Sequence> (TC_Ref (element_type, true), bound);
	}

	static TypeCode::_ref_type create_array_tc (ULong length,
		TypeCode::_ptr_type element_type)
	{
		check_type (element_type);
		return make_pseudo <TC_Array> (TC_Ref (element_type, true), length);
	}

	static TypeCode::_ref_type create_value_tc (const RepositoryId& id,
		const Identifier& name, ValueModifier type_modifier,
		TypeCode::_ptr_type concrete_base, const ValueMemberSeq& members)
	{
		check_id (id);
		check_name (name);
		NameSet names;
		TC_Value::Members smembers;
		if (members.size ()) {
			smembers.construct (members.size ());
			TC_Value::Member* pm = smembers.begin ();
			for (const ValueMember& m : members) {
				check_member_name (names, m.name ());
				check_type (m.type ());
				pm->name = m.name ();
				pm->type = m.type ();
				pm->visibility = m.access ();
				++pm;
			}
		}
		return make_pseudo <TC_Value> (id, name, type_modifier,
			TC_Ref (concrete_base, true), std::move (smembers));
	}

	static TypeCode::_ref_type create_value_box_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type boxed_type)
	{
		check_id (id);
		check_name (name);
		check_type (boxed_type);

		return make_pseudo <TC_ValueBox> (id, name, TC_Ref (boxed_type, true));
	}

	static TypeCode::_ref_type create_native_tc (const RepositoryId& id,
		const Identifier& name)
	{
		check_id (id);
		check_name (name);

		return make_pseudo <TC_Native> (id, name);
	}

	static TypeCode::_ref_type create_recursive_tc (const RepositoryId& id)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_abstract_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		check_id (id);
		check_name (name);

		return make_pseudo <TC_Abstract> (id, name);
	}

	static TypeCode::_ref_type create_local_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return create_interface_tc (TCKind::tk_local_interface, id, name);
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

	static TypeCode::_ref_type create_event_tc (const RepositoryId& id,
		const Identifier& name, ValueModifier type_modifier,
		TypeCode::_ptr_type concrete_base, const ValueMemberSeq& members)
	{
		throw NO_IMPLEMENT ();
	}

	// Thread related operations

	bool work_pending ()
	{
		return false;
	}

	void perform_work ()
	{}

	void run ()
	{}

	void shutdown (bool wait_for_completion)
	{
		throw NO_IMPLEMENT ();
	}

	void destroy ()
	{}

	// Value factory operations

	static ValueFactoryBase::_ref_type register_value_factory (const RepositoryId&, ValueFactoryBase::_ptr_type)
	{
		throw NO_IMPLEMENT ();
	}

	static void unregister_value_factory (const RepositoryId&)
	{
		throw NO_IMPLEMENT ();
	}

	static ValueFactoryBase::_ref_type lookup_value_factory (const RepositoryId& id)
	{
		try {
			return Binder::bind_interface <ValueFactoryBase> (id);
		} catch (...) {
			throw BAD_PARAM (MAKE_OMG_MINOR (1));
		}
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

	static TypeCode::_ref_type create_struct_tc (TCKind kind, const RepositoryId& id,
		const Identifier& name, const StructMemberSeq& members);

	static TypeCode::_ref_type create_interface_tc (TCKind kind, const RepositoryId& id,
		const Identifier& name);
};

void ORB::check_name (const IDL::String& name, ULong minor)
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

void ORB::check_member_name (NameSet& names, const IDL::String& name)
{
	// Operations that take members shall check that the member names are valid IDL names
	// and that they are unique within the member list, and if the name is found to be incorrect,
	// they shall raise a BAD_PARAM with standard minor code 17.
	check_name (name, MAKE_OMG_MINOR (17));
	if (!names.insert (name).second)
		throw BAD_PARAM (MAKE_OMG_MINOR (17));
}

void ORB::check_id (const IDL::String& id)
{
	// Operations that take a RepositoryId argument shall check that the argument passed in is
	// a string of the form <format> : <string>and if not, then raise a BAD_PARAM exception with
	// standard minor code 16.
	size_t col = id.find (':');
	if (col == IDL::String::npos || col == 0 || col == id.size () - 1)
		throw BAD_PARAM (MAKE_OMG_MINOR (16));
}

void ORB::check_type (TypeCode::_ptr_type tc)
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

TypeCode::_ref_type ORB::create_struct_tc (TCKind kind, const RepositoryId& id,
	const Identifier& name, const StructMemberSeq& members)
{
	check_id (id);
	check_name (name);
	NameSet names;
	TC_Struct::Members smembers;
	if (members.size ()) {
		smembers.construct (members.size ());
		TC_Struct::Member* pm = smembers.begin ();
		for (const StructMember& m : members) {
			check_member_name (names, m.name ());
			check_type (m.type ());
			pm->name = m.name ();
			pm->type = m.type ();
			++pm;
		}
	}
	return make_pseudo <TC_Struct> (kind, id, name, std::move (smembers));
}

TypeCode::_ref_type ORB::create_interface_tc (TCKind kind, const RepositoryId& id,
	const Identifier& name)
{
	check_id (id);
	check_name (name);
	return make_pseudo <TC_Interface> (kind, id, name);
}

}

NIRVANA_SELECTANY
const ImportInterfaceT <ORB> g_ORB = { OLF_IMPORT_INTERFACE,
	"CORBA/g_ORB", CORBA::Internal::RepIdOf <ORB>::id,
	NIRVANA_STATIC_BRIDGE (ORB, Core::ORB) };

}

NIRVANA_EXPORT (_exp_CORBA_g_ORB, "CORBA/g_ORB", CORBA::ORB, CORBA::Core::ORB)
