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
#ifndef NIRVANA_ORB_CORE_POA_ROOT_H_
#define NIRVANA_ORB_CORE_POA_ROOT_H_
#pragma once

#include "POA_Implicit.h"
#include "POAManagerFactory.h"
#include "../MapUnorderedStable.h"
#include <CORBA/Servant_var.h>
#include <random>

namespace IOP {
typedef CORBA::OctetSeq ObjectKey;
}

namespace PortableServer {
namespace Core {

class POA_Root :
	public virtual POA_Base,
	public POA_ImplicitUniqueTransient
{
	typedef POA_ImplicitUniqueTransient Base;
	typedef std::conditional_t <(sizeof (size_t) > 4), std::mt19937_64, std::mt19937> RandomGen;

public:
	POA_Root (CORBA::servant_reference <POAManager>&& manager,
		CORBA::servant_reference <POAManagerFactory>&& manager_factory) :
		POA_Base (nullptr, nullptr, std::move (manager)),
		manager_factory_ (std::move (manager_factory)),
		random_gen_ (RandomGen::result_type (Nirvana::Core::Chrono::UTC ().time () / TimeBase::SECOND))
	{
		root_ = this;
	}

	~POA_Root ()
	{
		assert (is_destroyed ());
		root_ = nullptr;
	}

	virtual IDL::String the_name () const override
	{
		return "RootPOA";
	}

	virtual CORBA::OctetSeq id () const override
	{
		return CORBA::OctetSeq ();
	}

	static void invoke (RequestRef request, bool async) NIRVANA_NOEXCEPT;
	static void invoke_async (RequestRef request, Nirvana::DeadlineTime deadline);

	static POA_Ref find_child (const AdapterPath& path) NIRVANA_NOEXCEPT;

	static CORBA::Object::_ref_type unmarshal (const IDL::String& iid, const IOP::ObjectKey& iop_key)
	{
		ObjectKey key;
		key.unmarshal (iop_key);
		CORBA::Object::_ref_type root = get_root (); // Hold root POA reference
		SYNC_BEGIN (CORBA::Core::local2proxy (root)->sync_context (), nullptr)
			CORBA::Core::ReferenceLocalRef ref = root_->find_reference (key);
			if (ref)
				ref->set_primary_interface (iid);
			else {

				POA_Ref adapter = find_child (key.adapter_path ());
				if (adapter)
					return adapter->create_reference (std::move (key), iid);

				ref = Servant_var <CORBA::Core::ReferenceLocal> (&const_cast <CORBA::Core::ReferenceLocal&> (
					*root_->create_reference (std::move (key), iid, false).first));
			}
			return CORBA::Object::_ref_type (ref->get_proxy ());
		SYNC_END ();
	}

	static PortableServer::POA::_ref_type create ();

	POAManagerFactory& manager_factory () NIRVANA_NOEXCEPT
	{
		return *manager_factory_;
	}

	ObjectId generate_persistent_id ()
	{
		struct
		{
			TimeBase::TimeT time;
			RandomGen::result_type random;
		} id = { Nirvana::Core::Chrono::UTC ().time (), dist_ (random_gen_)};

		const CORBA::Octet* p = (const CORBA::Octet*)&id;
		const CORBA::Octet* end = p + 8 + sizeof (RandomGen::result_type);
		return ObjectId (p, end);
	}

	static void check_persistent_id (const ObjectId& oid)
	{
		if (oid.size () != 8 + sizeof (RandomGen::result_type))
			throw CORBA::BAD_PARAM ();
	}

	void remove_reference (const CORBA::Core::ReferenceLocal& ref) NIRVANA_NOEXCEPT
	{
		references_.erase (ref);
	}

	typedef Nirvana::Core::SetUnorderedStable <CORBA::Core::ReferenceLocal, std::hash <ObjectKey>,
		std::equal_to <ObjectKey>, Nirvana::Core::UserAllocator <CORBA::Core::ReferenceLocal> > References;

	template <class ... Args>
	std::pair <References::iterator, bool> create_reference (Args ... args)
	{
		return references_.emplace (std::forward <Args> (args)...);
	}

	CORBA::Core::ReferenceLocalRef get_reference (const ObjectKey& key);
	CORBA::Core::ReferenceLocalRef find_reference (const ObjectKey& key) NIRVANA_NOEXCEPT;

private:
	class InvokeAsync;

	static void invoke_sync (POA_Ref adapter, const RequestRef& request);

private:
	References references_;
	CORBA::servant_reference <POAManagerFactory> manager_factory_;
	RandomGen random_gen_;
	std::uniform_int_distribution <RandomGen::result_type> dist_;
};

inline
PortableServer::POAManagerFactory::_ref_type POA_Base::the_POAManagerFactory () NIRVANA_NOEXCEPT
{
	return root_->manager_factory ()._this ();
}

inline
void POA_Base::implicit_activate (POA::_ptr_type adapter, CORBA::Core::ProxyObject& proxy)
{
	const CORBA::Core::ProxyLocal* adapter_proxy = CORBA::Core::local2proxy (adapter);
	POA_Base* adapter_impl = get_implementation (adapter_proxy);
	if (!adapter_impl)
		throw CORBA::OBJ_ADAPTER ();
	SYNC_BEGIN (adapter_proxy->sync_context (), nullptr);
	adapter_impl->activate_object (ObjectKey (*adapter_impl), std::ref (proxy), true);
	SYNC_END ();
}

inline
PortableServer::POA::_ref_type POA_Root::create ()
{
	auto manager_factory = CORBA::make_reference <POAManagerFactory> ();
	auto manager = manager_factory->create ("RootPOAManager", CORBA::PolicyList ());
	return CORBA::make_reference <POA_Root> (std::move (manager), std::move (manager_factory))->_this ();
}

}
}

#endif
