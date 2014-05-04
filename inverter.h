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
    static const unsigned char GET_INVERTER[];
    static const unsigned char GET_DETAIL[];
    static const unsigned char GET_DATA[];

    const QString m_serial;
    QTcpServer * m_server;
    QTcpSocket * m_socket;
    QUdpSocket * m_udpsocket;

    QTimer m_connectTimer;
    QTimer m_dataTimer;
};

#endif // INVERTER_H
