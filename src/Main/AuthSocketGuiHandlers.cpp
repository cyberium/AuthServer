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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "Config/Config.h"
#include "Log/Log.h"
#include "RealmList.h"
#include "AuthSocket.h"
#include "AuthCodes.h"
#include "RealmListMgr.h"

bool AuthSocket::_HandleSetGuidMode()
{
    ReadSkip(ReadLengthRemaining());

    if (_accountSecurityLevel < SEC_ADMINISTRATOR)
    {
        sLog.outError("Account '%s' tried to set GUI mode with insufficient credential!", _login.c_str());

        ByteBuffer pkt;
        pkt << (uint8)MSG_SET_GUI_MODE_RESPONSE;
        pkt << (uint8)WOW_FAIL_FAIL_NOACCESS;
        Write((const char*)pkt.contents(), pkt.size());

        return false;
    }
    m_guiId = sRealmListMgr.AddGuiSocket(shared<AuthSocket>());
    return true;
}