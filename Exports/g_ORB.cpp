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
#include <ORB/TC_ObjRef.h>
#include <ORB/TC_Struct.h>
#include <ORB/TC_Union.h>
#include <ORB/TC_Enum.h>
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
		TC_Struct::Members smembers;
		smembers.construct (members.size ());
		TC_Struct::Member* pm = smembers.begin ();
		for (const StructMember& m : members) {
			pm->name = m.name ();
			pm->type = m.type ();
			++pm;
		}
		return make_pseudo <TC_Struct> (id, name, std::move (smembers));
	}

	static TypeCode::_ref_type create_union_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type discriminator_type, const UnionMemberSeq& members)
	{
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
		TC_Union::Members smembers;
		smembers.construct (members.size ());
		TC_Union::Member* pm = smembers.begin ();
		Nirvana::Core::SetUnorderedUnstable <ULongLong> labels;
		Long default_index = -1;
		for (auto it = members.begin (); it != members.end (); ++it, ++pm) {
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
		return make_pseudo <TC_Union> (id, name, std::move (discriminator_type), default_index,
			std::move (smembers));
	}

	static TypeCode::_ref_type create_enum_tc (const RepositoryId& id,
		const Identifier& name, const EnumMemberSeq& members)
	{
		TC_Enum::Members smembers;
		smembers.construct (members.size ());
		TC_Base::String* pm = smembers.begin ();
		for (const Identifier& m : members) {
			*pm = m;
		}
		return make_pseudo <TC_Enum> (id, name, std::move (smembers));
	}

	static TypeCode::_ref_type create_alias_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type original_type)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_exception_tc (const RepositoryId& id,
		const Identifier& name, const StructMemberSeq& members)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return make_pseudo <TC_ObjRef> (id, name);
	}

	static TypeCode::_ref_type create_string_tc (ULong bound)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_wstring_tc (ULong bound)
	{
		throw NO_IMPLEMENT ();
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
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_array_tc (ULong length,
		TypeCode::_ptr_type element_type)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_value_tc (const RepositoryId& id,
		const Identifier& name, ValueModifier type_modifier,
		TypeCode::_ptr_type concrete_base, const ValueMemberSeq& members)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_value_box_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type boxed_type)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_native_tc (const RepositoryId& id,
		const Identifier& name)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_recursive_tc (const RepositoryId& id)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_abstract_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_local_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		throw NO_IMPLEMENT ();
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
};

}

NIRVANA_SELECTANY
const ImportInterfaceT <ORB> g_ORB = { OLF_IMPORT_INTERFACE,
	"CORBA/g_ORB", CORBA::Internal::RepIdOf <ORB>::id,
	NIRVANA_STATIC_BRIDGE (ORB, Core::ORB) };

}

NIRVANA_EXPORT (_exp_CORBA_g_ORB, "CORBA/g_ORB", CORBA::ORB, CORBA::Core::ORB)
