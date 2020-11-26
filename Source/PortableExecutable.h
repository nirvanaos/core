#ifndef NIRVANA_CORE_PORTABLEEXECUTABLE_H_
#define NIRVANA_CORE_PORTABLEEXECUTABLE_H_

#include "COFF.h"
#include "Section.h"

namespace Nirvana {
namespace Core {

class PortableExecutable : public COFF
{
public:
	PortableExecutable (const void* base_address);

	const void* base_address () const NIRVANA_NOEXCEPT
	{
		return base_address_;
	}

	bool find_OLF_section (Core::Section& section) const NIRVANA_NOEXCEPT;

private:
	static const void* get_COFF (const void* base_address);

private:
	const void* base_address_;
};


}
}

#endif
