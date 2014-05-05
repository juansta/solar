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

#ifndef INVERTER_H
#define INVERTER_H

#include <QDateTime>
#include <QTcpServer>       // allow inverter to connect here
#include <QTcpSocket>       // socket used after server accept
#include <QUdpSocket>       // udp socket used to discover inverters
#include <QAbstractSocket>

#include <QTimer>

class Inverter
        : public QObject
{
    Q_OBJECT

public:
    Inverter();
    ~Inverter();

public:
    class Data
    {
        QDateTime time;

        float   temperature;

        float accPower;
        float outPower;

        float   acVolt   ;
        float   acCurrent;
        float   acPower  ;

        float   pvVolt   [3];
        float   pvCurrent[3];
        float   pvPower  [3];
    };

signals:
    void newData(QString serial, Data data);

public slots:
    // timer function used to find inverters using UDP broadcast messages
    void doConnect();

    // timer function used to poll connected inverters using TCP messaging
    void doData();

    // after connection established from new inverter
    void newClient();

    // tcp connections from inverters
    void disconnected();
    //void bytesWritten(qint64 bytes);
    void readyRead();

private:
    static const int MSG_DETAIL = 0;
    static const int MSG_DATA1 = 1;
    static const int MSG_DATA2 = 2;
    static const int MSG_DATA3 = 3;
    static const int MSG_TOTAL = 4;

    static const unsigned char GET_INVERTER[];
    static const unsigned char GET_DETAIL[];
    static const unsigned char GET_DATA1[];
    static const unsigned char GET_DATA2[];
    static const unsigned char GET_DATA3[];
    static const unsigned char MSGS[MSG_TOTAL][9];

    const QString m_serial;
    QTcpServer * m_server;
    QTcpSocket * m_socket;
    QUdpSocket * m_udpsocket;

    QTimer m_connectTimer;
    QTimer m_dataTimer;
};

#endif // INVERTER_H
