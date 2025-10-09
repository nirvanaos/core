/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#ifndef NIRVANA_CORE_SYSTEMINFO_H_
#define NIRVANA_CORE_SYSTEMINFO_H_
#pragma once

#include <Port/SystemInfo.h>

namespace Nirvana {
namespace Core {

/// System information
class SystemInfo : private Port::SystemInfo
{
	using Base = Port::SystemInfo;

public:
	static bool initialize () noexcept
	{
		return Base::initialize ();
	}

	static void terminate () noexcept
	{
		Base::terminate ();
	}

	/// Get OLF section for the Core image.
	static const void* get_OLF_section (size_t& size) noexcept
	{
		return Base::get_OLF_section (size);
	}

	/// Base address of the Core image in memory.
	static const void* base_address () noexcept
	{
		return Base::base_address ();
	}

	static unsigned int hardware_concurrency () noexcept
	{
		return Base::hardware_concurrency ();
	}

	using Base::SUPPORTED_PLATFORM_CNT;

	static const uint16_t* supported_platforms () noexcept
	{
		return Base::supported_platforms ();
	}

};

}
}

#endif

