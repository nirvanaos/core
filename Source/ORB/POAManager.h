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
#ifndef NIRVANA_ORB_CORE_POAMANAGER_H_
#define NIRVANA_ORB_CORE_POAMANAGER_H_
#pragma once

#include "POA_Base.h"
#include <queue>

namespace PortableServer {
namespace Core {

class POAManagerFactory;

class POAManager : public CORBA::servant_traits <PortableServer::POAManager>::Servant <POAManager>
{
public:
	POAManager (POAManagerFactory& factory, const IDL::String& id, const CORBA::PolicyList& policies) :
		factory_ (factory),
		id_ (&id),
		state_ (State::HOLDING),
		request_cnt_ (0),
		requests_completed_ (true),
		signature_ (SIGNATURE)
	{}

	~POAManager ();

	POAManagerFactory& factory () const NIRVANA_NOEXCEPT
	{
		return factory_;
	}

	void activate ()
	{
		switch (state_) {
			case State::HOLDING:
				state_ = State::ACTIVE;
				if (!queue_.empty ()) {
					Nirvana::Core::SyncContext& sc = CORBA::Core::local2proxy (_this ())->sync_context ();
					do {
						const QElem& top = queue_.top ();
						if (!top.request->is_cancelled ()) {
							if (top.adapter->is_destroyed ())
								top.request->set_exception (POA::AdapterNonExistent ());
							on_request_start ();
							try {
								Nirvana::Core::ExecDomain::async_call (top.deadline,
									Nirvana::Core::CoreRef <Nirvana::Core::Runnable>::
									create <Nirvana::Core::ImplDynamic <ServeRequest> > (std::ref (top)), sc);
							} catch (CORBA::Exception& e) {
								top.request->set_exception (std::move (e));
								on_request_finish ();
							}
						}
						queue_.pop ();
					} while (!queue_.empty ());
				}
				break;

			case State::DISCARDING:
				state_ = State::ACTIVE;
			case State::ACTIVE:
				break;

			default:
				throw AdapterInactive ();
		}
	}

	void hold_requests (bool wait_for_completion)
	{
		if (State::INACTIVE == state_)
			throw AdapterInactive ();

		if (wait_for_completion)
			POA_Base::check_wait_completion ();

		state_ = State::HOLDING;

		if (wait_for_completion)
			requests_completed_.wait ();
	}

	void discard_requests (bool wait_for_completion)
	{
		if (State::INACTIVE == state_)
			throw AdapterInactive ();

		if (wait_for_completion)
			POA_Base::check_wait_completion ();

		if (state_ != State::DISCARDING) {
			state_ = State::DISCARDING;
			discard_queued_requests ();
		}

		if (wait_for_completion)
			requests_completed_.wait ();
	}

	void discard_queued_requests ();

	void deactivate (bool etherealize_objects, bool wait_for_completion)
	{
		if (wait_for_completion)
			POA_Base::check_wait_completion ();

		if (State::INACTIVE != state_) {
			state_ = State::INACTIVE;
			discard_queued_requests ();
			if (etherealize_objects)
				for (auto p : associated_adapters_) {
					p->etherealize_objects ();
				}
		}

		if (wait_for_completion)
			requests_completed_.wait ();
	}

	State get_state () const NIRVANA_NOEXCEPT
	{
		return state_;
	}

	const IDL::String& get_id () const NIRVANA_NOEXCEPT
	{
		return *id_;
	}

	static POAManager* get_implementation (const CORBA::Core::ProxyLocal* proxy)
	{
		if (proxy) {
			POAManager* impl = static_cast <POAManager*> (
				static_cast <Bridge <CORBA::LocalObject>*> (&proxy->servant ()));
			if (impl->signature_ == SIGNATURE)
				return impl;
			else
				throw CORBA::INV_OBJREF (); // User try to create own POAManager implementation?
		}
		return nullptr;
	}

	void invoke (POA_Base& adapter, const RequestRef& request)
	{
		switch (state_) {
			case State::HOLDING:
				queue_.emplace (adapter, request);
				break;
			case State::ACTIVE:
				on_request_start ();
				adapter.serve (request);
				break;
			case State::DISCARDING:
				throw CORBA::TRANSIENT (MAKE_OMG_MINOR (1));
				break;
			default:
				throw CORBA::OBJ_ADAPTER (MAKE_OMG_MINOR (1));
		}
	}

	void on_adapter_create (POA_Base& adapter)
	{
		associated_adapters_.push_back (&adapter);
	}

	void on_adapter_destroy (POA_Base& adapter) NIRVANA_NOEXCEPT
	{
		// POA are created and destroyed rarely so we use simple linear search.
		associated_adapters_.erase (
			std::find (associated_adapters_.begin (), associated_adapters_.end (), &adapter));
	}

	void on_request_start () NIRVANA_NOEXCEPT
	{
		if (1 == ++request_cnt_)
			requests_completed_.reset ();
	}

	void on_request_finish () NIRVANA_NOEXCEPT
	{
		if (0 == --request_cnt_)
			requests_completed_.signal ();
	}

private:
	struct QElem
	{
		Nirvana::DeadlineTime deadline;
		CORBA::servant_reference <POA_Base> adapter;
		RequestRef request;

		QElem (POA_Base& a, const RequestRef& r) :
			deadline (Nirvana::Core::ExecDomain::current ().deadline ()),
			adapter (&a),
			request (r)
		{}

		bool operator < (const QElem& rhs) const NIRVANA_NOEXCEPT
		{
			return deadline < rhs.deadline;
		}
	};

	class ServeRequest : public Nirvana::Core::Runnable
	{
	protected:
		ServeRequest (const QElem& qelem) :
			adapter_ (qelem.adapter),
			request_ (qelem.request)
		{}

	private:
		virtual void run () override;
		virtual void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT override;

	private:
		CORBA::servant_reference <POA_Base> adapter_;
		RequestRef request_;
	};

	POAManagerFactory& factory_;
	const IDL::String* id_;
	std::vector <POA_Base*> associated_adapters_;
	std::priority_queue <QElem> queue_;
	State state_;
	unsigned int request_cnt_;
	Nirvana::Core::Event requests_completed_;
	static const int32_t SIGNATURE = 'POAM';
	const int32_t signature_;
};

inline
PortableServer::POAManager::_ref_type POA_Base::the_POAManager () const
{
	return the_POAManager_->_this ();
}

inline
void POA_Base::invoke (const RequestRef& request)
{
	the_POAManager_->invoke (*this, request);
}

}
}

#endif
