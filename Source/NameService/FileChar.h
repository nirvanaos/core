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
#ifndef NIRVANA_CORE_FILECHAR_H_
#define NIRVANA_CORE_FILECHAR_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/File_s.h>
#include "../FileAccessChar.h"

namespace Nirvana {
namespace Core {

class FileChar : public CORBA::servant_traits <Nirvana::File>::Servant <FileChar>
{
public:
	FileChar () :
		access_ (nullptr)
	{}

	static FileType type () noexcept
	{
		return FileType::character;
	}

	static void stat (FileStat& st) noexcept
	{
		zero (st);
	}

	static void remove ()
	{
		throw_NO_IMPLEMENT ();
	}

	static FileSize size () noexcept
	{
		return 0;
	}

	Access::_ref_type open (uint_fast16_t flags, uint_fast16_t mode)
	{
		Access::_ref_type ret;
		if (!access_) {
			Ref <FileAccessChar> access = create_access (flags);
			ret = open (*access, flags);
			access_ = access;
		} else {
			access_->check_flags (flags);
			ret = open (*access_, flags);
		}
		return ret;
	}

	void on_access_destroy () noexcept
	{
		access_ = nullptr;
	}

protected:
	virtual Ref <FileAccessChar> create_access (unsigned flags) = 0;

private:
	static Access::_ref_type open (FileAccessChar& access, unsigned flags);

private:
	FileAccessChar* access_;
};

}
}

#endif
