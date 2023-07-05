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
#ifndef NIRVANA_NAMESERVICE_DIRITER_H_
#define NIRVANA_NAMESERVICE_DIRITER_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "../CoreInterface.h"
#include <regex>

namespace CosNaming {
namespace Core {

class Iterator;

}
}

namespace Nirvana {
namespace Core {

class Dir;

class DirIter
{
public:
	DirIter (Dir& dir, const std::string& regexp, unsigned flags);

	static Nirvana::DirIterator::_ref_type create_iterator (std::unique_ptr <DirIter>&& vi);

	bool next_one (DirEntry& de);
	bool next_n (uint32_t how_many, DirEntryList& l);

	bool end () const noexcept
	{
		return !iterator_;
	}

private:
	static std::regex::flag_type get_regex_flags (unsigned flags);

	Ref <Dir> dir_;
	std::unique_ptr <CosNaming::Core::Iterator> iterator_;
	std::regex regex_;
	unsigned flags_;
	static const unsigned USE_REGEX = 0x8000;
};

}
}

#endif
