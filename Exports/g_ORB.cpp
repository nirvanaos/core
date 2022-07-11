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
#include <ORB/IDL/ORB_s.h>
#include <Binder.h>

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

/// Implementation of the CORBA::ORB interface.
class ORB :
	public CORBA::servant_traits <CORBA::ORB>::ServantStatic <ORB>
{
public:
	static Object::_ref_type resolve_initial_references (const ObjectId& identifier)
	{
		return Binder::bind_service (identifier);
	}

	static TypeCode::_ref_type create_struct_tc (const RepositoryId& id,
		const Identifier& name, const StructMemberSeq& members)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_union_tc (const RepositoryId& id,
		const Identifier& name, TypeCode::_ptr_type discriminator_type, const UnionMemberSeq& members)
	{
		throw NO_IMPLEMENT ();
	}

	static TypeCode::_ref_type create_enum_tc (const RepositoryId& id,
		const Identifier& name, const EnumMemberSeq& members)
	{
		throw NO_IMPLEMENT ();
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
		throw NO_IMPLEMENT ();
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
		throw NO_IMPLEMENT ();
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
		return Binder::bind_interface <ValueFactoryBase> (id);
	}
};

}

__declspec (selectany)
const ImportInterfaceT <ORB> g_ORB = { OLF_IMPORT_INTERFACE,
	"CORBA/g_ORB", CORBA::Internal::RepIdOf <ORB>::id,
	NIRVANA_STATIC_BRIDGE (ORB, Core::ORB) };

}

NIRVANA_EXPORT (_exp_CORBA_g_ORB, "CORBA/g_ORB", CORBA::ORB, CORBA::Core::ORB)
