/*
* Nirvana package manager.
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
#include "pch.h"
#include "PacMan.h"
#include "factory_s.h"

namespace Nirvana {
namespace PM {

class Static_pacman_factory :
	public CORBA::servant_traits <Nirvana::PM::PacManFactory>::ServantStatic <Static_pacman_factory>
{
public:
	static PacMan::_ref_type create (Nirvana::SysDomain::_ptr_type sys_domain, CosEventChannelAdmin::ProxyPushConsumer::_ptr_type completion)
	{
		return CORBA::make_reference <::PacMan> (sys_domain, completion)->_this ();
	}
};

}
}

NIRVANA_EXPORT_OBJECT (_exp_Nirvana_PM_pacman_factory, Nirvana::PM::Static_pacman_factory)

