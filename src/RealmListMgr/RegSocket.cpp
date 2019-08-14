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

#include "RegSocket.h"
#include "Log/Log.h"
#include "Utilities/ByteConverter.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include "RealmListMgr.h"

using namespace RealmList2;
namespace pt = boost::property_tree;

RegistrationSocket::RegistrationSocket(boost::asio::io_service& service, std::function<void(Socket*)> closeHandler) :
    MaNGOS::Socket(service, std::move(closeHandler)), m_status(CONNECTION_STATUS_NOT_REGISTERED)
{

}

RegistrationSocket::~RegistrationSocket()
{
    DEBUG_LOG("RegistrationSocket> Connection was closed.");
}

int32 RegistrationSocket::GetPayLoadLength()
{
    char buffer[2];
    Read(&buffer[0], sizeof(buffer));
    uint16* pktSize = static_cast<uint16*>(static_cast<void*>(&buffer[0]));
    EndianConvert(*pktSize);

    if (ReadLengthRemaining() < *pktSize)
        return -1;

    return int32(*pktSize);
}

bool RegistrationSocket::ProcessIncomingData()
{
    DEBUG_LOG("RegistrationSocket::ProcessIncomingData");

    bool result = true;
    PacketHeader header;
    if (ReadLengthRemaining() > sizeof(PacketHeader))
    {
        char buffer;
        Read(&buffer, 1);
        const ServerRegistrationCommands cmd = static_cast<ServerRegistrationCommands>(buffer);
        switch (cmd)
        {
            case SRC_REGISTERING_REQUEST:
                result = _HandleRegisteringRequest();
                break;

            case SRC_USER_CONFIRMATION_REQUEST:
            case SRC_SECURITY_LEVEL_UPDATE:
            case SRC_POPULATION_UPDATE:
            case SRC_STATUS_UPDATE:
            {
                ReadSkip(ReadLengthRemaining());
                Send("Not Handled yet");
                break;
            }
            default:
            {
                ReadSkip(ReadLengthRemaining());
                Send("Bad command!");
                break;
            }
        }
    }

    return result;
}

bool RegistrationSocket::HandleInput()
{
    DEBUG_LOG("RegistrationSocket::HandleInput> received %s", m_input.c_str());
    Send("Ready>");

    m_input.clear();

    return true;
}

void RegistrationSocket::Send(const std::string& message)
{
    Write(message.c_str(), message.length());
}

bool RegistrationSocket::Open()
{
    if (!Socket::Open())
        return false;

    DEBUG_LOG("RegistrationSocket> Incoming connection from %s.", m_address.c_str());

    Send("hi");

    return true;
}

bool RealmList2::RegistrationSocket::_HandleRegisteringRequest()
{
    if (m_status != CONNECTION_STATUS_NOT_REGISTERED)
    {
        sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Server %u sent registration request while already registered!",
            m_realmID);
        return false;
    }

    int32 payLoadLength = GetPayLoadLength();
    if (payLoadLength < 0)
    {
        sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Received mal formed packet!");
        return false;
    }

    if (payLoadLength > 0)
    {
        std::unique_ptr<char> buffer = std::unique_ptr<char>(new char[payLoadLength]);
        Read(buffer.get(), payLoadLength);
        imemstream sBuff(buffer.get(), buffer.get() + payLoadLength);
        pt::ptree receivedJSON;
        try
        {
            pt::read_json(sBuff, receivedJSON);

            if (sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG))
            {
                pt::write_json(std::cout, receivedJSON);
            }

            std::unique_ptr<RealmData> regData = std::unique_ptr<RealmData>(new RealmData());

            regData->Name = receivedJSON.get<std::string>("Name");
            regData->Id = receivedJSON.get<uint32>("Id");
            regData->Type = static_cast<RealmType>(receivedJSON.get<uint32>("Type"));
            regData->Flags = receivedJSON.get<uint32>("Flags");
            regData->TimeZone = static_cast<RealmZone>(receivedJSON.get<uint32>("TimeZone"));
            regData->AllowedSecLevel = static_cast<AccountTypes>(receivedJSON.get<uint32>("AllowedSecLevel"));
            regData->PopulationLevel = receivedJSON.get<float>("PopulationLevel");

            for (const auto& buildItr : receivedJSON.get_child("AcceptedBuilds"))
                regData->AcceptedBuilds.insert(buildItr.second.get_value<uint32>());

            m_realmID = regData->Id;

            sRealmListMgr.AddRealm(std::move(regData));
        }
        catch (const pt::ptree_error& e)
        {
            sLog.outError("RegistrationSocket::_HandleRegisteringRequest> %s", e.what());
            return false;
        }
        catch (...)
        {
            sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Unable to parse JSON!");
            return false;
        }
    }

    Send("Need Implementation!");

    if (ReadLengthRemaining() > 0)
    {
        sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Packet received contain unprocessed datas!");
        ReadSkip(ReadLengthRemaining());
    }

    return true;
}
