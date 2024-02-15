/*
* Nirvana SFloat module.
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
#include "Context.h"
#include <Nirvana/System.h>

extern "C" {

#include "softfloat.h"
#include "specialize.h"

}

extern "C" const uint_fast8_t softfloat_roundingMode = softfloat_round_near_even;
extern "C" const uint_fast8_t softfloat_detectTininess = init_detectTininess;
extern "C" const uint_fast8_t extF80_roundingPrecision = 80;

namespace CORBA {
namespace Internal {

SFloatTLS::SFloatTLS ()
{
	tls_idx_ = Nirvana::g_system->TLS_alloc (nullptr);
}

SFloatTLS::~SFloatTLS ()
{
	Nirvana::g_system->TLS_free (tls_idx_);
}

static CORBA::Internal::SFloatTLS sfloat_tls;

SFloatContext::SFloatContext () :
	exception_flags_ (0)
{
	Nirvana::g_system->TLS_set (sfloat_tls.tls_idx (), &exception_flags_);
}

SFloatContext::~SFloatContext ()
{
	Nirvana::g_system->TLS_set (sfloat_tls.tls_idx (), nullptr);
}

}
}

extern "C" uint_fast8_t * _softfloat_exceptionFlags ()
{
	return (uint_fast8_t*)Nirvana::g_system->TLS_get (CORBA::Internal::sfloat_tls.tls_idx ());
}
