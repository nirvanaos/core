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

#define LCONV_STR(t) const_cast <char*> (t)

namespace Nirvana {
namespace Core {

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

#define INIT_STR(name) name##_ [name##_max_] = 0; lconv_.name = name##_
#define COPY_STR(src, name) strncpy (name##_, src->name, name##_max_);
#define COPY_CHAR(src, name) lconv_.name = src->name

Locale::Locale (unsigned mask, Nirvana::Locale::_ptr_type locale, Nirvana::Locale::_ptr_type base)
{
  assert (locale);
  if (!base)
    base = locale_default ();

  INIT_STR (decimal_point);
  INIT_STR (thousands_sep);
  INIT_STR (grouping);
  INIT_STR (mon_decimal_point);
  INIT_STR (mon_thousands_sep);
  INIT_STR (mon_grouping);
  INIT_STR (positive_sign);
  INIT_STR (negative_sign);
  INIT_STR (int_curr_symbol);
  INIT_STR (currency_symbol);

  const struct lconv* lconv_loc = locale->localeconv ();
  const struct lconv* lconv_base = base->localeconv ();

  if (mask & LC_CTYPE_MASK)
    code_page_ = locale->code_page ();
  else
    code_page_ = base->code_page ();

  copy_numeric ((mask & LC_NUMERIC_MASK) ? lconv_loc : lconv_base);
  copy_monetary ((mask & LC_MONETARY_MASK) ? lconv_loc : lconv_base);
}

inline void Locale::copy_monetary (const struct lconv* src)
{
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

inline void Locale::copy_numeric (const struct lconv* src)
{
  COPY_STR (src, decimal_point);
  COPY_STR (src, thousands_sep);
  COPY_STR (src, grouping);
}

}
}
