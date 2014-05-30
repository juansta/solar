/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "stats.h"
#include <QDebug>


Stats::Stats(QString sysId, QString apiKey, int intervalLength)
    : m_energy      (0.0f),
      m_arrayV      (0.0f),
      m_gridP       (0.0f),
      m_temperature (0.0f),
      m_intervalLen (intervalLength * 60),
      m_nextInterval(QDateTime::currentDateTime()),
      m_curl        (NULL),
      m_chunk       (NULL),
      m_intervalSet (true)
{
    // setup a new interval time
    int msecAlarm = m_intervalLen * 1000;

    // get the number of milli-seconds to our next roll over
    qint64 msec   = m_nextInterval.currentMSecsSinceEpoch();
    qint64 offset = msecAlarm - ((msec + msecAlarm) % msecAlarm);

    // add the offset
    msec += offset;
    m_nextInterval.setMSecsSinceEpoch(msec);

    // initialise our curl library
    curl_global_init(CURL_GLOBAL_ALL);

    // setup our HTTP headers
    // these never change between subsequent calls
    QString temp;
    temp.append("X-Pvoutput-Apikey:" + apiKey);
    m_chunk = curl_slist_append(m_chunk, temp.toStdString().c_str());

    temp.clear();
    temp.append("X-Pvoutput-SystemId:" + sysId);
    m_chunk = curl_slist_append(m_chunk, temp.toStdString().c_str());
}

Stats::~Stats()
{
    curl_slist_free_all(m_chunk);

    // clean up our curl instance
    curl_global_cleanup();

}
void Stats::flush()
{
    // get a curl handle
    m_curl = curl_easy_init();
    if (m_curl)
    {
        int fails = 0;

        // flush out any stored values
        for (QVector<QString>::iterator iter = m_queue.begin();
             fails < 100 && iter != m_queue.end();)
        {
            CURLcode res;

            // setup our request
            res = curl_easy_setopt(m_curl, CURLOPT_URL, iter->toStdString().c_str());
            if (res != CURLE_OK)
                std::cerr << "curl_easy_setopt(CURLOPT_URL) failed: " << curl_easy_strerror(res) << std::endl;
            else
            {
                res = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_chunk);
                if (res != CURLE_OK)
                    std::cerr << "curl_easy_setopt(CURLOPT_HTTPHEADER) failed: " << curl_easy_strerror(res) << std::endl;
                else
                {
                    // perform the actual request
                    res = curl_easy_perform(m_curl);
                    if (res != CURLE_OK)
                        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                    else
                    {
                        iter = m_queue.erase(iter);
                        std::cout << iter->toStdString().c_str() << std::endl;
                        std::cout << curl_easy_strerror(res) << std::endl;
                    }
                }
            }

            if (res != CURLE_OK)
                fails++;
        }
    }

    curl_easy_cleanup(m_curl);
    m_curl = NULL;
}
void Stats::doNewData(Inverter::dataMsg data)
{
    // smooth out our instantaneous values
    if (data.energy > 0.0f && data.energy < 40000.0f)
        m_energy = m_energy * ALPHA + data.energy  * BETA;
    if (data.panel1V > 0.0f && data.panel1V < 500.0f)
        m_arrayV = m_arrayV * ALPHA + data.panel1V * BETA;
    if (data.gridP > 0.0f && data.gridP < 4000.0f)
        m_gridP  = m_gridP  * ALPHA + data.gridP   * BETA;

    if (data.temperature > 0.0f && data.temperature < 200.0f)
        m_temperature  = m_temperature  * ALPHA + data.temperature   * BETA;

    // check to see if we need to upload to pvoutput
    if (!m_intervalSet)
    {
        // setup a new interval time
        int msecAlarm = m_intervalLen * 1000;

        // get the number of milli-seconds to our next roll over
        qint64 msec   = m_nextInterval.currentMSecsSinceEpoch();
        qint64 offset = msecAlarm - ((msec + msecAlarm) % msecAlarm);

        // add the offset
        msec += offset;
        m_nextInterval.setMSecsSinceEpoch(msec);

        m_intervalSet = true;
    }

    if ((m_intervalLen > 0) && (data.timeStamp > m_nextInterval))
    {
        QString post;

        post.append("http://pvoutput.org/service/r2/addstatus.jsp?d="   + m_nextInterval.date().toString("yyyyMMdd")
                  + "&t="  + m_nextInterval.time().toString("HH:mm")
                  + "&v1=" + QString::number(m_energy, 'f', 0)
                  + "&v2=" + QString::number(m_gridP, 'f', 0)
                  + "&v5=" + QString::number(m_temperature, 'f', 1)
                  + "&v6=" + QString::number(m_arrayV, 'f', 1)
                 );

        m_queue.push_back(post);

        flush();

        // work out when our next interval is going to be
        m_nextInterval = m_nextInterval.addSecs(m_intervalLen);
    }
}
void Stats::doNewDay()
{
    std::cout << "New Day" << std::endl;

    m_energy = 0.0f;
    m_arrayV = 0.0f;
    m_gridP  = 0.0f;
    m_temperature  = 0.0f;

    m_intervalSet = false;
}
