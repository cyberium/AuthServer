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

#ifndef _SERVER_COMMUNICATION_SOCKET_H
#define _SERVER_COMMUNICATION_SOCKET_H

#include "Network/Socket.hpp"
#include "SrvComOpcodes.h"
#include "RealmListDef.h"
#include <boost/asio/deadline_timer.hpp>
#include <memory>

namespace RealmList2
{

// consider the server connection have to be closed if nothing received after deadline response time (in seconds)
#define DEADLINE_RESPONSE_TIME 30
#define HEARTBEAT_INTERVAL 10

    enum SocketStatus
    {
        CONNECTION_STATUS_NOT_REGISTERED = 0,
        CONNECTION_STATUS_REGISTERED = 1
    };

#pragma pack(push,1)
    struct PacketHeader
    {
        PacketHeader(uint8 cmd, uint16 size) : command(cmd), packetSize(size) {}

        uint8 command;
        uint16 packetSize;
    };
#pragma pack(pop)

    class ServerComPacket;

    typedef std::unique_ptr<ServerComPacket> ServerComPacketUPtr;

    struct membuf : std::streambuf
    {
        membuf(char* begin, char* end) { this->setg(begin, begin, end); }
    };

    class imemstream : private virtual membuf, public std::istream
    {
    public:
        imemstream(char* begin, char* end) : membuf(begin, end), std::istream(static_cast<std::streambuf*>(this)) {}
    };

    class SrvComSocket : public MaNGOS::Socket
    {
    private:
        virtual bool ProcessIncomingData() override;
        void Send(PacketHeader const& header, char pData[]);
        int32 GetPayLoadLength();

        void StartHeartbeat();
        void HandleHeartbeatWrite(const boost::system::error_code& ec);
        void CheckDeadline();


        void SendRegisteringResponse(bool added);
        
        SocketStatus m_status;
        std::string m_input;
        uint32 m_realmID;
        PacketHeader m_heartbeatCommand;

        boost::asio::deadline_timer m_deadlineTimer;
        boost::asio::deadline_timer m_heartbeatTimer;

    public:
        SrvComSocket(boost::asio::io_service& service, std::function<void(Socket*)> closeHandler);
        virtual ~SrvComSocket();
        virtual bool Open() override;
        virtual void Close() override;


        // all handlers
        bool _HandleNullMsg(ServerComPacketUPtr pkt);
        bool _HandleRegisteringRequest(ServerComPacketUPtr pkt);
        bool _HandleHeartbeat(ServerComPacketUPtr pkt);
        //bool _HandleUserConfirmationRequest(ServerComPacket& pkt);
        //bool _HandleSecurityLevelUpdate(ServerComPacket& pkt);
        //bool _HandlePopulationUpdate(ServerComPacket& pkt);
        bool _HandleStatusUpdate(ServerComPacketUPtr pkt);
    };

    typedef struct SrvOpcodeHandler
    {
        const char* name;
        SrvComSecReq access;
        bool (SrvComSocket::* handler)(ServerComPacketUPtr);
    } SrvOpcodeHandler;

    // Warning :: should be strictly incremental(+1) values corresponding to the SrvRegOpcodes enum
    static const SrvOpcodeHandler SrvComOpcodesTable[] =
    {
        {"MSG_NULL_ACTION"                  , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleNullMsg                },
        {"SMSG_REGISTERING_REQUEST"         , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleRegisteringRequest     },
        {"SMSG_USER_CONFIRMATION_REQUEST"   , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleNullMsg                },
        {"SMSG_SECURITY_LEVEL_UPDATE"       , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleNullMsg                },
        {"SMSG_POPULATION_UPDATE"           , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleNullMsg                },
        {"SMSG_STATUS_UPDATE"               , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleStatusUpdate           },

        {"RMSG_REGISTRATION_RESPONSE"       , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleNullMsg                },
        {"RMSG_USER_CONFIRMATION_RESPONSE"  , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleNullMsg                },
        {"RMSG_SECURITY_LEVEL_RESPONSE"     , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleNullMsg                },

        {"MSG_HEARTBEAT_COMMAND"            , SRV_COM_SECURITY_FREE     , &SrvComSocket::_HandleHeartbeat              },
    };

    inline char const* LookupSrvComOpcodeName(SrvComOpcodes opcode) { assert(opcode <= MSG_HEARTBEAT_COMMAND); return SrvComOpcodesTable[opcode].name; }

    class ServerComPacket : public ByteBuffer
    {
    public:     
        // just container for later use
        ServerComPacket() : ByteBuffer(0), m_opcode(MSG_NULL_ACTION)
        {
        }
        explicit ServerComPacket(SrvComOpcodes opcode, size_t res = 200) : ByteBuffer(res), m_opcode(opcode) { }
        // copy constructor
        ServerComPacket(const ServerComPacket& packet) : ByteBuffer(packet), m_opcode(packet.m_opcode)
        {
        }

        void Initialize(SrvComOpcodes opcode, size_t newres = 200)
        {
            clear();
            _storage.reserve(newres);
            m_opcode = opcode;
        }

        SrvComOpcodes GetOpcode() const { return m_opcode; }
        void SetOpcode(SrvComOpcodes opcode) { m_opcode = opcode; }
        inline const char* GetOpcodeName() const { return LookupSrvComOpcodeName(m_opcode); }

    protected:
        SrvComOpcodes m_opcode;
    };
}

#endif
