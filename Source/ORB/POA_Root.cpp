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

#include "POA_Root.h"
#include "RqHelper.h"
#include <CORBA/Servant_var.h>

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace CORBA;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

Object::_ref_type create_RootPOA ()
{
	auto manager_factory = CORBA::make_reference <POAManagerFactory> ();
	auto manager = manager_factory->create ("RootPOAManager", CORBA::PolicyList ());
	return CORBA::make_reference <POA_Root> (std::move (manager), std::move (manager_factory))->_this ();
}

void POA_Root::check_object_id (const ObjectId& oid)
{
	if (!oid.empty ()) // Empty ObjectId mean the SysDomain object.
		POA_ImplicitUnique::check_object_id (oid);
}

ReferenceLocalRef POA_Root::emplace_reference (ObjectKey&& key, bool unique, const IDL::String& primary_iid,
	unsigned flags)
{
	auto ins = references_.emplace (std::move (key), Reference::DEADLINE_MAX);
	References::reference entry = *ins.first;
	if (ins.second) {
		RefPtr p (new ReferenceLocal (entry.first, primary_iid, flags));
		Servant_var <ReferenceLocal> ret (p.get ());
		entry.second.finish_construction (std::move (p));
		return ret;
	} else if (unique)
		return nullptr;
	else {
		const RefPtr& p = entry.second.get ();
		p->check_primary_interface (primary_iid);
		return p.get ();
	}
}

ReferenceLocalRef POA_Root::emplace_reference (ObjectKey&& key, bool unique, ServantBase& servant,
	unsigned flags)
{
	auto ins = references_.emplace (std::move (key), Reference::DEADLINE_MAX);
	References::reference entry = *ins.first;
	if (ins.second) {
		RefPtr p (new ReferenceLocal (entry.first, servant, flags));
		Servant_var <ReferenceLocal> ret (p.get ());
		entry.second.finish_construction (std::move (p));
		return ret;
	} else if (unique)
		return nullptr;
	else {
		const RefPtr& p = entry.second.get ();
		p->check_primary_interface (servant.proxy ().primary_interface_id ());
		return p.get ();
	}
}

ReferenceLocalRef POA_Root::find_reference (const ObjectKey& key) NIRVANA_NOEXCEPT
{
	References::iterator it = references_.find (key);
	ReferenceLocalRef ref;
	if (it != references_.end ())
		ref = it->second.get ().get ();
	return ref;
}

POA_Ref POA_Root::find_child (const AdapterPath& path, bool activate_it)
{
	POA_Ref adapter = root_;
	for (const auto& name : path) {
		adapter = adapter->find_child (name, activate_it);
		if (!adapter)
			break;
	}
	return adapter;
}

void POA_Root::invoke (RequestRef request, bool async) NIRVANA_NOEXCEPT
{
	try {
		Object::_ref_type root = get_root (); // Hold root POA reference
		SYNC_BEGIN (local2proxy (root)->sync_context (), nullptr);

		invoke_sync (request);

		if (async) // Do not reschedule exec domain, it will be released immediately.
			_sync_frame.return_to_caller_context ();

		SYNC_END ();

	} catch (Exception& e) {
		request->set_exception (std::move (e));
	} catch (...) {
		request->set_unknown_exception ();
	}
}

void POA_Root::invoke_sync (const RequestRef& request)
{
	find_child (request->object_key ().adapter_path (), true)->invoke (request);
}

class POA_Root::InvokeAsync :
	public Runnable,
	public SharedObject
{
public:
	InvokeAsync (POA_Base* root, RequestRef&& request) :
		root_ (root),
		request_ (std::move (request))
	{}

private:
	virtual void run ();
	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT;

private:
	POA_Ref root_;
	RequestRef request_;
};

void POA_Root::invoke_async (RequestRef request, DeadlineTime deadline)
{
	const ProxyLocal* proxy = CORBA::Core::local2proxy (get_root ());
	POA_Base* adapter = get_implementation (proxy);
	assert (adapter);

	ExecDomain::async_call (deadline, CoreRef <Runnable>::create <ImplDynamic <InvokeAsync> >
		(adapter, std::move (request)), proxy->sync_context ());
}

void POA_Root::InvokeAsync::run ()
{
	try {
		invoke_sync (request_);
	} catch (Exception& e) {
		request_->set_exception (std::move (e));
	}
}

void POA_Root::InvokeAsync::on_exception () NIRVANA_NOEXCEPT
{
	request_->set_unknown_exception ();
}

void POA_Root::InvokeAsync::on_crash (const siginfo& signal) NIRVANA_NOEXCEPT
{
	Any a = RqHelper::signal2exception (signal);
	request_->set_exception (a);
}

}
}
