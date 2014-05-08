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

#ifndef STATS_H
#define STATS_H

#include <curl/curl.h>
#include "inverter.h"

class Stats
        : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief class used to smooth out incoming sampels and upload data to pvoutput
     * @param intervalLength Length (in minutes) to upload data
     */
    Stats(QString sysId, QString apiKey, int intervalLength = 5);
    ~Stats();

signals:
    void newStats();

public slots:
    // timer function used to find inverters using UDP broadcast messages
    void doNewData(Inverter::dataMsg data);

private:
    // exponential averaging coefficients
    // this smooths out the incoming 1 second data
    static const float ALPHA = 0.15f;
    static const float BETA  = 0.85f;

    float     m_energy;      // today's kWh
    float     m_arrayV;      // array voltage
    float     m_gridP;       // W supplied to grid
    float     m_temperature; // inverter temperature

    const int m_intervalLen;
    QDateTime m_nextInterval;

    // actual http poster
    CURL * m_curl;
    // custom http headers
    struct curl_slist * m_chunk;
};

#endif // INVERTER_H
