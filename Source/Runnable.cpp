#include "Runnable.h"

namespace Nirvana {
namespace Core {

void Runnable::on_exception () NIRVANA_NOEXCEPT
{
	// TODO: Log
	assert (false);
}

void Runnable::on_crash (Word error_code) NIRVANA_NOEXCEPT
{
	// TODO: Log
	assert (false);
}

}
}
