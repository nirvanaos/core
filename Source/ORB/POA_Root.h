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
#include "HashOctetSeq.h"
#include <CORBA/Servant_var.h>
#include <Nirvana/CoreDomains.h>
#include <random>

namespace IOP {
typedef CORBA::OctetSeq ObjectKey;
}

namespace PortableServer {
namespace Core {

class POA_Root :
	public POA_ImplicitUnique,
	public virtual POA_Base
{
	typedef std::conditional_t <(sizeof (size_t) > 4), std::mt19937_64, std::mt19937> RandomGen;

public:
	POA_Root (CORBA::servant_reference <POAManager>&& manager,
		CORBA::servant_reference <POAManagerFactory>&& manager_factory) :
		POA_Base (nullptr, nullptr, std::move (manager), CORBA::servant_reference <CORBA::Core::DomainManager> ()),
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

	virtual void check_object_id (const ObjectId& oid) override;

	virtual IDL::String the_name () const override
	{
		return "RootPOA";
	}

	static void invoke (RequestRef request, bool async) NIRVANA_NOEXCEPT;
	static void invoke_async (RequestRef request, Nirvana::DeadlineTime deadline);

	static POA_Ref find_child (const AdapterPath& path, bool activate_it);

	static CORBA::Object::_ref_type unmarshal (const IDL::String& iid, IOP::ObjectKey&& iop_key)
	{
		CORBA::Object::_ref_type root = get_root (); // Hold root POA reference

		SYNC_BEGIN (CORBA::Core::local2proxy (root)->sync_context (), nullptr)
		CORBA::Core::ReferenceLocalRef ref = root_->find_reference (iop_key);
		if (ref)
			ref->check_primary_interface (iid);
		else {
			ObjectKey core_key (iop_key);
			ref = find_child (core_key.adapter_path (), true)->create_reference (std::move (core_key), iid);
		}
		return CORBA::Object::_ref_type (ref->get_proxy ());
		SYNC_END ();
	}

	static void get_DGC_objects (const IDL::Sequence <IOP::ObjectKey>& keys, CORBA::Core::ReferenceLocalRef* refs)
	{
		CORBA::Object::_ref_type root = get_root (); // Hold root POA reference
		SYNC_BEGIN (CORBA::Core::local2proxy (root)->sync_context (), nullptr)
		for (const auto& iop_key : keys) {
			CORBA::Core::ReferenceLocalRef ref = root_->find_reference (iop_key);
			if (ref && (ref->flags () & CORBA::Core::Reference::GARBAGE_COLLECTION))
				*refs = std::move (ref);
			++refs;
		}
		SYNC_END ();
	}

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
			throw CORBA::BAD_PARAM (MAKE_OMG_MINOR (14));
	}

	typedef std::unique_ptr <CORBA::Core::ReferenceLocal> RefPtr;
	typedef Nirvana::Core::WaitableRef <RefPtr> RefVal;
	typedef Nirvana::Core::MapUnorderedStable <IOP::ObjectKey, RefVal, std::hash <IOP::ObjectKey>,
		std::equal_to <IOP::ObjectKey>, Nirvana::Core::UserAllocator> References;

	CORBA::Core::ReferenceLocalRef emplace_reference (ObjectKey&& core_key,
		bool unique, const IDL::String& primary_iid, unsigned flags,
		CORBA::Core::DomainManager* domain_manager);
	CORBA::Core::ReferenceLocalRef emplace_reference (ObjectKey&& core_key,
		bool unique, CORBA::Core::ServantProxyObject& proxy, unsigned flags,
		CORBA::Core::DomainManager* domain_manager);

	void remove_reference (const IOP::ObjectKey& key) NIRVANA_NOEXCEPT
	{
		references_.erase (key);
	}

	void remove_reference (References::iterator it) NIRVANA_NOEXCEPT
	{
		references_.erase (it);
	}

	CORBA::Core::ReferenceLocalRef find_reference (const ObjectKey& key) NIRVANA_NOEXCEPT;

	static void shutdown ()
	{
		if (root_) // POA was used in some way
			root_->_this ()->destroy (true, true);
	}

private:
	class InvokeAsync;

	static void invoke_sync (const RequestRef& request);

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
void POA_Base::implicit_activate (POA::_ptr_type adapter, CORBA::Core::ServantProxyObject& proxy)
{
	const CORBA::Core::ServantProxyLocal* adapter_proxy = CORBA::Core::local2proxy (adapter);
	POA_Base* adapter_impl = get_implementation (adapter_proxy);
	if (!adapter_impl)
		throw CORBA::OBJ_ADAPTER ();
	SYNC_BEGIN (adapter_proxy->sync_context (), nullptr);
	adapter_impl->activate_object (std::ref (proxy), CORBA::Core::Reference::GARBAGE_COLLECTION);
	SYNC_END ();
}

inline
POA::_ref_type POA_Base::create_POA (const IDL::String& adapter_name,
	PortableServer::POAManager::_ptr_type a_POAManager, const CORBA::PolicyList& policies)
{
	CORBA::servant_reference <CORBA::Core::DomainManager> domain_manager;
	POA_Policies pols;
	{
		CORBA::Core::PolicyMap object_policies;
		pols.set_values (policies, object_policies);
		if (!object_policies.empty ())
			domain_manager = CORBA::make_reference <CORBA::Core::DomainManager> (std::move (object_policies));
	}

	auto ins = children_.emplace (adapter_name, POA_Ref ());
	if (!ins.second)
		throw AdapterAlreadyExists ();

	try {

		CORBA::servant_reference <POAManager> manager = POAManager::get_implementation (
			CORBA::Core::local2proxy (a_POAManager));
		if (!manager)
			manager = root_->manager_factory ().create_auto (adapter_name);

		POA_Ref ref;
		const POA_FactoryEntry* pf = std::lower_bound (factories_, factories_ + FACTORY_COUNT, pols);
		if (pf != factories_ + FACTORY_COUNT && !(pols < *pf))
			ref = (pf->factory) (this, &ins.first->first, std::move (manager), std::move (domain_manager));
		
		if (!ref)
			throw InvalidPolicy (); // Do not return the index, it's too complex to calculate it.
		else
			ins.first->second = std::move (ref);

	} catch (...) {
		children_.erase (ins.first);
		throw;
	}

	return ins.first->second->_this ();
}

}
}

#endif
