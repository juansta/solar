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
            printf("\r\n0x");
            for (int i = 0; i < data.length(); i++)
                printf("%02X,", (unsigned char)outData[i]);
            printf("\r\n");

            dataMsg dataMsgPtr;

            dataMsgPtr.temperature = ((short)outData[ 7] << 8 & 0xff00) | (outData[ 8] & 0x00ff);
            dataMsgPtr.panel1V     = ((short)outData[ 9] << 8 & 0xff00) | (outData[10] & 0x00ff);
            dataMsgPtr.panel1I     = ((short)outData[13] << 8 & 0xff00) | (outData[14] & 0x00ff);
            dataMsgPtr.panel1P     = ((short)outData[17] << 8 & 0xff00) | (outData[18] & 0x00ff);

            dataMsgPtr.gridF     = ((short)outData[50] << 8 & 0xff00) | (outData[51] & 0x00ff);

            qDebug() << dataMsgPtr.temperature;
            qDebug() << dataMsgPtr.panel1V;
            qDebug() << dataMsgPtr.panel1I;
            qDebug() << dataMsgPtr.panel1P;
            qDebug() << dataMsgPtr.gridF;

        }
    }
}
