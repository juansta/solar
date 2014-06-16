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
    Inverter(bool output = false);
    ~Inverter();

public:
    class dataMsg
    {
    public:
        dataMsg()
            : timeStamp(QDateTime::currentDateTime())
        {}

        // result time stamp
        QDateTime timeStamp;

        // internal heat sink temperature
        float temperature;

        // PV array data
        float panel1V;
        float panel1I;
        float panel1P;

        // grid output data
        float gridI;
        float gridV;
        float gridF;
        float gridP;

        // energy generated - today only
        float energy;
    } ;

signals:
    void newData(Inverter::dataMsg data);
    void newDay();
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
    static const int TCP_PORT         = 1200;
    static const int UDP_PORT         = 1300;

    static const int CONNECTION_TIME  = 1000;
    static const int DATA_TIME        = 1000;

    static const int READ_BUFFER_SIZE = 1024;

    static const int MSG_DETAIL = 0;
    static const int MSG_DATA   = 1;
    static const int MSG_TOTAL  = 2;

    static const unsigned char MSGS[MSG_TOTAL][9];
    static const unsigned char GET_INVERTER[];

    const bool m_stdout;
    int m_cycle;
    const QString m_serial;
    QTcpServer * m_server;
    QTcpSocket * m_socket;
    QUdpSocket * m_udpsocket;

    QTimer m_connectTimer;
    QTimer m_dataTimer;
};

#endif // INVERTER_H
