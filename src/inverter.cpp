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

#include "inverter.h"
#include <QDebug>
#include <stdio.h>

// initial UDP message that inverters respond to
// this is broadcast to 255.255.255.255
// this is the "I AM SERVER"
const unsigned char Inverter::GET_INVERTER[] = {0x55, 0xAA, 0x00, 0x40, 0x02, 0x00, 0x0B, 0x49, 0x20, 0x41, 0x4D, 0x20, 0x53, 0x45, 0x52, 0x56, 0x45, 0x52, 0x04, 0x3A};

// inverters then connect to our running server - which is this code

// messages sent from server to inverter
const unsigned char Inverter::GET_DETAIL[] = {0x55, 0xAA, 0x01, 0x03, 0x02, 0x00, 0x00, 0x01, 0x05};
const unsigned char Inverter::GET_DATA1 [] = {0x55, 0xAA, 0x01, 0x00, 0x02, 0x00, 0x00, 0x01, 0x02};
const unsigned char Inverter::GET_DATA2 [] = {0x55, 0xAA, 0x01, 0x09, 0x02, 0x00, 0x00, 0x01, 0x0B};
const unsigned char Inverter::GET_DATA3 [] = {0x55, 0xAA, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04};

const unsigned char Inverter::MSGS[Inverter::MSG_TOTAL][9] =
{
    {0x55, 0xAA, 0x01, 0x03, 0x02, 0x00, 0x00, 0x01, 0x05},
    {0x55, 0xAA, 0x01, 0x00, 0x02, 0x00, 0x00, 0x01, 0x02},
    {0x55, 0xAA, 0x01, 0x09, 0x02, 0x00, 0x00, 0x01, 0x0B},
    {0x55, 0xAA, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04},
};

Inverter::Inverter()
{
    // setup our tcp server
    m_server = new QTcpServer(this);
    m_udpsocket = new QUdpSocket(this);

    connect(m_server, SIGNAL(newConnection()),this, SLOT(newClient()));
    m_server->listen(QHostAddress::Any, 1200);

    connect(&m_connectTimer, SIGNAL(timeout()), this, SLOT(doConnect()));
    m_connectTimer.start(2000);

}
Inverter::~Inverter()
{
    m_socket->close();

}

void Inverter::doConnect()
{
    // go get some inverters
    m_udpsocket->writeDatagram((const char *)GET_INVERTER, 20, QHostAddress::Broadcast, 1300);
}

void Inverter::newClient()
{
    qDebug() << "new inverter connected";
    m_connectTimer.stop();
    m_socket = m_server->nextPendingConnection();

    if (m_socket)
    {
        m_socket->setReadBufferSize(1024);

        connect(m_socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
        connect(m_socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

        connect(&m_dataTimer, SIGNAL(timeout()), this, SLOT(doData()));

        m_dataTimer.start(2000);
    }
}

void Inverter::doData()
{
    static int cycle = 0;

    m_socket->write((const char*)MSGS[cycle], 9);

    //cycle = (cycle + 1) % MSG_TOTAL;
    cycle = 3;
}

void Inverter::disconnected()
{
    qDebug() << "TCP disconnected...";
    m_connectTimer.start(5000);
}
typedef struct
{
    short temperature;
    short panel1V;
    short panel1I;
    short panel1P;
    int   workHrs;
    short energy;
    short gridI;
    short gridV;
    short gridF;
    short outputP;
    int   energyTotal;
} dataMsg;

void Inverter::readyRead()
{
    qDebug() << "\r\nTCP reading...";

    // read the data from the socket
    QByteArray data = m_socket->readAll();

    if (data.length())
    {
        const char * outData = data.data();
        // check what message is being returned
        if (data[2] == 0x01 && data[3] == 0x83)
        {
            // response to general detail msg
            qDebug() << "version"  << &outData[10];
            qDebug() << "model"    << &outData[25];
            qDebug() << "desc"     << &outData[35];
            qDebug() << "serial"   << &outData[51];
            qDebug() << "software" << &outData[67];
        }
        else
        {
           dataMsg dataMsgPtr;

            dataMsgPtr.temperature = ((short)outData[ 7] << 8 & 0xff00) | (outData[ 8] & 0x00ff);
            dataMsgPtr.panel1V     = ((short)outData[ 9] << 8 & 0xff00) | (outData[10] & 0x00ff);
            dataMsgPtr.panel1I     = ((short)outData[13] << 8 & 0xff00) | (outData[14] & 0x00ff);
            dataMsgPtr.panel1P     = ((short)outData[17] << 8 & 0xff00) | (outData[18] & 0x00ff);

            dataMsgPtr.gridI     = ((short)outData[49] << 8 & 0xff00) | (outData[50] & 0x00ff);
            dataMsgPtr.gridV     = ((short)outData[51] << 8 & 0xff00) | (outData[52] & 0x00ff);
            dataMsgPtr.gridF     = ((short)outData[53] << 8 & 0xff00) | (outData[54] & 0x00ff);

            dataMsgPtr.energy     = ((short)outData[23] << 8 & 0xff00) | (outData[24] & 0x00ff);

            qDebug() << "temperature" << (float)dataMsgPtr.temperature / 10.0f;

            qDebug() << "Panel V" << (float)dataMsgPtr.panel1V / 10.0f;
            qDebug() << "Panel I" << (float)dataMsgPtr.panel1I / 10.0f;
            qDebug() << "Panel P" << (float)dataMsgPtr.panel1P / 10.0f;

            qDebug() << "Grid I" << (float)dataMsgPtr.gridI / 10.0f;
            qDebug() << "Grid V" << (float)dataMsgPtr.gridV / 10.0f;
            qDebug() << "Grid F" << (float)dataMsgPtr.gridF / 100.0f;

            qDebug() << "Energy Today" << (float)dataMsgPtr.energy / 100.0f;

        }
    }
}
