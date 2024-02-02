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
