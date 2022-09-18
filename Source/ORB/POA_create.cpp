/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include "POA_Root.h"
#include "POA_Activator.h"
#include "POA_Implicit.h"
#include "POA_Persistent.h"
#include "POA_Retain.h"
#include "POA_System.h"
#include "POA_Transient.h"
#include "POA_Unique.h"

using namespace CORBA;

namespace PortableServer {
namespace Core {

template <class ... Bases>
class POA_Impl :
	public Bases...,
	public virtual POA_Base
{
public:
	POA_Impl (POA_Base * parent, const IDL::String * name,
		servant_reference <POAManager> && manager) :
		POA_Base (parent, name, std::move (manager))
	{}
};

template <class ... Bases>
POA_Ref POA_create (POA_Base* parent, const IDL::String* name,
	servant_reference <POAManager>&& manager, const CORBA::PolicyList&)
{
	return make_reference <POA_Impl <Bases...> > (parent, name, std::move (manager));
}

struct POA_Policies
{
	ThreadPolicyValue thread;
	LifespanPolicyValue lifespan;
	IdUniquenessPolicyValue id_uniqueness;
	IdAssignmentPolicyValue id_assignment;
	ImplicitActivationPolicyValue implicit_activation;
	ServantRetentionPolicyValue servant_retention;
	RequestProcessingPolicyValue request_processing;

	POA_Policies () :
		thread (ThreadPolicyValue::ORB_CTRL_MODEL),
		lifespan (LifespanPolicyValue::TRANSIENT),
		id_uniqueness (IdUniquenessPolicyValue::MULTIPLE_ID),
		id_assignment (IdAssignmentPolicyValue::SYSTEM_ID),
		implicit_activation (ImplicitActivationPolicyValue::NO_IMPLICIT_ACTIVATION),
		servant_retention (ServantRetentionPolicyValue::RETAIN),
		request_processing (RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY)
	{}

