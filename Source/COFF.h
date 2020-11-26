#ifndef NIRVANA_CORE_COFF_H_
#define NIRVANA_CORE_COFF_H_

#include <core.h>

namespace llvm {
namespace COFF {
struct header;
struct section;
}
}

namespace Nirvana {
namespace Core {

class COFF
{
public:
	typedef llvm::COFF::section Section;

	COFF (const void* addr) NIRVANA_NOEXCEPT :
		hdr_ ((const llvm::COFF::header*)addr)
	{}

	const Section* find_section (const char* name) const NIRVANA_NOEXCEPT;

private:
	static bool is_section (const Section& s, const char* name) NIRVANA_NOEXCEPT;

private:
	const llvm::COFF::header* hdr_;
};

}
}

#endif
