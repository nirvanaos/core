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

#include "NamingContextImpl.h"
#include <CORBA/Server.h>
#include <CORBA/CosNaming_s.h>
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

public:
	// NamingContext

	virtual void bind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) override;
	virtual void rebind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) override;
	virtual void bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) override;
	virtual void rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) override;
	virtual CORBA::Object::_ref_type resolve1 (const Istring& name, BindingType& type, Name& n) override;
	virtual NamingContext::_ref_type create_context1 (Istring&& name, Name& n, bool& created) override;
	virtual NamingContext::_ref_type bind_new_context1 (Istring&& name, Name& n) override;

	static void destroy ()
	{
		throw NotEmpty ();
	}

	// NamingContextEx

	static StringName to_string (Name& n)
	{
		check_name (n);
		Name::iterator it = n.begin ();
		StringName sn = Base::to_string (std::move (*(it++)));
		while (it != n.end ()) {
			sn += '/';
			sn += Base::to_string (std::move (*(it++)));
		}
		return sn;
	}

	static Name to_name (const StringName& sn)
	{
		Name n;
		size_t begin = 0;
		for (size_t end = 0; (end = sn.find ('/', end)) != StringName::npos;) {
			if (end > 0 && sn [end - 1] == '\\') { // Escaped
				++end;
				continue;
			}
			n.push_back (to_component (sn.substr (begin, end - begin)));
			begin = ++end;
		}
		n.push_back (to_component (sn.substr (begin)));
		return n;
	}

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

	static void shutdown (CORBA::Object::_ptr_type root) noexcept
	{
		const CORBA::Core::ServantProxyLocal* proxy = get_proxy (root);
		SYNC_BEGIN (proxy->sync_context (), nullptr);
		NameService* impl = static_cast <NameService*> (get_implementation (root));
		assert (impl);
		impl->shutdown ();
		SYNC_END ();
	}

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

private:
	void check_no_fs (const Istring& name);
};

}
}

#endif
