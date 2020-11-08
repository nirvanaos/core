#include "Runnable.h"

namespace Nirvana {
namespace Core {

void Runnable::on_exception ()
{
	// TODO: Log
	assert (false);
}

void Runnable::on_crash (Word error_code)
{
	// TODO: Log
	assert (false);
}

}
}
