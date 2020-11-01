// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include <Nirvana/Nirvana.h>
#include <Port/config.h>

namespace Nirvana {
namespace Core {
namespace Port {

NIRVANA_NORETURN void _unrecoverable_error (const char* file, unsigned line);

}
}
}

#define unrecoverable_error() ::Nirvana::Core::Port::_unrecoverable_error (__FILE__, __LINE__)

#endif
