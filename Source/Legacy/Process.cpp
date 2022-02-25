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
#include "Process.h"
#include "Synchronized.h"
#include <iostream>

using namespace std;
using namespace Nirvana::Core;

namespace Nirvana {
namespace Legacy {
namespace Core {

void Process::copy_strings (Strings& src, Pointers& dst)
{
	for (auto& s : src) {
		dst.push_back (const_cast <char*> (s.data ()));
	}
	dst.push_back (nullptr);
}

void Process::run ()
{
	assert (INIT == state_);
	state_ = RUNNING;

	sync_.construct (ref (sync_context ()), ref (*this));

	Pointers v;
	v.reserve (argv_.size () + envp_.size () + 2);
	copy_strings (argv_, v);
	copy_strings (envp_, v);
	ret_ = executable_.main ((int)argv_.size (), v.data (), v.data () + argv_.size () + 1);
	finish ();
}

void Process::finish () NIRVANA_NOEXCEPT
{
	get_TLS ().clear ();
	object_list_.clear ();
	runtime_support_.clear ();
	{
		Strings tmp (move (argv_));
	}
	{
		Strings tmp (move (envp_));
	}
	executable_.unbind ();
	state_ = COMPLETED;
	if (callback_) {
		try {
			callback_->on_process_finish (proxy_);
		} catch (...) {
		}
		callback_ = nullptr;
		proxy_ = nullptr;
	}
	sync_.destruct ();
}

void Process::on_exception () NIRVANA_NOEXCEPT
{
	ret_ = -1;
	finish ();
	console_ << "Unhandled exception.\n";
}

void Process::on_crash (const siginfo_t& signal) NIRVANA_NOEXCEPT
{
	if (SIGABRT == signal.si_signo)
		ret_ = 3;
	else
		ret_ = -1;
	finish ();
	console_ << "Process crashed.\n";
}

RuntimeProxy::_ref_type Process::runtime_proxy_get (const void* obj)
{
	RuntimeProxy::_ref_type ret;
	if (!RUNTIME_SUPPORT_DISABLE) {
		SYNC_BEGIN (sync_, nullptr);
		ret = runtime_support_.runtime_proxy_get (obj);
		SYNC_END ();
	}
	return ret;
}

void Process::runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT
{
	// Debug iterators
#ifdef _DEBUG
	if (RUNNING != state_)
		return;
#endif

	if (!RUNTIME_SUPPORT_DISABLE) {
		SYNC_BEGIN (sync_, nullptr);
		runtime_support_.runtime_proxy_remove (obj);
		SYNC_END ();
	}
}

void Process::on_object_construct (MemContextObject& obj) NIRVANA_NOEXCEPT
{
	SYNC_BEGIN (sync_, nullptr);
	object_list_.push_back (obj);
	SYNC_END ();
}

void Process::on_object_destruct (MemContextObject& obj) NIRVANA_NOEXCEPT
{
	SYNC_BEGIN (sync_, nullptr);
	obj.remove ();
	SYNC_END ();
}

TLS& Process::get_TLS () NIRVANA_NOEXCEPT
{
	return ThreadBase::current ().get_TLS ();
}

Process& Process::process () NIRVANA_NOEXCEPT
{
	return *this;
}

}
}
}
