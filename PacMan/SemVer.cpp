/*
* Nirvana package manager.
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
#include "SemVer.h"

SemVer::SemVer (const IDL::String& full_name) :
	version_ (0)
{
	// Version number begins from the first colon
	size_t ver = full_name.find ('.');
	if (std::string::npos == ver)
		name_ = full_name;
	else if (0 == ver)
		invalid_name (full_name);
	else {
		const char* p = full_name.data () + ver;
		++p;
		char* end;
		unsigned long major = strtoul (p, &end, 10);
		if (end == p)
			invalid_name (full_name);
		if (major == 0 || major > std::numeric_limits <uint16_t>::max ())
			invalid_name (full_name);
		unsigned long minor = 0, patch = 0;
		if ('.' == *end) {
			p = end + 1;
			minor = strtoul (p, &end, 10);
			if (end == p)
				invalid_name (full_name);
			if (minor > std::numeric_limits <uint16_t>::max ())
				invalid_name (full_name);
			if ('.' == *end) {
				p = end + 1;
				patch = strtoul (p, &end, 10);
				if (end == p)
					invalid_name (full_name);
				if (patch > std::numeric_limits <uint16_t>::max ())
					invalid_name (full_name);
			}
		}
		p = end;
		
		int pre = *p;
		switch (pre) {
		case 0:
			break;
		case '-':
			++p;
			break;
		case '+':
			pre = 0;
			break;
		default:
			invalid_name (full_name);
		}

		name_.assign (full_name, 0, ver);
		version_ = ((int64_t)major << 32) + (minor << 16) + patch;
		if (pre) {
			const char* end = full_name.data () + full_name.size ();
			end = std::find (p, end, '+');
			prerelease_.assign (p, end);
		}
	}
}

SemVer::SemVer (IDL::String&& name, int64_t version, IDL::String&& prerelease) noexcept :
	version_ (version),
	name_ (std::move (name)),
	prerelease_ (std::move (prerelease))
{}

IDL::String SemVer::to_string () const
{
	IDL::String s = name_;

	if (version_ || !prerelease_.empty ()) {
		uint16_t major = (uint16_t)(version_ >> 32);
		uint16_t minor = (uint16_t)(version_ >> 16);
		uint16_t patch = (uint16_t)version_;

		s += '.';
		s += std::to_string (major);
		s += '.';
		s += std::to_string (minor);
		if (patch) {
			s += '.';
			s += std::to_string (patch);
		}

		if (!prerelease_.empty ()) {
			s += '-';
			s += prerelease_;
		}
	}

	return s;
}

NIRVANA_NORETURN void SemVer::invalid_name (const IDL::String& full_name)
{
	Nirvana::BindError::throw_message ("Invalid name: " + full_name);
}
