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
#ifndef NIRVANA_ORB_CORE_TC_NATIVE_H_
#define NIRVANA_ORB_CORE_TC_NATIVE_H_
#pragma once

#include "TC_IdName.h"
#include "TC_Impl.h"

namespace CORBA {
namespace Core {

class TC_Native :
	public TC_Impl <TC_Native, TC_IdName>,
	public Internal::TypeCodeOps <void>
{
	typedef TC_Impl <TC_Native, TC_IdName> Impl;
	typedef Internal::TypeCodeOps <void> Ops;

public:
	using Servant::_s_id;
	using Servant::_s_name;
	using Ops::_s_n_size;
	using Ops::_s_n_align;
	using Ops::_s_n_is_CDR;
	using Ops::_s_n_construct;
	using Ops::_s_n_destruct;
	using Ops::_s_n_copy;
	using Ops::_s_n_move;
	using Ops::_s_n_marshal_in;
	using Ops::_s_n_marshal_out;
	using Ops::_s_n_unmarshal;
	using Ops::_s_n_byteswap;

	TC_Native (IDL::String&& id, IDL::String&& name) NIRVANA_NOEXCEPT :
		Impl (TCKind::tk_native, std::move (id), std::move (name))
	{}
};

}
}

#endif
