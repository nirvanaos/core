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
#ifndef NIRVANA_ORB_CORE_ORB_H_
#define NIRVANA_ORB_CORE_ORB_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/ORB_s.h>
#include "../Binder.h"
#include <CORBA/TC_Factory.h>
#include "Services.h"
#include "PolicyFactory.h"
#include "unmarshal_object.h"

namespace CORBA {
namespace Core {

/// Implementation of the CORBA::ORB interface.
class ORB :
	public CORBA::servant_traits <CORBA::ORB>::ServantStatic <ORB>
{
public:
	static const UShort IIOP_DEFAULT_PORT = 2809;

	static Internal::Type <ORBid>::ABI_ret _s_id (Internal::Bridge <CORBA::ORB>*, Internal::Interface*)
	{
		return Internal::Type <ORBid>::ret ();
	}

	static IDL::String object_to_string (Object::_ptr_type obj)
	{
		Nirvana::Core::ImplStatic <StreamOutEncap> stm;
		ProxyManager::cast (obj)->marshal (stm);
		IDL::String str;
		str.reserve (4 + stm.data ().size () * 2);
		str += "ior:";
		for (unsigned oct : stm.data ()) {
			str += to_hex (oct >> 4);
			str += to_hex (oct & 0xf);
		}
		return str;
	}

	static Object::_ref_type string_to_object (const IDL::String& str);

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

	static Object::_ref_type resolve_initial_references (const Internal::StringView <Char>& identifier)
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

	static Policy::_ref_type create_policy (PolicyType type, const Any& val)
	{
		return PolicyFactory::create (type, val);
	}

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
			return Nirvana::Core::Binder::bind_interface <ValueFactoryBase> (id);
		} catch (...) {
			throw BAD_PARAM (MAKE_OMG_MINOR (1));
		}
	}

	struct TC_Recurse
	{
		const TC_Recurse* prev;
		TypeCode::_ptr_type tc;
	};

	static TypeCode::_ref_type get_compact_typecode (TypeCode::_ptr_type tc, const TC_Recurse* parent = nullptr);

	struct TC_Pair
	{
		const TC_Pair* prev;
		TypeCode::_ptr_type left, right;
	};

	static bool tc_equal (TypeCode::_ptr_type left, TypeCode::_ptr_type right, const TC_Pair* parent = nullptr);
	static bool tc_equivalent (TypeCode::_ptr_type left, TypeCode::_ptr_type right, const TC_Pair* parent = nullptr);

private:

	static TC_Factory::_ptr_type tc_factory ()
	{
		return TC_Factory::_narrow (Services::bind (Services::TC_Factory));
	}

	static ULongLong get_union_label (TypeCode::_ptr_type tc, ULong i);

	static Char to_hex (unsigned d) noexcept
	{
		return d < 10 ? '0' + d : 'a' + d - 10;
	}

	static unsigned from_hex (int x);
	static bool schema_eq (const Char* schema, const Char* begin, const Char* end) noexcept;

	static OctetSeq unescape_object_key (const Char* begin, const Char* end);
};

