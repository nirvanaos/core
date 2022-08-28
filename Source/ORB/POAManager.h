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

class POA_Base;
class POAManager;
typedef Nirvana::Core::MapUnorderedStable <IDL::String, POAManager*,
	std::hash <IDL::String>, std::equal_to <IDL::String>,
	Nirvana::Core::UserAllocator <std::pair <IDL::String, POAManager*> > > POAManagerMap;

class POAManagerFactory;

class POAManager : public CORBA::servant_traits <PortableServer::POAManager>::Servant <POAManager>
{
public:
	POAManager (POAManagerFactory& factory, const IDL::String& id);
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
				while (!queue_.empty ()) {
					if (!queue_.top ().request->is_cancelled ()) {
						auto runnable = Nirvana::Core::CoreRef <Nirvana::Core::Runnable>::
							create <Nirvana::Core::ImplDynamic <ServeRequest> > (
								std::move (queue_.top ().adapter), std::move (queue_.top ().request));
						Nirvana::Core::ExecDomain::async_call (queue_.top ().deadline, runnable,
							Nirvana::Core::g_core_free_sync_context, queue_.top ().memory);
					}
					queue_.pop ();
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

		// Currently we do not support wait_for_completion.
		if (wait_for_completion)
			throw CORBA::BAD_INV_ORDER (MAKE_OMG_MINOR (3));

		state_ = State::HOLDING;
	}

	void discard_requests (bool wait_for_completion)
	{
		if (State::INACTIVE == state_)
			throw AdapterInactive ();

		// Currently we do not support wait_for_completion.
		if (wait_for_completion)
			throw CORBA::BAD_INV_ORDER (MAKE_OMG_MINOR (3));

		state_ = State::DISCARDING;
		discard_queued_requests ();
	}

	void discard_queued_requests ();

	void deactivate (bool etherealize_objects, bool wait_for_completion)
	{
		// Currently we do not support wait_for_completion.
		if (wait_for_completion)
			throw CORBA::BAD_INV_ORDER (MAKE_OMG_MINOR (3));

		if (State::INACTIVE != state_) {
			state_ = State::INACTIVE;
			discard_queued_requests ();
			if (etherealize_objects)
				for (auto p : associated_adapters_) {
					p->etherealize_objects ();
				}
		}
	}

	State get_state () const NIRVANA_NOEXCEPT
	{
		return state_;
	}

	const IDL::String& get_id () const NIRVANA_NOEXCEPT
	{
		return iterator_->first;
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

	void invoke (POA_Base& adapter, CORBA::Core::RequestInBase& request, Nirvana::Core::CoreRef <Nirvana::Core::MemContext>&& memory)
	{
		switch (state_) {
			case State::HOLDING:
				queue_.emplace (adapter, request, std::move (memory));
				break;
			case State::ACTIVE:
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

private:
	POAManagerFactory& factory_;
	POAManagerMap::iterator iterator_;

	class ServeRequest : public Nirvana::Core::Runnable
	{
	protected:
		ServeRequest (CORBA::servant_reference <POA_Base>&& a,
			Nirvana::Core::CoreRef <CORBA::Core::RequestInBase>&& r) :
			adapter (std::move (a)),
			request (std::move (r))
		{}

	private:
		virtual void run () override;
		virtual void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT override;

	private:
		CORBA::servant_reference <POA_Base> adapter;
		Nirvana::Core::CoreRef <CORBA::Core::RequestInBase> request;
	};

	struct QElem
	{
		Nirvana::DeadlineTime deadline;
		CORBA::servant_reference <POA_Base> adapter;
		Nirvana::Core::CoreRef <CORBA::Core::RequestInBase> request;
		Nirvana::Core::CoreRef <Nirvana::Core::MemContext> memory;

		QElem (POA_Base& a, CORBA::Core::RequestInBase& r,
			Nirvana::Core::CoreRef <Nirvana::Core::MemContext>&& m) :
			deadline (Nirvana::Core::ExecDomain::current ().deadline ()),
			adapter (&a),
			request (&r),
			memory (std::move (m))
		{}

		bool operator < (const QElem& rhs) const NIRVANA_NOEXCEPT
		{
			return deadline < rhs.deadline;
		}
	};

	std::vector <POA_Base*> associated_adapters_;
	std::priority_queue <QElem> queue_;
	State state_;
	static const int32_t SIGNATURE = 'POAM';
	const int32_t signature_;
};

inline
PortableServer::POAManager::_ref_type POA_Base::the_POAManager () const
{
	return the_POAManager_->_this ();
}

inline
void POA_Base::invoke (CORBA::Core::RequestInBase& request, Nirvana::Core::CoreRef <Nirvana::Core::MemContext>&& memory)
{
	the_POAManager_->invoke (*this, request, std::move (memory));
}

}
}

#endif
