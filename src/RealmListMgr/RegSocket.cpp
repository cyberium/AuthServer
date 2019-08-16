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
    MaNGOS::Socket(service, std::move(closeHandler)), m_status(CONNECTION_STATUS_NOT_REGISTERED), m_realmID(0),
    m_deadlineTimer(service), m_heartbeatTimer(service), m_heartbeatCommand(uint8(SRR_HEARTBEAT_COMMAND), 0)
{
}

RegistrationSocket::~RegistrationSocket()
{
}

void RegistrationSocket::StartHeartbeat()
{
    if (IsClosed())
        return;

    // Start an asynchronous operation to send a heartbeat message.
    boost::asio::async_write(GetAsioSocket(), boost::asio::buffer(&m_heartbeatCommand, sizeof(m_heartbeatCommand)),
        boost::bind(&RegistrationSocket::HandleHeartbeatWrite, shared<RegistrationSocket>(), _1));
}

void RegistrationSocket::HandleHeartbeatWrite(const boost::system::error_code& ec)
{
    if (IsClosed())
        return;

    if (!ec)
    {
        DEBUG_LOG("Heartbeat sent... ");

        // Wait 10 seconds before sending the next heartbeat.
        m_heartbeatTimer.expires_from_now(boost::posix_time::seconds(HEARTBEAT_INTERVAL));
        m_heartbeatTimer.async_wait(boost::bind(&RegistrationSocket::StartHeartbeat, shared<RegistrationSocket>()));
    }
    else
    {
        std::cout << "Error on Heartbeat: " << ec.message() << "\n";

        Close();
    }
}

void RegistrationSocket::CheckDeadline()
{
    if (IsClosed())
        return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (m_deadlineTimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are canceled.
        DEBUG_LOG("RegistrationSocket::CheckDeadline> No hearbeat since %usec from server id(%u), closing connection!", uint32(DEADLINE_RESPONSE_TIME), m_realmID);
        Close();

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is set.
        m_deadlineTimer.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    m_deadlineTimer.async_wait(boost::bind(&RegistrationSocket::CheckDeadline, shared<RegistrationSocket>()));
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
    if (ReadLengthRemaining() >= sizeof(PacketHeader))
    {
        char buffer;
        Read(&buffer, 1);
        const ServerRegistrationCommands cmd = static_cast<ServerRegistrationCommands>(buffer);
        switch (cmd)
        {
            case SRC_REGISTERING_REQUEST:
                result = _HandleRegisteringRequest();
                break;

            case SRC_HEARTBEAT_COMMAND:
                DEBUG_LOG("Heartbeat received... ");
                ReadSkip(ReadLengthRemaining());
                break;

            case SRC_USER_CONFIRMATION_REQUEST:
            case SRC_SECURITY_LEVEL_UPDATE:
            case SRC_POPULATION_UPDATE:
            case SRC_STATUS_UPDATE:
            {
                ReadSkip(ReadLengthRemaining());
                DEBUG_LOG("Not Handled yet");
                break;
            }
            default:
            {
                ReadSkip(ReadLengthRemaining());
                DEBUG_LOG("Bad command!");
                break;
            }
        }
    }
    else
    {
        DEBUG_LOG("RegistrationSocket::ProcessIncomingData> recived mal formed packet!");
        ReadSkip(ReadLengthRemaining());
    }
    m_deadlineTimer.expires_from_now(boost::posix_time::seconds(DEADLINE_RESPONSE_TIME));
    return result;
}

bool RegistrationSocket::Open()
{
    if (!Socket::Open())
        return false;

    DEBUG_LOG("RegistrationSocket> Incoming connection from %s.", m_address.c_str());

    StartHeartbeat();

    m_deadlineTimer.expires_from_now(boost::posix_time::seconds(DEADLINE_RESPONSE_TIME));
    CheckDeadline();

    return true;
}

void RegistrationSocket::Close()
{
    m_deadlineTimer.cancel();
    m_heartbeatTimer.cancel();
    Socket::Close();
    sRealmListMgr.SetRealmOnlineStatus(m_realmID, false);
    DEBUG_LOG("RegistrationSocket> Connection was closed.");
}

bool RegistrationSocket::_HandleRegisteringRequest()
{
    bool added = false;
    if (m_status != CONNECTION_STATUS_NOT_REGISTERED)
    {
        sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Server %u sent registration request while already registered!",
            m_realmID);
    }
    else
    {
        int32 payLoadLength = GetPayLoadLength();
        if (payLoadLength <= 0)
        {
            sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Received mal formed packet!");
        }
        else
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

                regData->ServerSocket = shared<MaNGOS::Socket>();

                sRealmListMgr.AddRealm(std::move(regData));

                added = true;
            }
            catch (const pt::ptree_error& e)
            {
                sLog.outError("RegistrationSocket::_HandleRegisteringRequest> %s", e.what());
            }
            catch (...)
            {
                sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Unable to parse JSON!");
            }
        }
    }

    SendRegisteringResponse(added);

    if (ReadLengthRemaining() > 0)
    {
        sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Packet received contain unprocessed datas!");
        ReadSkip(ReadLengthRemaining());
    }

    return true;
}

void RegistrationSocket::Send(PacketHeader const& header, char pData[])
{
    Write((char const*)&header, sizeof(header), pData, header.packetSize);
    m_heartbeatTimer.expires_from_now(boost::posix_time::seconds(HEARTBEAT_INTERVAL));
}

// Response
void RegistrationSocket::SendRegisteringResponse(bool added)
{
    uint8 response = added ? 1 : 0;
    PacketHeader header(SRR_REGISTRATION_RESPONSE, sizeof(uint8));
    Send(header, (char*)& response);
}
