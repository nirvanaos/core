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
#ifndef NIRVANA_NAMESERVICE_NAMESERVICE_H_
#define NIRVANA_NAMESERVICE_NAMESERVICE_H_
#pragma once

#include "NamingContextDefault.h"
#include "ORB/SysServant.h"
#include "ORB/system_services.h"

namespace CosNaming {
namespace Core {

/// Naming Service root
class NameService : 
	public CORBA::Core::SysServantImpl <NameService, NamingContextExt, NamingContext>,
	public NamingContextImpl
{
	typedef NamingContextImpl Base;

	static const TimeBase::TimeT FS_CREATION_DEADLINE = 1 * TimeBase::MILLISECOND;

public:
	NameService () :
		file_system_ (nullptr)
	{}

	~NameService ()
	{}

	// NamingContext

	virtual void bind1 (Name& n, CORBA::Object::_ptr_type obj) override;
	virtual void rebind1 (Name& n, CORBA::Object::_ptr_type obj) override;
	virtual void bind_context1 (Name& n, NamingContext::_ptr_type nc) override;
	virtual void rebind_context1 (Name& n, NamingContext::_ptr_type nc) override;
	virtual CORBA::Object::_ref_type resolve1 (Name& n) override;
	virtual NamingContext::_ref_type bind_new_context1 (Name& n) override;

	static void destroy ()
	{
		throw NotEmpty ();
	}

	// NamingContextEx

	static StringName to_string (Name& n)
	{
		NamingContextRoot::check_name (n);
		StringName sn;
		if (n.size () > 1)
			sn = to_string_unchecked (n);
		else
			append_string (sn, std::move (n.front ()));
		return sn;
	}

	static StringName to_string_unchecked (const Name& n);

	static Name to_name (const StringName& sn);

	static URLString to_url (const Address& addr, const StringName& sn)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	CORBA::Object::_ref_type resolve_str (const StringName& sn)
	{
		Name name = to_name (sn);
		return resolve (name);
	}

	// Details

	virtual NamingContext::_ptr_type this_context () override
	{
		return _this ();
	}

	static void shutdown (CORBA::Object::_ptr_type root)
	{
		const CORBA::Core::ServantProxyLocal* proxy = get_proxy (root);
		SYNC_BEGIN (proxy->sync_context (), nullptr);
		NameService* impl = static_cast <NameService*> (get_implementation (root));
		assert (impl);
		impl->shutdown ();
		SYNC_END ();
	}

protected:
	void shutdown () noexcept
	{
		NamingContextImpl::shutdown ();
	}

	virtual void add_link (const NamingContextImpl& parent) override
	{}

	virtual bool remove_link (const NamingContextImpl& parent) override
	{
		return true;
	}
	
	virtual bool is_cyclic (ContextSet& parents) const override
	{
		return false;
	}

	virtual void get_bindings (IteratorStack& iter) const override;

private:
	static bool is_file_system (const NameComponent& nc);
	
	static const char file_system_name_ [];

	static void check_no_fs (const Name& n);

	Nirvana::Core::WaitableRef <CORBA::Object::_ref_type> file_system_;
};

}
}

#endif
