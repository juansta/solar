#include <QCoreApplication>
#include <inverter.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Inverter inv;

    inv.doConnect();

    return a.exec();
}
