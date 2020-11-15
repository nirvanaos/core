// Platform domain environment specific inter-ORB protocol.
#ifndef NIRVANA_ORB_CORE_ESIOP_H_
#define NIRVANA_ORB_CORE_ESIOP_H_

#include "../core.h"

namespace Nirvana {
namespace ESIOP {

#pragma pack(push, 1)

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

	struct Header
	{
		Type type;

		uint16_t flags;
		static const uint16_t IMMEDIATE_DATA = 0x0001;

		uintptr_t target;
	};

	struct Request : Header
	{
		uintptr_t reply_target; // Zero for oneway requests
	};

	struct SystemException : Header
	{
		CORBA::SystemException::Code exception_code;
		CORBA::SystemException::Data exception_data;
	};

	union Buffer
	{
		Request request;
		SystemException system_exception;
	};
};

#pragma pack(pop)

}
}

#endif
