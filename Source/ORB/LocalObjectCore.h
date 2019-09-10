#ifndef NIRVANA_TESTORB_LOCALOBJECTCORE_H_
#define NIRVANA_TESTORB_LOCALOBJECTCORE_H_

#include <CORBA/LocalObject_s.h>
#include "ObjectImpl.h"
#include "ReferenceCounterImpl.h"

namespace Nirvana {
namespace Core {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

class LocalObjectCore :
	public ImplementationPseudo <LocalObjectCore, LocalObject, ReferenceCounter>,
	public ObjectImpl <LocalObjectCore>,
	public ReferenceCounterBase,
	public LifeCycleNoCopy <LocalObjectCore>
{
public:
	LocalObjectCore (AbstractBase_ptr servant, DynamicServant_ptr dynamic) :
		ObjectImpl <LocalObjectCore> (servant),
		ReferenceCounterBase (dynamic)
	{}
};

}
}

#endif