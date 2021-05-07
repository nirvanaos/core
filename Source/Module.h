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
#ifndef NIRVANA_CORE_MODULE_H_
#define NIRVANA_CORE_MODULE_H_

#include <CORBA/Server.h>
#include <Module_s.h>
#include <Nirvana/ModuleInit.h>
#include "AtomicCounter.h"
#include <Port/Module.h>
#include "CoreObject.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE Module :
	public CoreObject,
	public Port::Module,
	public CORBA::Nirvana::Servant <Module, ::Nirvana::Module>,
	public CORBA::Nirvana::LifeCycleRefCnt <Module>
{
public:
	virtual ~Module ();

	const void* base_address () const
	{
		return Port::Module::address ();
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

	bool singleton () const
	{
		return singleton_;
	}

protected:
	Module (const CoreString& name, bool singleton);

	static void initialize (ModuleInit::_ptr_type entry_point)
	{
		entry_point->initialize ();
	}

	static void terminate (ModuleInit::_ptr_type entry_point) NIRVANA_NOEXCEPT;

private:
	void on_release ()
	{}

protected:
	bool singleton_;
	AtomicCounter <false> ref_cnt_;
	ModuleInit::_ptr_type entry_point_;
};

}
}

#endif
