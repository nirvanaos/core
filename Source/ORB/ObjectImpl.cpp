#include "ObjectImpl.h"

namespace Nirvana {
namespace Core {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

Boolean ObjectBase::_is_a (const Char* type_id) const
{
	Bridge <Interface>* itf = servant_->_query_interface (type_id);
	if (itf)
		return TRUE;
	else
		return FALSE;
}

}
}
