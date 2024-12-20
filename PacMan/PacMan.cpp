/*
* Nirvana package manager.
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
#include "pch.h"
#include "PacMan.h"

PacMan::Lock::Lock (PacMan& obj) :
	obj_ (obj)
{
	if (!obj.completion_)
		Nirvana::throw_TRANSACTION_UNAVAILABLE ();

	if (obj.busy_)
		Nirvana::throw_INVALID_TRANSACTION ();

	obj.busy_ = true;
}

void PacMan::complete () noexcept
{
	if (completion_) {
		CosEventComm::PushConsumer::_ref_type tmp = std::move (completion_);
		sys_domain_ = nullptr;
		try {
			tmp->push (CORBA::Any ());
			tmp->disconnect_push_consumer ();
		} catch (...) {
			NIRVANA_ASSERT (false);
		}
	}
}

long PacMan::compare_name (const Nirvana::PM::ObjBinding& l, const Nirvana::PM::ObjBinding& r) noexcept
{
	long cmp = l.name ().compare (r.name ());
	if (cmp != 0)
		return cmp;

	return version (r.major (), r.minor ()) - version (l.major (), l.minor ());
}
