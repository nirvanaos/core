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

bool SemVer::from_string (const IDL::String& full_name)
{
	// Version number begins from the first colon
	size_t ver = full_name.find ('.');
	if (std::string::npos == ver) {
		name = full_name;
		version = 0;
		prerelease.clear ();
	} else if (0 == ver)
		return false;
	else {
		const char* p = full_name.data () + ver;
		++p;
		char* end;
		unsigned long major = strtoul (p, &end, 10);
		if (end == p)
			return false;
		if (major == 0 || major > std::numeric_limits <uint16_t>::max ())
			return false;
		unsigned long minor = 0, patch = 0;
		if ('.' == *end) {
			p = end + 1;
			minor = strtoul (p, &end, 10);
			if (end == p)
				return false;
			if (minor > std::numeric_limits <uint16_t>::max ())
				return false;
			if ('.' == *end) {
				p = end + 1;
				patch = strtoul (p, &end, 10);
				if (end == p)
					return false;
				if (patch > std::numeric_limits <uint16_t>::max ())
					return false;
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
			return false;
		}

		name.assign (full_name, 0, ver);
		version = ((int64_t)major << 32) + (minor << 16) + patch;
		if (pre) {
			const char* end = full_name.data () + full_name.size ();
			end = std::find (p, end, '+');
			prerelease.assign (p, end);
		} else
			prerelease.clear ();
	}
	return true;
}

IDL::String SemVer::to_string () const
{
	IDL::String s = name;

	if (version || !prerelease.empty ()) {
		uint16_t major = (uint16_t)(version >> 32);
		uint16_t minor = (uint16_t)(version >> 16);
		uint16_t patch = (uint16_t)version;

		s += '.';
		s += std::to_string (major);
		s += '.';
		s += std::to_string (minor);
		if (patch) {
			s += '.';
			s += std::to_string (patch);
		}

		if (!prerelease.empty ()) {
			s += '-';
			s += prerelease;
		}
	}

	return s;
}