	void set_values (const CORBA::PolicyList& policies)
	{
		for (auto it = policies.begin (); it != policies.end (); ++it) {
			CORBA::Policy::_ptr_type policy = *it;
			CORBA::PolicyType type = policy->policy_type ();
			switch (type) {
				case THREAD_POLICY_ID:
					thread = ThreadPolicy::_narrow (policy)->value ();
					break;
				case LIFESPAN_POLICY_ID:
					lifespan = LifespanPolicy::_narrow (policy)->value ();
					break;
				case ID_UNIQUENESS_POLICY_ID:
					id_uniqueness = IdUniquenessPolicy::_narrow (policy)->value ();
					break;
				case ID_ASSIGNMENT_POLICY_ID:
					id_assignment = IdAssignmentPolicy::_narrow (policy)->value ();
					break;
				case IMPLICIT_ACTIVATION_POLICY_ID:
					implicit_activation = ImplicitActivationPolicy::_narrow (policy)->value ();
					break;
				case SERVANT_RETENTION_POLICY_ID:
					servant_retention = ServantRetentionPolicy::_narrow (policy)->value ();
					break;
				case REQUEST_PROCESSING_POLICY_ID:
					request_processing = RequestProcessingPolicy::_narrow (policy)->value ();
					break;
			}
		}
	}
};

inline
POA::_ref_type POA_Base::create_POA (const IDL::String& adapter_name,
	PortableServer::POAManager::_ptr_type a_POAManager, const CORBA::PolicyList& policies)
{
	auto ins = children_.emplace (adapter_name, POA_Ref ());
	if (!ins.second)
		throw AdapterAlreadyExists ();
	try {
		POA_Policies pol;
		pol.set_values (policies);
		if (pol.thread != ThreadPolicyValue::ORB_CTRL_MODEL)
			throw InvalidPolicy ();

		servant_reference <POAManager> manager = POAManager::get_implementation (CORBA::Core::local2proxy (a_POAManager));
		if (!manager)
			manager = root_->manager_factory ().create_auto (adapter_name);

		POA_Ref ref;
		if (LifespanPolicyValue::TRANSIENT == pol.lifespan) {
			if (IdUniquenessPolicyValue::UNIQUE_ID == pol.id_uniqueness) {
				if (IdAssignmentPolicyValue::USER_ID == pol.id_assignment) {
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation)
						; // SYSTEM_ID is required
					else {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_Transient, POA_Unique> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_Transient, POA_Unique, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					}
				} else {
					// SYSTEM_ID
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation) {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_ImplicitUnique> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_System, POA_Unique, POA_Activator, POA_Implicit> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					} else {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_System, POA_Unique> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_System, POA_Unique, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					}
				}
			} else {
				// MULTIPLE_ID
				if (IdAssignmentPolicyValue::USER_ID == pol.id_assignment) {
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation)
						; // SYSTEM_ID is required
					else {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_Transient, POA_Retain> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_Transient, POA_Retain, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					}
				} else {
					// SYSTEM_ID
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation) {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_System, POA_Retain, POA_Implicit> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_System, POA_Retain, POA_Activator, POA_Implicit> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					} else {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_System, POA_Retain> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_System, POA_Retain, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					}
				}
			}
		} else {
			// PERSISTENT
			if (IdUniquenessPolicyValue::UNIQUE_ID == pol.id_uniqueness) {
				if (IdAssignmentPolicyValue::USER_ID == pol.id_assignment) {
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation)
						; // SYSTEM_ID is required
					else {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_Persistent, POA_Unique> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_Persistent, POA_Unique, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					}
				} else {
					// SYSTEM_ID
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation) {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_SystemPersistent, POA_Unique, POA_Implicit> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_SystemPersistent, POA_Unique, POA_Activator, POA_Implicit> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} // RETAIN is required for UNIQUE_ID
					} else {
						// NO_IMPLICIT_ACTIVATION
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_SystemPersistent, POA_Unique> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									throw InvalidPolicy (); // Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_SystemPersistent, POA_Unique, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} else
							throw InvalidPolicy (); // RETAIN is required for UNIQUE_ID
					}
				}
			} else {
				// MULTIPLE_ID
				if (IdAssignmentPolicyValue::USER_ID == pol.id_assignment) {
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation)
						; // SYSTEM_ID is required
					else {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_Persistent, POA_Retain> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_Persistent, POA_Retain, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} else {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported: default
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									// Not yet supported: locator
									break;
							}
						}
					}
				} else {
					// SYSTEM_ID
					if (ImplicitActivationPolicyValue::IMPLICIT_ACTIVATION == pol.implicit_activation) {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_SystemPersistent, POA_Retain, POA_Implicit> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_SystemPersistent, POA_Retain, POA_Activator, POA_Implicit> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} else {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported: default
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									// Not yet supported: locator
									break;
							}
						}
					} else {
						if (ServantRetentionPolicyValue::RETAIN == pol.servant_retention) {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_ACTIVE_OBJECT_MAP_ONLY:
									ref = POA_create <POA_SystemPersistent, POA_Retain> (this, &ins.first->first, std::move (manager), policies);
									break;
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									ref = POA_create <POA_SystemPersistent, POA_Retain, POA_Activator> (this, &ins.first->first, std::move (manager), policies);
									break;
							}
						} else {
							switch (pol.request_processing) {
								case RequestProcessingPolicyValue::USE_DEFAULT_SERVANT:
									// Not yet supported: default
									break;
								case RequestProcessingPolicyValue::USE_SERVANT_MANAGER:
									// Not yet supported: locator
									break;
							}
						}
					}
				}
			}
		}

		if (ref)
			ins.first->second = std::move (ref);
		else
			throw InvalidPolicy ();
	} catch (...) {
		children_.erase (ins.first);
		throw;
	}
	
	return ins.first->second->_this ();
}

PortableServer::POA::_ref_type POA_Root::create ()
{
	auto manager_factory = CORBA::make_reference <POAManagerFactory> ();
	auto manager = manager_factory->create ("RootPOAManager", CORBA::PolicyList ());
	return CORBA::make_reference <POA_Root> (std::move (manager), std::move (manager_factory))->_this ();
}

}
}
