#include "otlistener.h"
#include "logger.h"

OtListener::OtListener(QObject *parent)
    : QObject(parent)
{    

}

OtListener::~OtListener()
{
}

void OtListener::init()
{   
    QString device, ip, mac, mask;
    std::string sVendor;
    QJsonObject vendor{};

    _ot_client.type = "sensor";

    networkTools.getDeviceInfo(device, ip, mac, mask);
    _ot_client.iface = networkTools.cleanQString(device);
    _ot_client.ifaceIp = ip;
    _ot_client.ifaceMac = mac;
    auto netSegment = networkTools.geNetSegment(_ot_client.ifaceIp, mask) + "/24";
    _ot_client.netSegment = netSegment;

    const QString content =
            QString("Ot Listener Info:\n")
            + QString("Interface : ") + _ot_client.iface + "\n"
            + QString("Ip : ") + _ot_client.ifaceIp + "\n"
            + QString("Mac : ") + _ot_client.ifaceMac + "\n"
            + QString("Netsegment : ") + _ot_client.netSegment;

    Logger::getInstance()->write(content);

    Adapter *adapter = new Adapter(this, _ot_client);
    adapterList.append(adapter);
    Logger::getInstance()->write(QString("New Adapter [%1] added to sensor.").arg("ip"));
}

void OtListener::stop()
{
    for(auto* adapter : adapterList)
    {
        if(adapter)
        {
            adapter->stopLoop();
        }
    }
    adapterList.clear();
}

void OtListener::setPause(bool _pause)
{
    for(auto* adapter : adapterList)
    {
        if(adapter)
        {
            adapter->setPause(_pause);
        }
    }
}

