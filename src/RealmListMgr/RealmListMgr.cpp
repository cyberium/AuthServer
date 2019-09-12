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
#include "SrvComSocket.h"
#include "Network/Listener.hpp"
#include "Log/Log.h"
#include <memory>
#include <thread>
#include <chrono>
#include "../Main/AuthCodes.h"

using namespace RealmList2;

RealmListMgr::RealmListMgr() : m_regListener(nullptr), m_guiIdCounter(1)
{
}

RealmListMgr::~RealmListMgr()
{

}

void RealmListMgr::UpdateThread()
{
    while (m_regListener != nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        //TODO:: add GUI clients update
    }
}

void RealmListMgr::StartServer()
{
    if (m_regListener != nullptr)
        return;

    m_regListener.reset(new MaNGOS::Listener<SrvComSocket>("0.0.0.0", 3444, 1));
    m_realmListThread.reset(new std::thread(&RealmListMgr::UpdateThread, this));
}

void RealmListMgr::StopServer()
{
    if (m_regListener == nullptr)
        return;

    m_regListener.reset(nullptr);
    m_realmListThread->join();
}

RealmData const* RealmListMgr::GetRealmData(uint32 realmId)
{
    std::lock_guard<std::mutex> lock(m_realmListMutex);
    auto itr = m_realms.find(realmId);

    if (itr != m_realms.end())
    {
        return itr->second.get();
    }
    return nullptr;
}

void RealmListMgr::SendPacketToAllGUI(ByteBuffer const& pkt)
{
    std::lock_guard<std::mutex> lock(m_guiListMutex);

    for (auto const& guiItr : m_guiSocketMap)
        guiItr.second->Write((const char*)pkt.contents(), pkt.size());
}

void RealmListMgr::SendNewRealm(RealmData const& rData)
{
    // lock critical section
    ByteBuffer newRealmPkt;
    newRealmPkt << (uint8)MSG_GUI_ADD_REALM;
    RealmDataToByteBuffer(rData, newRealmPkt);
    SendPacketToAllGUI(newRealmPkt);
}

void RealmListMgr::SendRealmStatus(RealmData const& rData)
{
    ByteBuffer pkt;
    pkt << (uint8)MSG_GUI_ADD_REALM;
    pkt << (uint32)rData.Id;
    pkt << (uint8)rData.Flags;

    SendPacketToAllGUI(pkt);
}

bool RealmListMgr::AddRealm(RealmDataUPtr rData)
{
    bool result = false;

    // set it online
    SetOnlineStatus(*rData, true);

    {
        std::lock_guard<std::mutex> lock(m_realmListMutex);
        auto it = m_realms.find(rData->Id);
        if (it == m_realms.end())
        {
            // update all GUI clients
            SendNewRealm(*rData);

            // add the realm to realm list
            m_realms.emplace(rData->Id, std::move(rData));
            result = true;
        }
        else
            it->second = std::move(rData);
    }

    return result;
}

void RealmListMgr::RemoveRealm(uint32 realmId)
{
    std::lock_guard<std::mutex> lock(m_realmListMutex);
    m_realms.erase(realmId);
}

void RealmListMgr::SetRealmOnlineStatus(uint32 realmId, bool status)
{
    std::lock_guard<std::mutex> lock(m_realmListMutex);
    auto itr = m_realms.find(realmId);

    if (itr != m_realms.end())
        SetOnlineStatus(*itr->second, status);
}

void RealmListMgr::SetOnlineStatus(RealmData& rData, bool status)
{
    if (status)
        rData.Flags = rData.Flags & ~REALM_FLAG_OFFLINE;
    else
        rData.Flags = rData.Flags | REALM_FLAG_OFFLINE;
}

uint64 RealmListMgr::AddGuiSocket(AuthSocketSPtr skt)
{
    if (skt->GetGuiId() != 0)
    {
        sLog.outError("RealmListMgr::AddGuiSocket> trying to add a second time Gui socket(%s) to gui list!",
            skt->GetRemoteAddress().c_str());
        return 0;
    }

    // lock critical section
    std::lock_guard<std::mutex> lock(m_guiListMutex);

    // for now only one client at a time!
    if (!m_guiSocketMap.empty())
        return 0;

    // get new id
    uint64 newId = m_guiIdCounter++;

    sLog.outString("GUI(id:" UI64FMTD ") from %s and using %s credential successfully connected.", newId,  skt->GetRemoteAddress().c_str(), skt->GetAccountName().c_str());
    // add the realm to realm list
    m_guiSocketMap.emplace(newId, std::move(skt));

    return newId;
}

void RealmListMgr::RemoveGuiSocket(uint64 id)
{
    // lock critical section
    std::lock_guard<std::mutex> lock(m_guiListMutex);

    auto itr = m_guiSocketMap.find(id);
    if (itr != m_guiSocketMap.end())
    {
        sLog.outString("Successfully removed GUI(id: " UI64FMTD ") from %s.", id, itr->second->GetRemoteAddress().c_str());
        m_guiSocketMap.erase(itr);
    }
}

void RealmListMgr::RealmDataToByteBuffer(RealmData const& data, ByteBuffer& pkt) const
{
    pkt << (uint32)data.Id;
    pkt << (std::string)data.Name;
    pkt << (std::string)data.Address;
    pkt << (uint8)data.Flags;
    pkt << (uint8)data.Type;
    pkt << (uint8)data.AllowedSecLevel;
    pkt << (float)data.PopulationLevel;
    pkt << (uint8)data.AcceptedBuilds.size();
    for (auto build : data.AcceptedBuilds)
        pkt << (uint32)build;
}

bool RealmListMgr::GetRealmData(uint32 realmId, ByteBuffer& pkt)
{
    std::lock_guard<std::mutex> lock(m_realmListMutex);
    auto const& itr = m_realms.find(realmId);

    if (itr != m_realms.end())
    {
        RealmData const& data = *itr->second;
        RealmDataToByteBuffer(data, pkt);
        return true;
    }
    return false;
}

void RealmListMgr::GetRealmsList(ByteBuffer& pkt)
{
    std::lock_guard<std::mutex> lock(m_realmListMutex);
    size_t sizePos = (uint32) pkt.wpos();
    pkt << (uint32)1;
    pkt << (uint8) m_realms.size();

    if (m_realms.empty())
        return;

    for (auto const& realmDataItr : m_realms)
    {
        RealmData const& data = *realmDataItr.second;
        RealmDataToByteBuffer(data, pkt);
    }

    uint32 totalPacketSize = static_cast<uint32>(pkt.wpos() - (sizePos + 4));

    pkt.put<uint32>(sizePos, totalPacketSize);
}

void RealmListMgr::UpdateRealmStatus(uint32 realmId, uint8 flags, float populationLevel)
{
    {
        std::lock_guard<std::mutex> lock(m_realmListMutex);

        auto itr = m_realms.find(realmId);
        if (itr == m_realms.end())
            return;

        itr->second->Flags = flags;
        itr->second->PopulationLevel = populationLevel;
    }

    ByteBuffer pkt;
    pkt << (uint8)MSG_GUI_SET_REALM_STATUS;
    pkt << (uint32)realmId;
    pkt << (uint8)flags;
    pkt << (float)populationLevel;
    SendPacketToAllGUI(pkt);
}
