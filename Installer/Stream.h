/*
* Nirvana Installer module.
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
#ifndef NIRVANA_INSTALLER_STREAM_H_
#define NIRVANA_INSTALLER_STREAM_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include <streambuf>
#include <istream>

namespace Installer {

class StreamBuf : public std::streambuf
{
public:
	StreamBuf (Nirvana::AccessBuf::_ref_type& access) :
		access_ (std::move (access))
	{}

protected:
	virtual std::streamsize xsgetn (char_type* s, std::streamsize count) override
	{
		return access_->read (s, count);
	}

private:
	Nirvana::AccessBuf::_ref_type access_;
};

class Stream : public std::istream
{
public:
	Stream (Nirvana::AccessBuf::_ref_type& access) :
		std::istream (&streambuf_),
		streambuf_ (std::move (access))
	{}

private:
	StreamBuf streambuf_;
};

}

#endif
