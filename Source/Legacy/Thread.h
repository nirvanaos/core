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
#ifndef NIRVANA_LEGACY_CORE_THREAD_H_
#define NIRVANA_LEGACY_CORE_THREAD_H_
#pragma once

#include "ThreadBase.h"
#include "../Event.h"
#include "../UserObject.h"
#include <CORBA/Server.h>
#include <Nirvana/Legacy/Legacy_s.h>
#include <Nirvana/Legacy/Runnable.h>

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Legacy thread
class Thread :
	public CORBA::servant_traits <Nirvana::Legacy::Thread>::Servant <Thread>,
	public CORBA::Internal::LifeCycleRefCnt <Thread>,
	public Nirvana::Core::UserObject,
	public ThreadBase,
	public SimpleList <Thread>::Element
{
public:
	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

	void join ()
	{
		event_.wait ();
	}

	~Thread ();

private:
	template <class S, class ... Args> friend
		CORBA::servant_reference <S> CORBA::make_reference (Args ...);

	Thread (Process& process, Nirvana::Legacy::Runnable::_ptr_type runnable) :
		process_ (process),
		runnable_ (runnable)
	{
		if (!runnable)
			throw_BAD_PARAM ();
	}

	// Core::Runnable::
	virtual void run () override;
	virtual void on_crash (const siginfo& signal) noexcept override;

	// ThreadBase::
	virtual Process& process () noexcept override
	{
		return process_;
	}

private:
	Process& process_;
	Nirvana::Legacy::Runnable::_ref_type runnable_;
	Nirvana::Core::Event event_;
	Nirvana::Core::RefCounter ref_cnt_;
};

}
}
}

#endif
