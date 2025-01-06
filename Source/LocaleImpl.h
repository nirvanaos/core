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
#ifndef NIRVANA_CORE_LOCALEIMPL_H_
#define NIRVANA_CORE_LOCALEIMPL_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/nls_s.h>
#include <Nirvana/locale.h>
#include "Heap.h"
#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

template <const struct lconv* LCONV, CORBA::Internal::Bridge <Nirvana::CodePage>* CP>
class LocaleStatic :
	public CORBA::servant_traits <Nirvana::Locale>::ServantStatic <LocaleStatic <LCONV, CP> >
{
public:
	Nirvana::CodePage::_ptr_type code_page () const noexcept
	{
		return Nirvana::CodePage::_ptr_type (static_cast <Nirvana::CodePage*> (CP));
	}

	const struct lconv* localeconv () const noexcept
	{
		return LCONV;
	}
};

extern const struct lconv lconv_posix;

inline Nirvana::Locale::_ptr_type locale_default ()
{
	return LocaleStatic <&lconv_posix, nullptr>::_get_ptr ();
}

/// \brief Locale is immutable object.
/// We can create a new one but can't change existing.
class Locale :
	public CORBA::servant_traits <Nirvana::Locale>::Servant <ImplDynamic <Locale> >,
	public CORBA::Internal::LifeCycleRefCnt <ImplDynamic <Locale> >,
	public SharedObject
{
public:
	friend class ImplDynamic <Locale>;

	static Nirvana::Locale::_ref_type create (unsigned mask, Nirvana::Locale::_ptr_type locale,
		Nirvana::Locale::_ptr_type base)
	{
		return CORBA::make_pseudo <ImplDynamic <Locale> > (mask, locale, base);
	}

	Nirvana::CodePage::_ptr_type code_page () const noexcept
	{
		return code_page_;
	}

	const struct lconv* localeconv () const noexcept
	{
		return &lconv_;
	}

private:
	Locale (unsigned mask, Nirvana::Locale::_ptr_type locale, Nirvana::Locale::_ptr_type base);

	void copy_monetary (const struct lconv* src);
	void copy_numeric (const struct lconv* src);

private:
	Nirvana::CodePage::_ref_type code_page_;
	struct lconv lconv_;

#pragma push_macro("STR")
#define STR(name, len) static const size_t name##_max_ = len; char name##_ [name##_max_ + 1]

	STR (decimal_point, 4);
	STR (thousands_sep, 4);
	STR (grouping, 5);
	STR (mon_decimal_point, 4);
	STR (mon_thousands_sep, 4);
	STR (mon_grouping, 5);

	STR (positive_sign, 4);
	STR (negative_sign, 4);

	STR (int_curr_symbol, 4);

	STR (currency_symbol, 4);

#pragma pop_macro("STR")

};

}
}

#endif
