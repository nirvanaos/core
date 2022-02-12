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
#include <iostream>

using namespace std;
using namespace Nirvana::Core;

namespace Nirvana {
namespace Legacy {
namespace Core {

void Process::copy_strings (const vector <StringView>& src, Strings& dst)
{
	dst.reserve (src.size ());
	for (const auto& sv : src) {
		dst.emplace_back (sv.data (), sv.size ());
	}
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
	Pointers v;
	v.reserve (argv_.size () + envp_.size () + 2);
	copy_strings (argv_, v);
	copy_strings (envp_, v);
	mutex_.construct (std::ref (*this));
	ret_ = main ((int)argv_.size (), v.data (), v.data () + argv_.size () + 1);
	mutex_.destruct ();
}

void Process::on_exception () NIRVANA_NOEXCEPT
{
	ret_ = -1;
	console_ << "Unhandled exception.\n";
}

void Process::on_crash (int error_code) NIRVANA_NOEXCEPT
{
	ret_ = -1;
	console_ << "Process crashed.\n";
}

RuntimeProxy::_ref_type Process::runtime_proxy_get (const void* obj)
{
	if (!RUNTIME_SUPPORT_DISABLE) {
		std::lock_guard <MutexCore> lock (mutex_);
		return MemContextEx::runtime_proxy_get (obj);
	} else
		return nullptr;
}

void Process::runtime_proxy_remove (const void* obj)
{
	if (!RUNTIME_SUPPORT_DISABLE) {
		mutex_->lock ();
		MemContextEx::runtime_proxy_remove (obj);
		mutex_->unlock ();
	}
}

void Process::on_object_construct (MemContextObject& obj)
{
	mutex_->lock ();
	MemContextEx::on_object_construct (obj);
	mutex_->unlock ();
}

void Process::on_object_destruct (MemContextObject& obj)
{
	mutex_->lock ();
	obj.remove ();
	mutex_->unlock ();
}

}
}
}
