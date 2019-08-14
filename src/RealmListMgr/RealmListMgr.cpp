/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "RealmListMgr.h"
#include "RegSocket.h"
#include "Network/Listener.hpp"
#include <memory>

using namespace RealmList2;

RealmListMgr::RealmListMgr() : m_acceptConnection(false)
{

}

RealmListMgr::~RealmListMgr()
{

}

void RealmListMgr::StartServer()
{
    if (m_regListener != nullptr)
        return;

    m_regListener.reset(new MaNGOS::Listener<RegistrationSocket>("0.0.0.0", 3444, 1));
}

void RealmListMgr::StopServer()
{
    if (m_regListener == nullptr)
        return;

    m_regListener.reset(nullptr);
}

bool RealmListMgr::AddRealm(RealmDataUPtr rData)
{
    auto it = m_realms.find(rData->Id);
    if (it != m_realms.end())
    {
        DEBUG_LOG("RealmListMgr::AddRealm> Trying to add an already existing server ID(%u)", rData->Id);
        return false;
    }

    m_realms.emplace(rData->Id, std::move(rData));



    return true;
}
