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

using namespace Nirvana::Core;
using namespace CORBA;

namespace Nirvana {
namespace Legacy {
namespace Core {

Process* Process::current_ptr () noexcept
{
	ThreadBase* thr = ThreadBase::current_ptr ();
	if (thr)
		return &thr->process ();
	else
		return nullptr;
}

Process::~Process ()
{}

Process& Process::process () noexcept
{
	return *this;
}

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

	try {
		Pointers v;
		v.reserve (argv_.size () + envp_.size () + 2);
		copy_strings (argv_, v);
		copy_strings (envp_, v);
		ret_ = executable_.main ((int)argv_.size (), v.data (), v.data () + argv_.size () + 1);
	} catch (const std::exception& ex) {
		ret_ = -1;
		std::string msg ("Unhandled exception: ");
		msg += ex.what ();
		msg += '\n';
		error_message (msg.c_str ());
	} catch (...) {
		ret_ = -1;
		error_message ("Unknown exception\n");
	}
	finish ();
}

void Process::finish () noexcept
{
	MemContextUser::clear ();
	{
		Strings tmp (std::move (argv_));
	}
	{
		Strings tmp (std::move (envp_));
	}
	executable_.unbind ();
	state_ = COMPLETED;
	completed_.signal ();
	if (callback_) {
		try {
			callback_->on_process_finish (_this ());
		} catch (...) {
		}
		callback_ = nullptr;
	}
}

void Process::on_crash (const siginfo& signal) noexcept
{
	if (SIGABRT == signal.si_signo)
		ret_ = 3;
	else
		ret_ = -1;
	error_message ("Process crashed.\n");
	finish ();
}

RuntimeGlobal& Process::runtime_global () noexcept
{
	return ThreadBase::current ().runtime_global ();
}

RuntimeProxy::_ref_type Process::runtime_proxy_get (const void* obj)
{
	assert (&MemContext::current () == this);

	RuntimeProxy::_ref_type ret;
	if (!RUNTIME_SUPPORT_DISABLE) {
		SYNC_BEGIN (*sync_domain_, nullptr);
		ret = MemContextUser::runtime_proxy_get (obj);
		SYNC_END ();
	}
	return ret;
}

void Process::runtime_proxy_remove (const void* obj) noexcept
{
	assert (&MemContext::current () == this);

	// Debug iterators
#ifdef NIRVANA_DEBUG_ITERATORS
	if (RUNNING != state_)
		return;
#endif

	if (!RUNTIME_SUPPORT_DISABLE) {
		SYNC_BEGIN (*sync_domain_, nullptr);
		MemContextUser::runtime_proxy_remove (obj);
		SYNC_END ();
	}
}

void Process::on_object_construct (MemContextObject& obj) noexcept
{
	assert (&MemContext::current () == this);

	SYNC_BEGIN (*sync_domain_, nullptr);
	MemContextUser::on_object_construct (obj);
	SYNC_END ();
}

void Process::on_object_destruct (MemContextObject& obj) noexcept
{
	assert (&MemContext::current () == this);

	SYNC_BEGIN (*sync_domain_, nullptr);
	MemContextUser::on_object_destruct (obj);
	SYNC_END ();
}

CosNaming::Name Process::get_current_dir_name () const
{
	assert (&MemContext::current () == this);

	SYNC_BEGIN (*sync_domain_, nullptr);
	return MemContextUser::get_current_dir_name ();
	SYNC_END ();
}

void Process::chdir (const IDL::String& path)
{
	assert (&MemContext::current () == this);

	SYNC_BEGIN (*sync_domain_, nullptr);
	MemContextUser::chdir (path);
	SYNC_END ();
}

unsigned Process::fd_open (const IDL::String& path, uint_fast16_t flags, mode_t mode)
{
	SYNC_BEGIN (*sync_domain_, nullptr);
	return MemContextUser::fd_open (path, flags, mode);
	SYNC_END ();
}

void Process::fd_close (unsigned fd)
{
	SYNC_BEGIN (*sync_domain_, nullptr);
	MemContextUser::fd_close (fd);
	SYNC_END ();
}

size_t Process::fd_read (unsigned fd, void* p, size_t size)
{
	SYNC_BEGIN (*sync_domain_, nullptr);
	return MemContextUser::fd_read (fd, p, size);
	SYNC_END ();
}

void Process::fd_write (unsigned fd, const void* p, size_t size)
{
	SYNC_BEGIN (*sync_domain_, nullptr);
	MemContextUser::fd_write (fd, p, size);
	SYNC_END ();
}

uint64_t Process::fd_seek (unsigned fd, const int64_t& off, unsigned method)
{
	SYNC_BEGIN (*sync_domain_, nullptr);
	return MemContextUser::fd_seek (fd, off, method);
	SYNC_END ();
}

void Process::error_message (const char* msg)
{
	fd_write (2, msg, strlen (msg));
}

}
}
}
