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
#include <ORB/TC_Factory.h>
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
		return tc_factory ()->create_struct_tc (id, name, members);
	}

	static TypeCode::_ref_type create_union_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type discriminator_type, const UnionMemberSeq& members)
	{
		return tc_factory ()->create_union_tc (id, name, discriminator_type, members);
	}

	static TypeCode::_ref_type create_enum_tc (const RepositoryId& id,
		const Identifier& name, const EnumMemberSeq& members)
	{
		return tc_factory ()->create_enum_tc (id, name, members);
	}

	static TypeCode::_ref_type create_alias_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type original_type)
	{
		return tc_factory ()->create_alias_tc (id, name, original_type);
	}

	static TypeCode::_ref_type create_exception_tc (const RepositoryId& id,
		const Identifier& name, const StructMemberSeq& members)
	{
		return tc_factory ()->create_exception_tc (id, name, members);
	}

	static TypeCode::_ref_type create_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return tc_factory ()->create_interface_tc (id, name);
	}

	static TypeCode::_ref_type create_string_tc (ULong bound)
	{
		return tc_factory ()->create_string_tc (bound);
	}

	static TypeCode::_ref_type create_wstring_tc (ULong bound)
	{
		return tc_factory ()->create_wstring_tc (bound);
	}

	static TypeCode::_ref_type create_fixed_tc (UShort digits, Short scale)
	{
		return tc_factory ()->create_fixed_tc (digits, scale);
	}

	static TypeCode::_ref_type create_sequence_tc (ULong bound,
		TypeCode::_ptr_type element_type)
	{
		return tc_factory ()->create_sequence_tc (bound, element_type);
	}

	static TypeCode::_ref_type create_array_tc (ULong length,
		TypeCode::_ptr_type element_type)
	{
		return tc_factory ()->create_array_tc (length, element_type);
	}

	static TypeCode::_ref_type create_value_tc (const RepositoryId& id,
		const Identifier& name, ValueModifier type_modifier,
		TypeCode::_ptr_type concrete_base, const ValueMemberSeq& members)
	{
		return tc_factory ()->create_value_tc (id, name, type_modifier, concrete_base, members);
	}

	static TypeCode::_ref_type create_value_box_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type boxed_type)
	{
		return tc_factory ()->create_value_box_tc (id, name, boxed_type);
	}

	static TypeCode::_ref_type create_native_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return tc_factory ()->create_native_tc (id, name);
	}

	static TypeCode::_ref_type create_recursive_tc (const RepositoryId& id)
	{
		return tc_factory ()->create_recursive_tc (id);
	}

	static TypeCode::_ref_type create_abstract_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return tc_factory ()->create_abstract_interface_tc (id, name);
	}

	static TypeCode::_ref_type create_local_interface_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return tc_factory ()->create_local_interface_tc (id, name);
	}

	static TypeCode::_ref_type create_component_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return tc_factory ()->create_component_tc (id, name);
	}

	static TypeCode::_ref_type create_home_tc (const RepositoryId& id,
		const Identifier& name)
	{
		return tc_factory ()->create_home_tc (id, name);
	}

	static TypeCode::_ref_type create_event_tc (const RepositoryId& id,
		const Identifier& name, ValueModifier type_modifier,
		TypeCode::_ptr_type concrete_base, const ValueMemberSeq& members)
	{
		return tc_factory ()->create_event_tc (id, name, type_modifier, concrete_base, members);
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
	static TC_Factory::_ptr_type tc_factory ()
	{
		return TC_Factory::_narrow (Services::bind (Services::TC_Factory));
	}
};

}

NIRVANA_SELECTANY
const ImportInterfaceT <ORB> g_ORB = { OLF_IMPORT_INTERFACE,
	"CORBA/g_ORB", CORBA::Internal::RepIdOf <ORB>::id,
	NIRVANA_STATIC_BRIDGE (ORB, Core::ORB) };

}

NIRVANA_EXPORT (_exp_CORBA_g_ORB, "CORBA/g_ORB", CORBA::ORB, CORBA::Core::ORB)
