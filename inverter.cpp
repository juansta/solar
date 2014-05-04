#include "inverter.h"
#include <QDebug>

// initial UDP message that inverters respond to
// this is broadcast to 255.255.255.255
// this is the "I AM SERVER"
const unsigned char Inverter::GET_INVERTER[] = {0x55, 0xAA, 0x00, 0x40, 0x02, 0x00, 0x0B, 0x49, 0x20, 0x41, 0x4D, 0x20, 0x53, 0x45, 0x52, 0x56, 0x45, 0x52, 0x04, 0x3A};

// inverters then connect to our running server - which is this code

// messages sent from server to inverter
const unsigned char Inverter::GET_DETAIL[] = {0x55, 0xAA, 0x01, 0x03, 0x02, 0x00, 0x00, 0x01, 0x05};
const unsigned char Inverter::GET_DATA  [] = {0x55, 0xAA, 0x01, 0x00, 0x02, 0x00, 0x00, 0x01, 0x02};

Inverter::Inverter()
{
    // setup our tcp server
    m_server = new QTcpServer(this);

    connect(m_server, SIGNAL(newConnection()),this, SLOT(newClient()));
    m_server->listen(QHostAddress::Any, 1200);

    connect(&m_connectTimer, SIGNAL(timeout()), this, SLOT(doConnect()));
    m_connectTimer.start(2000);
}
Inverter::~Inverter()
{
}

void Inverter::doConnect()
{
    m_udpsocket = new QUdpSocket(this);

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
        //connect(m_socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
        connect(m_socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

        connect(&m_dataTimer, SIGNAL(timeout()), this, SLOT(doData()));

        m_dataTimer.start(1000);
    }
}

void Inverter::doData()
{
    m_socket->write((const char*)GET_DETAIL, 9);
}

void Inverter::disconnected()
{
    qDebug() << "TCP disconnected...";
    // restart our inverter search
    m_connectTimer.start(2000);
}

void Inverter::readyRead()
{
    qDebug() << "\r\nTCP reading...";

    // read the data from the socket
    qDebug() << m_socket->readAll();
}
