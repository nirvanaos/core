/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#ifndef NIRVANA_CORE_MODULE_H_
#define NIRVANA_CORE_MODULE_H_

#include <Nirvana/Module_s.h>
#include "AtomicCounter.h"
#include <CORBA/LifeCycleRefCnt.h>

namespace Nirvana {
namespace Core {

class Module :
	public CORBA::Nirvana::Servant <Module, ::Nirvana::Module>,
	public CORBA::Nirvana::LifeCycleRefCnt <Module>
{
public:
	Module (const void* base_address) :
		module_entry_ (nullptr),
		base_address_ (base_address)
	{}

	const void* base_address () const
	{
		return base_address_;
	}

	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement ())
			on_release ();
	}

private:
	void on_release ()
	{}

public:
	const ImportInterface* module_entry_;
	const void* base_address_;

private:
	RefCounter ref_cnt_;
};

}
}

#endif
