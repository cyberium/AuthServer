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
#ifndef _REALMLIST_MANAGER_H
#define _REALMLIST_MANAGER_H

#include "Common.h"
#include "RegSocket.h"
#include "RealmListDef.h"
#include "Network/Listener.hpp"

namespace RealmList2
{
    class RealmListMgr
    {
    public:
        static RealmListMgr& Instance()
        {
            static RealmListMgr realmListMgr;
            return realmListMgr;
        }

        RealmListMgr();
        ~RealmListMgr();

        void StartServer();
        void StopServer();
        bool AddRealm(RealmDataUPtr rData);

        RealmMap::const_iterator begin() const { return m_realms.begin(); }
        RealmMap::const_iterator end() const { return m_realms.end(); }
        uint32 size() const { return m_realms.size(); }

    private:
        RealmMap m_realms;
        bool m_acceptConnection;
        std::unique_ptr<MaNGOS::Listener<RegistrationSocket>> m_regListener;
    };
}

#define sRealmListMgr RealmList2::RealmListMgr::Instance()

#endif