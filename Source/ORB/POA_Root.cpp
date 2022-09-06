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
#include "Services.h"
#include "RqHelper.h"

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace CORBA;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

POA* POA_Root::root_ = nullptr;

POA::_ref_type POA_Root::get_root ()
{
	if (!root_) {
		Object::_ref_type svc = CORBA::Core::Services::bind (CORBA::Core::Services::RootPOA);
		POA::_ref_type adapter = POA::_narrow (svc);
		if (!adapter)
			throw INV_OBJREF ();
		root_ = static_cast <POA*> (&(POA::_ptr_type)adapter);
		return adapter;
	} else
		return POA::_ptr_type (root_);
}

void POA_Root::invoke (RequestRef request, bool async) NIRVANA_NOEXCEPT
{
	try {
		POA::_ref_type root = get_root (); // Hold root POA reference
		const ProxyLocal* proxy = local2proxy (root);
		POA_Ref adapter = get_implementation (proxy);
		assert (adapter);

		SYNC_BEGIN (proxy->sync_context (), nullptr);

		invoke_sync (std::move (adapter), request);

		if (async) // Do not reschedule exec domain, it will be released immediately.
			_sync_frame.return_to_caller_context ();

		SYNC_END ();

	} catch (Exception& e) {
		request->set_exception (std::move (e));
	} catch (...) {
		request->set_unknown_exception ();
	}
}

void POA_Root::invoke_sync (POA_Ref adapter, const RequestRef& request)
{
	const ObjectKey& object_key = request->object_key ();
	for (const auto& name : object_key.adapter_path ()) {
		adapter = &adapter->find_child (name, true);
	}
	adapter->invoke (request);
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
	servant_reference <POA_Base> root_;
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
		invoke_sync (std::move (root_), request_);
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
