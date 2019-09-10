#ifndef NIRVANA_TESTORB_SERVANTCORE_H_
#define NIRVANA_TESTORB_SERVANTCORE_H_

#include <CORBA/ServantBase_s.h>
#include "ObjectCore.h"
#include "ReferenceCounterImpl.h"

namespace Nirvana {
namespace Core {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

class ServantCore :
	public ImplementationPseudo <ServantCore, PortableServer::ServantBase, ReferenceCounter>,
	public ReferenceCounterBase,
	public LifeCycleNoCopy <ServantCore>
{
public:
	ServantCore (PortableServer::Servant servant, DynamicServant_ptr dynamic) :
		object_ (servant),
		ReferenceCounterBase (dynamic)
	{}

	operator Bridge <Object>& ()
	{
		return object_;
	}

	operator Bridge <AbstractBase>& ()
	{
		return object_;
	}

	PortableServer::POA_ptr _default_POA () const;

	InterfaceDef_ptr _get_interface () const
	{
		return static_cast <const ObjectBase&> (object_)._get_interface ();
	}

	Boolean _is_a (const Char* type_id)
	{
		return static_cast <const ObjectBase&> (object_)._is_a (type_id);
	}

	Boolean _non_existent () const
	{
		return !object_.is_active_;
	}

private:
	ObjectCore object_;
};

}
}

#endif
