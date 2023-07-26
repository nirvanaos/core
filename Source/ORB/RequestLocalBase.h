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
#ifndef NIRVANA_ORB_CORE_REQUESTLOCALBASE_H_
#define NIRVANA_ORB_CORE_REQUESTLOCALBASE_H_
#pragma once

#include "RequestLocalRoot.h"
#include "IndirectMap.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE RequestLocalBase : public RequestLocalRoot
{
	typedef RequestLocalRoot Base;

public:
	virtual void marshal_value (Internal::Interface::_ptr_type val) override;
	virtual Internal::Interface::_ref_type unmarshal_value (const IDL::String& interface_id) override;
	virtual void marshal_abstract (Internal::Interface::_ptr_type itf) override;
	virtual Internal::Interface::_ref_type unmarshal_abstract (const IDL::String& interface_id) override;

protected:
	RequestLocalBase (Nirvana::Core::MemContext* callee_memory, unsigned response_flags) noexcept :
		RequestLocalRoot (callee_memory, response_flags)
	{}

	virtual void reset () noexcept
	{
		Base::reset ();
		value_map_.clear ();
	}

private:
	void marshal_value (ValueBase::_ptr_type base, const IDL::String& interface_id);

private:
	IndirectMapItf value_map_;
};

}
}

#endif
