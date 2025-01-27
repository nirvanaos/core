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
#include "pch.h"
#include "LocaleImpl.h"
#include <Nirvana/locale.h>
#include "Heap.h"
#include "CoreInterface.h"
#include <algorithm>
#include <iterator>
#include <boost/preprocessor/repetition/repeat.hpp>

#define UTF8_CP_NAME "UTF-8"

#define LOCALE_DEF(name, lc) { name "." UTF8_CP_NAME, &lc }

#define LCONV_STR(t) const_cast <char*> (t)

namespace Nirvana {
namespace Core {

template <const LocaleDef* def>
class LocaleStatic :
	public CORBA::servant_traits <Nirvana::Locale>::ServantStatic <LocaleStatic <def> >
{
public:
	static Nirvana::CodePage::_ptr_type code_page () noexcept
	{
		return nullptr;
	}

	static const struct lconv* localeconv () noexcept
	{
		return def->lc;
	}

	static const char* get_name (int category) noexcept
	{
		if (category == LC_CTYPE)
			return strchr (def->name, '.') + 1;
		else
			return def->name;
	}
};

class LocaleDefCompare
{
public:
	bool operator () (const LocaleDef& l, const char* r) noexcept
	{
		return compare (l, r) < 0;
	}

	bool operator () (const char* l, const LocaleDef& r) noexcept
	{
		return compare (r, l) > 0;
	}

private:
	static int compare (const LocaleDef& l, const char* s) noexcept;
};

int LocaleDefCompare::compare (const LocaleDef& l, const char* s) noexcept
{
	const char* locale_name = l.name;
	for (;;) {
		char lc = *locale_name;
		char c = *s;
		if (!lc || '.' == lc)
			if (!c || '.' == c)
				return 0;
			else
				return -1;
		else if (!c || '.' == c)
			return 1;

		if ('A' <= c && c <= 'Z')
			c += 0x20;

		if (lc < c)
			return -1;
		else if (lc > c)
			return 1;

		++locale_name;
		++s;
	}
}

#define LOCALE_DEF_CNT 2

const struct lconv lconv_posix = {
	LCONV_STR ("."),
	LCONV_STR (""),
	LCONV_STR (""),
	LCONV_STR (""),
	LCONV_STR (""),
	LCONV_STR (""),
	LCONV_STR (""),
	LCONV_STR ("-"),
	LCONV_STR (""),
	LCONV_STR (""),
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX
};

const LocaleDef locale_defs [LOCALE_DEF_CNT] = {
	LOCALE_DEF ("c", lconv_posix),
	LOCALE_DEF ("posix", lconv_posix)
};

#define LOCALE_PTR(z, n, d) LocaleStatic <&locale_defs [n]>::_get_ptr (),

Nirvana::Locale* const locale_ptrs [LOCALE_DEF_CNT] = {
	BOOST_PP_REPEAT (LOCALE_DEF_CNT, LOCALE_PTR, 0)
};

Nirvana::Locale::_ref_type locale_get_utf8 (const char* name)
{
	auto f = std::lower_bound (locale_defs, std::end (locale_defs), name, LocaleDefCompare ());
	if (f != std::end (locale_defs))
		return Nirvana::Locale::_ptr_type (locale_ptrs [f - locale_defs]);
	return nullptr;
}

/// \brief Locale is immutable object.
/// We can create a new one but can't change existing.
class Locale :
	public CORBA::servant_traits <Nirvana::Locale>::Servant <ImplDynamic <Locale> >,
	public CORBA::Internal::LifeCycleRefCnt <ImplDynamic <Locale> >,
	public SharedObject
{
public:
	Nirvana::CodePage::_ptr_type code_page () const noexcept
	{
		return code_page_;
	}

	const struct lconv* localeconv () const noexcept
	{
		return &lconv_;
	}

	const char* get_name (unsigned category) const noexcept
	{
		if (category < std::size (names_))
			return names_ [category];
		else
			return nullptr;
	}

protected:
	Locale (unsigned mask, const char* locale, Nirvana::Locale::_ptr_type base);

private:
	void copy_code_page (Nirvana::Locale::_ptr_type src);
	void copy_monetary (Nirvana::Locale::_ptr_type src);
	void copy_numeric (Nirvana::Locale::_ptr_type src);

private:
	Nirvana::CodePage::_ref_type code_page_;
	struct lconv lconv_;
	Nirvana::Locale::_ref_type parent_, base_;
	const char* names_ [LC_TIME + 1];
};

static Nirvana::Locale::_ref_type create (unsigned mask, const char* locale,
	Nirvana::Locale::_ptr_type base)
{
	return CORBA::make_pseudo <ImplDynamic <Locale> > (mask, locale, base);
}

inline
Locale::Locale (unsigned mask, const char* name, Nirvana::Locale::_ptr_type base)
{
	if (mask & ~LC_ALL_MASK)
		throw_BAD_PARAM (make_minor_errno (EINVAL));

	if (!name [0])
		name = "c";

	parent_ = locale_get_utf8 (name);
	if (!parent_)
		throw_BAD_PARAM (make_minor_errno (ENOENT));

	if ((mask & LC_ALL_MASK) != LC_ALL_MASK) {
		if (base)
			base_ = base;
		else
			base_ = locale_default ();
	}

	std::fill (names_, std::end (names_), nullptr);
	names_ [LC_ALL] = parent_->get_name (LC_ALL);

	copy_code_page ((mask & LC_CTYPE_MASK) ? parent_ : base_);
	copy_numeric ((mask & LC_NUMERIC_MASK) ? parent_ : base_);
	copy_monetary ((mask & LC_MONETARY_MASK) ? parent_ : base_);
}

#define COPY_STR(lc, name) lconv_.name = lc->name
#define COPY_CHAR(lc, name) lconv_.name = lc->name

inline void Locale::copy_code_page (Nirvana::Locale::_ptr_type src)
{
	code_page_ = src->code_page ();
	names_ [LC_CTYPE] = src->get_name (LC_CTYPE);
}

inline void Locale::copy_numeric (Nirvana::Locale::_ptr_type loc)
{
	names_ [LC_NUMERIC] = loc->get_name (LC_NUMERIC);
	const struct lconv* src = loc->localeconv ();

	COPY_STR (src, decimal_point);
	COPY_STR (src, thousands_sep);
	COPY_STR (src, grouping);
}

inline void Locale::copy_monetary (Nirvana::Locale::_ptr_type loc)
{
	names_ [LC_MONETARY] = loc->get_name (LC_MONETARY);
	const struct lconv* src = loc->localeconv ();

	COPY_STR (src, mon_decimal_point);
	COPY_STR (src, mon_thousands_sep);
	COPY_STR (src, mon_grouping);
	COPY_STR (src, positive_sign);
	COPY_STR (src, negative_sign);
	COPY_STR (src, int_curr_symbol);
	COPY_STR (src, currency_symbol);

	COPY_CHAR (src, int_frac_digits);
	COPY_CHAR (src, frac_digits);
	COPY_CHAR (src, p_cs_precedes);
	COPY_CHAR (src, p_sep_by_space);
	COPY_CHAR (src, n_cs_precedes);
	COPY_CHAR (src, n_sep_by_space);
	COPY_CHAR (src, p_sign_posn);
	COPY_CHAR (src, n_sign_posn);
	COPY_CHAR (src, int_p_cs_precedes);
	COPY_CHAR (src, int_n_cs_precedes);
	COPY_CHAR (src, int_p_sep_by_space);
	COPY_CHAR (src, int_n_sep_by_space);
	COPY_CHAR (src, int_p_sign_posn);
	COPY_CHAR (src, int_n_sign_posn);
}

Nirvana::Locale::_ref_type locale_create (unsigned mask, const IDL::String& locale,
	Nirvana::Locale::_ptr_type base)
{
	return CORBA::make_pseudo <ImplDynamic <Locale> > (mask, locale.c_str (), base);
}

}
}