inline Object::_ref_type ORB::string_to_object (const IDL::String& str)
{
	const Char* str_begin = str.data ();
	const Char* str_end = str_begin + str.size ();
	const Char* schema_end = std::find (str_begin, str_end, ':');
	if (*schema_end != ':')
		throw BAD_PARAM (MAKE_OMG_MINOR (7));

	Object::_ref_type obj;
	if (schema_eq ("ior", str_begin, schema_end)) {

		size_t digits = str_end - schema_end - 1;
		if (digits % 2)
			throw BAD_PARAM (MAKE_OMG_MINOR (9));
		OctetSeq octets;
		octets.reserve (digits / 2);
		for (const Char* pd = schema_end + 1; pd != str_end; pd += 2) {
			octets.push_back ((from_hex (pd [0]) << 4) | from_hex (pd [1]));
		}
		ReferenceRemoteRef unconfirmed_ref;
		Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (octets));
		obj = unmarshal_object (stm, unconfirmed_ref);

	} else if (schema_eq ("corbaloc", str_begin, schema_end)) {

		const Char* addr_begin = schema_end + 1;
		const Char* addr_end = std::find (addr_begin, str_end, '/');
		while (addr_begin < addr_end) {
			const Char* prot_end = std::find (addr_begin, addr_end, ',');
			if (addr_begin == prot_end)
				throw BAD_PARAM (MAKE_OMG_MINOR (8)); // Empty protocol
			const Char* schema_end = std::find (addr_begin, prot_end, ':');
			if (*schema_end != ':')
				throw BAD_PARAM (MAKE_OMG_MINOR (8));
			if (schema_end == addr_begin || schema_eq ("iiop", addr_begin, schema_end)) {
				// iiop protocol

				Nirvana::Core::ImplStatic <StreamOutEncap> octets (true);
				{
					const Char* ver_begin = schema_end + 1;
					const Char* ver_end = std::find (ver_begin, prot_end, '@');
					const Char* host_begin;

					IIOP::Version ver (1, 0);
					if (ver_end == prot_end)
						host_begin = ver_begin;
					else {
						size_t len;
						unsigned major = std::stoi (std::string (ver_begin, ver_end), &len);
						ver_begin += len;
						if (*ver_begin != '.')
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						++ver_begin;
						unsigned minor = std::stoi (std::string (ver_begin, ver_end), &len);
						if (ver_begin + len < ver_end)
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						if (major != 1 || minor > 255)
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						host_begin = ver_end + 1;
					}
					const Char* host_end = std::find (host_begin, prot_end, ':');

					UShort port = IIOP_DEFAULT_PORT;
					if (host_end != prot_end) {
						const Char* port_begin = host_end;
						size_t len;
						unsigned u = std::stoi (std::string (port_begin, prot_end), &len);
						if (port_begin + len != prot_end || u > std::numeric_limits <UShort>::max ())
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						port = (UShort)u;
					}

					Nirvana::Core::ImplStatic <StreamOutEncap> encap;
					encap.write_c (alignof (IIOP::Version), sizeof (IIOP::Version), &ver);
					size_t host_len = host_end - host_begin;
					if (host_len) {
						encap.write_size (host_len);
						encap.write_c (1, host_len, host_begin);
					} else {
						encap.write_string_c (LocalAddress::singleton ().host ());
					}
					encap.write_c (alignof (UShort), sizeof (UShort), &port);

					OctetSeq object_key;
					if (addr_end < str_end)
						object_key = unescape_object_key (addr_end + 1, str_end);
					encap.write_seq (object_key);

					if (ver.minor () > 0)
						encap.write_size (0); // empty IOP::TaggedComponentSeq

					IOP::TaggedProfileSeq addr {
						IOP::TaggedProfile (IOP::TAG_INTERNET_IOP, std::move (encap.data ()))
					};

					octets.write_tagged (addr);
				}

				Object::_ref_type ref;
				{
					ReferenceRemoteRef unconfirmed_ref;
					Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (octets.data ()), true);
					ref = unmarshal_object (IDL::String (), stm, unconfirmed_ref);
				}

				IDL::String iid = ref->_repository_id ();

				{
					ReferenceRemoteRef unconfirmed_ref;
					Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (octets.data ()), true);
					obj = unmarshal_object (iid, stm, unconfirmed_ref);
				}

			} else if (schema_eq ("rir", addr_begin, schema_end)) {
				// The rir protocol cannot be used with any other protocol in a URL.
				if (*prot_end == ',')
					throw BAD_PARAM (MAKE_OMG_MINOR (8));

				if (addr_end < str_end) {
					const Char* key_begin = addr_end + 1;
					obj = resolve_initial_references (Internal::StringView <Char> (key_begin, str_end - key_begin));
				} else
					obj = resolve_initial_references ("NameService");
			}
			if (obj)
				break;
			addr_begin = prot_end + 1;
		}
		if (!obj)
			throw BAD_PARAM (MAKE_OMG_MINOR (8));

	} else if (schema_eq ("corbaname", str_begin, schema_end)) {
		// TODO: Support
		throw BAD_PARAM (MAKE_OMG_MINOR (7));
	} else
		throw BAD_PARAM (MAKE_OMG_MINOR (7));

	return obj;
}

}
}

#endif
