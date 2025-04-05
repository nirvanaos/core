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
#ifndef NIRVANA_CORE_LOCALECONTEXT_H_
#define NIRVANA_CORE_LOCALECONTEXT_H_
#pragma once

#include "UserAllocator.h"
#include <Nirvana/nls.h>
#include <vector>

namespace Nirvana {
namespace Core {

/// Context for the POSIX locale operations.
class LocaleContext
{
public:
	typedef int locale_t;

	LocaleContext () :
		cur_locale_ (0)
	{}

	locale_t cur_locale () const noexcept
	{
		return cur_locale_;
	}

	Nirvana::Locale::_ptr_type get_locale () const noexcept
	{
		if (cur_locale_ <= 0)
			return nullptr;
		return locales_ [cur_locale_ - 1];
	}

	Nirvana::Locale::_ptr_type get_locale (locale_t locobj) const noexcept
	{
		if (locobj > 0 && locobj <= locales_.size ())
			return locales_ [locobj - 1];
		else
			return nullptr;
	}

	locale_t add_locale (Nirvana::Locale::_ptr_type loc)
	{
		locale_t ret;
		auto it = locales_.begin ();
		for (; it != locales_.end (); ++it) {
			if (*it)
				break;
		}
		if (it == locales_.end ()) {
			locales_.emplace_back (loc);
			ret = (locale_t)locales_.size ();
		} else {
			*it = loc;
			ret = (locale_t)(it - locales_.begin () + 1);
		}
		return ret;
	}

	void freelocale (locale_t locobj) noexcept
	{
		if (locobj > 0 && locobj <= locales_.size ()) {
			int i = locobj - 1;
			locales_ [i] = nullptr;
			if (cur_locale_ == locobj)
				cur_locale_ = 0;
		}
	}

	locale_t uselocale (locale_t locobj)
	{
		locale_t ret = cur_locale_;
		if (locobj > 0) {
			int i = locobj - 1;
			if (locales_ [i])
				cur_locale_ = locobj;
			else
				throw_BAD_PARAM (make_minor_errno (EINVAL));
		} else
			cur_locale_ = 0;
		return ret;
	}
	
private:
	typedef Nirvana::Locale::_ref_type LocaleRef;
	typedef std::vector <LocaleRef, UserAllocator <LocaleRef> > Locales;
	Locales locales_;
	locale_t cur_locale_;
};

}
}

#endif
