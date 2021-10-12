#include "otlistener.h"
#include "logger.h"

OtListener::OtListener(QObject *parent, QString &_currentPath)
    : QObject(parent), m_authorized(false), currentPath(_currentPath)
{
    qRegisterMetaType<asset>("asset");
    settingsFile = currentPath + "/otlistener.ini";
}

OtListener::~OtListener()
{
    clean();
}

void OtListener::init()
{
    if (QFile(settingsFile).exists())
    {
        loadSettings();
    }
    else
    {
        qDebug() << "not exists" << settingsFile;
        if( QFile(settingsFile).open( QIODevice::WriteOnly ) )
        {
            _ot_client.hostMqtt = "broker";
            _ot_client.hostMqttPort = "1883";
            _ot_client.uID = "ot_service";
            _ot_client.caFilePath = currentPath + "/certs/ca/ca.crt";
            _ot_client.clientCrtFilePath = currentPath + "/certs/client/client.crt";
            _ot_client.clientKeyFilePath = currentPath + "/certs/client/client.key";
            _ot_client.debug = false;

            saveSettings();
        }
    }

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
            QString("Ot Listener Info:\n   Uid : ") + _ot_client.uID + "\n"
            + QString("   Interface : ") + _ot_client.iface + "\n"
            + QString("   Ip : ") + _ot_client.ifaceIp + "\n"
            + QString("   Mac : ") + _ot_client.ifaceMac + "\n"
            + QString("   Netsegment : ") + _ot_client.netSegment;

    Logger::getInstance()->write(content);

    Adapter *adapter = new Adapter(this, "id", "ip", _ot_client);
    adapterList.append(adapter);
    Logger::getInstance()->write(QString("New Adapter [%1] added to sensor.").arg("ip"));

    /*mosqManager = new MosqManager(this, _ot_client);
    if(mosqManager)
    {
        connect (mosqManager, &MosqManager::mqttReceivedMessage, this, &OtListener::receivedMessage);
        mqttPublisher = new MqttPublisher(this, mosqManager, _ot_client);
        mqttPublisher->start();
    }*/
}

void OtListener::pause()
{
    mosqManager->pauseLoop();
}

void OtListener::resume()
{
    mosqManager->resumeLoop();
}

void OtListener::stop()
{
    clean();
}

void OtListener::clean()
{
    for(auto* adapter : adapterList)
    {
        if(adapter)
        {
            adapter->stopLoop();
        }
    }

    adapterList.clear();

    if(mqttPublisher)
    {
        mqttPublisher->stopLoop();
        mqttPublisher->quit();
        mqttPublisher->wait(500);
    }

    if (mosqManager)
    {
        mosqManager->disconnectMqtt();
    }
}

void OtListener::loadSettings()
{
    QSettings settings(settingsFile, QSettings::IniFormat);
    _ot_client.hostMqtt = settings.value("mqhost", "").toString();
    _ot_client.hostMqttPort = settings.value("mqhostport", "").toString();
    _ot_client.uID = settings.value("uid").toString();
    if(_ot_client.uID.isEmpty())
        _ot_client.uID = QString("asmaclient");
    _ot_client.caFilePath = settings.value("caFilePath").toString();
    _ot_client.clientCrtFilePath = settings.value("clientCrtFilePath").toString();
    _ot_client.clientKeyFilePath = settings.value("clientKeyFilePath").toString();
    _ot_client.debug = StringToBool(settings.value("debug").toString());
    Logger::getInstance()->setDebug(_ot_client.debug);
    Logger::getInstance()->write(QString("Load settings: ") + settingsFile);
}

void OtListener::saveSettings()
{
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("mqhost", _ot_client.hostMqtt);
    settings.setValue("mqhostport", _ot_client.hostMqttPort);
    settings.setValue("uid", _ot_client.uID);
    settings.setValue("caFilePath", _ot_client.caFilePath);
    settings.setValue("clientCrtFilePath", _ot_client.clientCrtFilePath);
    settings.setValue("clientKeyFilePath", _ot_client.clientKeyFilePath);
    settings.setValue("debug", BoolToString(_ot_client.debug));
    Logger::getInstance()->setDebug(_ot_client.debug);
    Logger::getInstance()->write(QString("Saved settings: ") + settingsFile);
}

void OtListener::receivedMessage(QMQTTMessage msg)
{
    if(!mqttPublisher)
        return;

    printf("%s : %s\n", msg.topic.toUtf8().toStdString().c_str(), msg.payload.toUtf8().toStdString().c_str());

    /*QJsonDocument jsonResponse = QJsonDocument::fromJson(msg.payload.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();

    auto sensor = jsonObject.value("sensor").toString();

    if(!sensor.isEmpty() && sensor != _ot_client.ifaceIp)
        return;

    if(msg.topic == QString("uid"))
    {
        _ot_client.uID = jsonObject.value("uid").toString() ;
        saveSettings();
        Logger::getInstance()->write(QString("Curent uid changed to : %1").arg ( _ot_client.uID));
    }
    else if(msg.topic == QString("server_replay"))
    {
        QJsonDocument jsonResponse = QJsonDocument::fromJson(msg.payload.toUtf8());
        QJsonObject jsonObject = jsonResponse.object();
        QJsonValue res = jsonObject.value("authorized");
        auto auth = res.toVariant().toBool();
        if(auth != _ot_client.authorized)
        {
            _ot_client.authorized = auth;
            Logger::getInstance()->write(QString("Curent authorization changed to : %1").arg (BoolToString(_ot_client.authorized)));
        }
    }
    else if(msg.topic == QString("sensor_initial"))
    {
        QJsonObject assets = jsonObject["assetList"].toObject();
        if(mqttPublisher)
            mqttPublisher->set_initial_assets(assets);

    }
    else if(msg.topic == QString("sensor_settings"))
    {
        for(auto* adapter : adapterList)
        {
            if(adapter)
            {
                adapter->stopLoop();
            }
        }

        adapterList.clear();

        auto jsonArray = jsonObject["adapters"].toArray();

        foreach (const QJsonValue & value, jsonArray)
        {
            QJsonObject obj = value.toObject();
            auto id = obj["_id"].toString();
            auto mode = obj["mode"].toString();

            QJsonObject sobj = obj["settings"].toObject();
            adapterId key = std::make_pair(id, mode);

            if (!mode.isEmpty())
            {
                Adapter *adapter = new Adapter(this, id, mode, _ot_client);
                adapterList.append(adapter);
                Logger::getInstance()->write(QString("New Adapter [%1] added to sensor. Total [%2]").arg (mode).arg(adapterList.size()));
            }
        }
    }*/
}

Adapter *OtListener::getAdapter(QString& id)
{
    if(adapterList.size() == 0)
        return nullptr;

    for(auto* adapter : adapterList)
    {
        if(adapter->getKey() == id)
        {
            return adapter;
        }
    }
    return nullptr;
}
