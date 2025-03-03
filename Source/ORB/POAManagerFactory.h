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
#ifndef NIRVANA_ORB_CORE_POAMANAGERFACTORY_H_
#define NIRVANA_ORB_CORE_POAMANAGERFACTORY_H_
#pragma once

#include "POAManager.h"

namespace PortableServer {
namespace Core {

class POAManagerFactory :
	public CORBA::servant_traits <PortableServer::POAManagerFactory>::Servant <POAManagerFactory>
{
public:
	POAManagerFactory ()
	{}

	~POAManagerFactory ()
	{}

	PortableServer::POAManager::_ref_type create_POAManager (const IDL::String& id, const CORBA::PolicyList& policies)
	{
		if (!policies.empty ())
			throw CORBA::PolicyError (CORBA::UNSUPPORTED_POLICY);
		CORBA::servant_reference <POAManager> manager = create (id, policies);
		if (!manager)
			throw ManagerAlreadyExists ();
		return manager->_this ();
	}

	CORBA::servant_reference <POAManager> create (const IDL::String& id, const CORBA::PolicyList& policies = CORBA::PolicyList ());

	CORBA::servant_reference <POAManager> create_auto (const IDL::String& adapter_name)
	{
		IDL::String name = adapter_name + "Manager";
		CORBA::servant_reference <POAManager> manager;
		unsigned suffix = 0;
		size_t len = name.size ();
		while (!(manager = create (name))) {
			name.resize (len);
			name += std::to_string (++suffix);
		}
		return manager;
	}

	POAManagerSeq list () const
	{
		POAManagerSeq ret;
		ret.reserve (managers_.size ());
		for (auto& entry : managers_) {
			ret.push_back (const_cast <POAManager&> (entry)._this ());
		}
		return ret;
	}

	PortableServer::POAManager::_ref_type find (const IDL::String& id) const noexcept
	{
		PortableServer::POAManager::_ref_type ret;
		auto f = managers_.find (static_cast <const POAManager&> (id));
		if (f != managers_.end ())
			ret = const_cast <POAManager&> (*f)._this ();
		return ret;
	}

	void deactivate_all ()
	{
		for (auto& entry : managers_) {
			const_cast <POAManager&> (entry).deactivate (false, false);
		}
	}

private:
	friend class POAManager;

	typedef Nirvana::Core::SetUnorderedStable <POAManager, std::hash <IDL::String>, std::equal_to <IDL::String>,
		Nirvana::Core::UserAllocator> ManagerMap;

	ManagerMap managers_;
};

inline
POAManager::POAManager (POAManagerFactory& factory, const IDL::String& id, const CORBA::PolicyList& policies) :
	IDL::String (id),
	factory_ (&factory),
	state_ (State::HOLDING),
	request_cnt_ (0),
	requests_completed_ (true),
	signature_ (SIGNATURE)
{
	if (!policies.empty ()) {
		policies_ = CORBA::make_reference <CORBA::Core::PolicyMapShared> ();
		for (const auto& p : policies) {
			policies_->insert (p);
		}
	}
}

inline
void POAManager::_delete_object () noexcept
{
	CORBA::servant_reference <POAManagerFactory> tmp (std::move (factory_));
	tmp->managers_.erase (*this);
}

}
}

#endif
