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

#include <QCoreApplication>
#include <QSettings>
#include <stats.h>
#include <inverter.h>

Q_DECLARE_METATYPE(Inverter::dataMsg)


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("SolarSamil");
    QCoreApplication::setApplicationVersion("1.0");

    qRegisterMetaType<Inverter::dataMsg>();

    QSettings settings("config.ini", QSettings::IniFormat);

    QString id   = settings.value("System Id").toString();
    QString key  = settings.value("API Key")  .toString();
    int     rate = settings.value("Rate")     .toInt();

    if (id.length() && key.length())
    {
        Inverter inv;
        Stats    stats(id, key, rate);

        QObject::connect(&inv, SIGNAL(newData(Inverter::dataMsg)), &stats, SLOT(doNewData(Inverter::dataMsg)));
        QObject::connect(&inv, SIGNAL(newDay()), &stats, SLOT(doNewDay()));

        inv.doConnect();

        return a.exec();
    }

    return 1;
}
