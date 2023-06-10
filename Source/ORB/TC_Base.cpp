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
#include "TC_Base.h"
#include "Services.h"
#include "ServantProxyLocal.h"

using Nirvana::Core::SyncContext;

namespace CORBA {
namespace Core {

void TC_Base::_s_n_byteswap (Internal::Bridge <TypeCode>*, void*, size_t, Internal::Interface*)
{}

void TC_Base::_add_ref () noexcept
{
	ref_cnt_.increment ();
}

void TC_Base::_remove_ref () noexcept
{
	if (0 == ref_cnt_.decrement ())
		collect_garbage ();
}

void TC_Base::collect_garbage () noexcept
{
	try {
		SyncContext& sc = local2proxy (Services::bind (Services::TC_Factory))->sync_context ();
		if (&SyncContext::current () == &sc)
			delete this;
		else
			GarbageCollector::schedule (*this, sc);
	} catch (...) {}
}

}
}
