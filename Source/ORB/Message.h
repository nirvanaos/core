// Platform domain environment specific inter-ORB protocol.
#ifndef NIRVANA_ORB_CORE_MESSAGE_H_
#define NIRVANA_ORB_CORE_MESSAGE_H_

#include "../core.h"

namespace Nirvana {
namespace ESIOP {

struct Message
{
	enum class Type : uint16_t
	{
		REQUEST,
		REPLY,
		USER_EXCEPTION,
		SYSTEM_EXCEPTION,

		RESERVED_MESSAGES = 8
	};

	struct ObjRef
	{
		uintptr_t address;
		Core::ObjRefSignature signature;
	};

	struct ObjRefLocal : ObjRef
	{
		Core::ProtDomainId prot_domain;
	};

	struct Header
	{
		Type type;

		uint16_t flags;
		static const uint16_t IMMEDIATE_DATA = 0x0001;

		ObjRef target;
	};

	struct Request : Header
	{
		ObjRefLocal reply_target; // Zero for oneway requests
		Core::UserToken user;
	};

	struct SystemException : Header
	{
		CORBA::SystemException::Code code;
		CORBA::SystemException::_Data data;
	};

	union Buffer
	{
		Request request;
		SystemException system_exception;
	};
};

}
}

#endif
