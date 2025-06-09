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
#include "pch.h"
#include <CORBA/Server.h>
#include <Nirvana/Module_s.h>

namespace Nirvana {
namespace Core {

/// Module interface implementation for core static objects.
class CoreModule :
	public CORBA::servant_traits <Nirvana::Module>::ServantStatic <CoreModule>
{
public:
	static const void* base_address () noexcept
	{
		return nullptr;
	}

	static int32_t id () noexcept
	{
		return 0;
	}

	template <class Itf>
	static CORBA::Internal::Interface* __duplicate (CORBA::Internal::Interface* itf, CORBA::Internal::Interface*)
	{
		return itf;
	}

	template <class Itf>
	static void __release (CORBA::Internal::Interface*)
	{}

	void atexit (AtExitFunc f)
	{
		assert (at_exit_cnt_ < MAX_AT_EXIT_ENTRIES);
		at_exit_table_ [at_exit_cnt_++] = f;
	}

	static unsigned CS_alloc (Deleter deleter)
	{
		throw_NO_IMPLEMENT ();
	}

	static void CS_free (unsigned idx)
	{
		throw_NO_IMPLEMENT ();
	}

	static void CS_set (unsigned idx, void* p)
	{
		throw_NO_IMPLEMENT ();
	}

	static void* CS_get (unsigned idx) noexcept
	{
		return nullptr;
	}

private:
	static const size_t MAX_AT_EXIT_ENTRIES = 8;
	static AtExitFunc at_exit_table_ [MAX_AT_EXIT_ENTRIES];
	static size_t at_exit_cnt_;
};

AtExitFunc CoreModule::at_exit_table_ [MAX_AT_EXIT_ENTRIES];
size_t CoreModule::at_exit_cnt_;

}
}
