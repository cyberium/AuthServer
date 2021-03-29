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

#ifndef _TIMED_RING_BUFFER
#define _TIMED_RING_BUFFER
#include "Common.h"
#include <mutex>

class TimedRingBuffer
{
public:
    struct TimedItem
    {
        TimedItem(std::string const& elem, uint8 iType, TimePoint const& tp) : timeAdded(tp), item(elem), itemType(iType) {}
        TimePoint timeAdded;
        std::string item;
        uint8 itemType;
    };

    struct LogDatas
    {
        LogDatas(std::string const& _log, uint8 _type) : log(_log), type(_type) {}
        std::string log;
        uint8 type;
    };

    typedef std::vector<LogDatas> LogDatasVec;

    explicit TimedRingBuffer(uint32 size) : m_bufferSize(size), m_maxReached(false), m_first(0), m_last(0)
    {
        // initialize the buffer so no more need of allocation
        m_buffer.reserve(size);
        for (uint32 i = 0; i < size; ++i)
            m_buffer.emplace_back("", 0, TimePoint());
    }

    // add an element
    void Add(const std::string& item, uint8 itemType)
    {
        using namespace std::chrono;

        std::lock_guard<std::mutex> lock(m_mutex);

        // adding item as well as the current time
        auto& slot = m_buffer[m_first];
        slot.item = item;
        slot.itemType = itemType;
        slot.timeAdded = time_point_cast<TimePoint::duration>(TimePoint::clock::now());

        m_first = (m_first + 1) % m_bufferSize;

        if (m_maxReached)
            m_last = (m_last + 1) % m_bufferSize;
        else
            m_maxReached = m_first == m_last;
    }

    // get current count of elements
    uint32 GetCount()
    {
        uint32 result = m_bufferSize;
        if (!m_maxReached)
        {
            if (m_first >= m_last)
                result = m_first - m_last;
            else
                result = result + m_first - m_last;
        }

        return result;
    }

    // get a vector containing all element since provided TimePoint
    void GetAllAfter(LogDatasVec& result, TimePoint const& timePoint)
    {
        result.clear();
        std::lock_guard<std::mutex> lock(m_mutex);
        uint32 elementCount = GetCount();
        if (elementCount == 0)
            return;

        result.reserve(GetCount());

        uint32 count = (m_first - 1) % m_bufferSize;

        for (uint32 i = 1; i <= elementCount; ++i)
        {
            uint32 index = (m_first - i) % m_bufferSize;
            auto& slot = m_buffer[index];
            if (slot.timeAdded > timePoint)
                result.emplace_back(slot.item, slot.itemType);
            else
                break;
        }
    }

    // clear the buffer.
    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_first = 0;
        m_last = 0;
        m_maxReached = false;
    }

private:
    std::mutex m_mutex;
    std::vector<TimedItem> m_buffer;
    uint32 m_bufferSize;
    uint32 m_first;
    uint32 m_last;
    bool m_maxReached;
};

#endif

