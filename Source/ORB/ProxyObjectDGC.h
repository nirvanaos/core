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
#ifndef NIRVANA_ORB_CORE_PROXYOBJECTDGC_H_
#define NIRVANA_ORB_CORE_PROXYOBJECTDGC_H_
#pragma once

#include "ProxyObject.h"

namespace CORBA {
namespace Core {

/// \brief Server-side Object proxy with implicit deactivation.
/// 
/// This object is subject of the Distributed Garbage Collection (DGC).
class NIRVANA_NOVTABLE ProxyObjectDGC : public ProxyObject
{
	typedef ProxyObject Base;

public:
	virtual void activate (PortableServer::Core::ObjectKey&& key) override;
	virtual void deactivate () NIRVANA_NOEXCEPT override;

protected:
	ProxyObjectDGC (PortableServer::Servant servant, PortableServer::POA::_ptr_type adapter);
	ProxyObjectDGC (const ProxyObject& src, PortableServer::POA::_ptr_type adapter);

	~ProxyObjectDGC ()
	{}

	enum ActivationState
	{
		INACTIVE,
		IMPLICIT_ACTIVATION,
		ACTIVE,
		DEACTIVATION_SCHEDULED,
		DEACTIVATION_CANCELLED,
		ACTIVE_EXPLICIT
	};

	class Deactivator;

	bool change_state (ActivationState from, ActivationState to) NIRVANA_NOEXCEPT
	{
		return activation_state_.compare_exchange_strong (from, to);
	}

	bool is_DGC_subject () const NIRVANA_NOEXCEPT
	{
		return activation_state_ != ACTIVE_EXPLICIT;
	}

	void implicit_deactivate ();

private:
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	template <class> friend class Nirvana::Core::CoreRef;

protected:
	PortableServer::POA::_ref_type adapter_;
	std::atomic <ActivationState> activation_state_;
};

}
}

#endif
