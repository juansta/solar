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
#include "inverter.h"
#include <QDebug>
#include <stdio.h>

// initial UDP message that inverters respond to
// this is broadcast to 255.255.255.255
// this is the "I AM SERVER"
const unsigned char Inverter::GET_INVERTER[] = {0x55, 0xAA, 0x00, 0x40, 0x02, 0x00, 0x0B, 0x49, 0x20, 0x41, 0x4D, 0x20, 0x53, 0x45, 0x52, 0x56, 0x45, 0x52, 0x04, 0x3A};

// inverters then connect to our running server - which is this code

const unsigned char Inverter::MSGS[Inverter::MSG_TOTAL][9] =
{
    {0x55, 0xAA, 0x01, 0x03, 0x02, 0x00, 0x00, 0x01, 0x05}, // about inverter
    {0x55, 0xAA, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04}, // data message
};

Inverter::Inverter()
{
    // set up our UDP socket
    // this is used to find all inverters on the LAN
    m_udpsocket = new QUdpSocket(this);

    // setup our tcp server
    // this is used to send control messages to any inverters that respond to UDP messages
    m_server    = new QTcpServer(this);

    // start the TCP server and listen on the standard samil port
    connect(m_server, SIGNAL(newConnection()),this, SLOT(newClient()));
    m_server->listen(QHostAddress::Any, TCP_PORT);

    // start a new timer that will send out UDP discovery messages on timeout
    connect(&m_connectTimer, SIGNAL(timeout()), this, SLOT(doConnect()));
    m_connectTimer.start(CONNECTION_TIME);

}
Inverter::~Inverter()
{
    m_socket->close();

    delete m_server;
    delete m_udpsocket;

}

void Inverter::doConnect()
{
    // go get some inverters
    // this just sends out a multicast IAMSEVER and we wait for response
    m_udpsocket->writeDatagram((const char *)GET_INVERTER, 20, QHostAddress::Broadcast, UDP_PORT);
}

void Inverter::newClient()
{
    // an inverter has responded
    // add this to our main socket
    // note that this currently only handes one inverter
    m_socket = m_server->nextPendingConnection();

    if (m_socket)
    {
        // set up socket and signals for asynchronouse connection
        // start a timer that will ask the inverter for its front screen
        // display
        m_socket->setReadBufferSize(READ_BUFFER_SIZE);

        // general socket signals
        connect(m_socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
        connect(m_socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

        // start a timer to poll the inverter for current stats
        connect(&m_dataTimer, SIGNAL(timeout()), this, SLOT(doData()));
        m_dataTimer.start(DATA_TIME);
    }
}

void Inverter::doData()
{
    // cycle ensures that we first send out a message to ask for general inverter detail
    // once we have called this once, all subsequent messages ask for detail data
    static int cycle = MSG_DETAIL;

    m_socket->write((const char*)MSGS[cycle], 9);

    cycle = MSG_DATA;
}

void Inverter::disconnected()
{
    // we have been disconnected, try and reconnect
    qDebug() << "TCP disconnected...";
    m_connectTimer.start(CONNECTION_TIME);
}

void Inverter::readyRead()
{
    // read the data from the socket
    QByteArray data = m_socket->readAll();

    // if we have actually read some data
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

            // print out some nice CSV headings to std::out
            std::cout << "Date,Time,Temperature,Panel V, Panel I, Panel P, Grid V, Grid I, Grid P, Energy" << std::endl;
        }
        else
        {
            dataMsg dataMsgPtr;

            // The values that the inverter sends us only relate to -
            //      1 - the power being generated from the PV array
            //      2 - the power being exported to the grid
            // There is no way to get consumtion data from these.

            // heat sink temperature
            dataMsgPtr.temperature = (float)(((short)outData[ 7] << 8 & 0xff00) | (outData[ 8] & 0x00ff)) / 10.0f;

            // PV array outputs
            dataMsgPtr.panel1V     = (float)(((short)outData[ 9] << 8 & 0xff00) | (outData[10] & 0x00ff)) / 10.0f;
            dataMsgPtr.panel1I     = (float)(((short)outData[13] << 8 & 0xff00) | (outData[14] & 0x00ff)) / 10.0f;
            dataMsgPtr.panel1P     = dataMsgPtr.panel1V * dataMsgPtr.panel1I;

            // grid supply stats
            dataMsgPtr.gridI     = (float)(((short)outData[49] << 8 & 0xff00) | (outData[50] & 0x00ff)) / 10.0f;
            dataMsgPtr.gridV     = (float)(((short)outData[51] << 8 & 0xff00) | (outData[52] & 0x00ff)) / 10.0f;
            dataMsgPtr.gridF     = (float)(((short)outData[53] << 8 & 0xff00) | (outData[54] & 0x00ff));
            dataMsgPtr.gridP     = (float)(((short)outData[55] << 8 & 0xff00) | (outData[56] & 0x00ff));

            // "today" energy
            dataMsgPtr.energy     = (float)(((short)outData[23] << 8 & 0xff00) | (outData[24] & 0x00ff)) / 100.0f;

            std::cout << dataMsgPtr.timeStamp.date().toString("yyyy/MM/dd").toStdString() << ","
                      << dataMsgPtr.timeStamp.time().toString("HH:mm:ss").toStdString() << ","
                      << dataMsgPtr.temperature << ","
                      << dataMsgPtr.panel1V << ","
                      << dataMsgPtr.panel1I << ","
                      << dataMsgPtr.panel1P << ","
                      << dataMsgPtr.gridV << ","
                      << dataMsgPtr.gridI << ","
                      << dataMsgPtr.gridP << ","
                      << dataMsgPtr.energy << std::endl;
            qDebug()  << dataMsgPtr.timeStamp.time().toString ("HH:mm:ss");
        }
    }
}
