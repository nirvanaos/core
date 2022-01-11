#include <CORBA/Server.h>
#include <Nirvana/file_system.h>
#include "FileAccessDirect.h"

namespace Nirvana {

class file_factory :
	public CORBA::servant_traits <FileFactory>::ServantStatic <file_factory>
{
public:
	static FileAccessDirect::_ref_type open (const std::string& path, uint16_t flags)
	{
		return CORBA::make_reference <Core::FileAccessDirect> (std::ref (path), flags)->_this ();
	}
};

}

NIRVANA_STATIC_EXP (Nirvana, file_factory)
