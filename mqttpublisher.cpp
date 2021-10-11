#include "mqttpublisher.h"
#include "logger.h"

MqttPublisher* MqttPublisher::instance = nullptr;

MqttPublisher* MqttPublisher::getInstance()
{
    if (instance != nullptr)
    {
        return instance;
    }
    else
    {
        return nullptr;
    }
}

MqttPublisher::MqttPublisher(QObject *parent, MosqManager *mqtt, otclient& client)
    : QThread(parent) ,m_Mqtt(mqtt), _client(client)
{    
    qRegisterMetaType<asset>("asset");

    instance = this;

    asset_sources.clear();
}

MqttPublisher::~MqttPublisher()
{
}

void MqttPublisher::update_asset(const asset & _asset)
{
    publish_queue.enqueue(_asset);
}

void MqttPublisher::publish_asset_loop()
{
    while (true)
    {
        if (m_stop)
            break;

        if(!publish_queue.isEmpty())
        {
            auto asset = publish_queue.dequeue();
            if(asset.ip.length() == 0)
                return;

            auto currentTime = static_cast<double>(QDateTime::currentMSecsSinceEpoch()/1000.0);
            update_asset_source(asset, currentTime, true);
        }

        QThread::msleep(500);
    }
}

void MqttPublisher::set_initial_assets(QJsonObject &assets)
{   
    asset_sources.clear();

    foreach(const QString& key, assets.keys())
    {
        QJsonValue value = assets.value(key);
        assetTimes times = std::make_pair(static_cast<double>(QDateTime::currentMSecsSinceEpoch()/1000.0),
                                          static_cast<double>(QDateTime::currentMSecsSinceEpoch()/1000.0));
        asset_sources.insert (std::pair<QString, assetTimes>(key, times));
    }
    Logger::getInstance()->write(QString("Received sensor initial asset data [%1]").arg(asset_sources.size()));
}

void MqttPublisher::publish_asset_source(asset &_asset)
{

    QString topic = "sensor/asset";

    double cTime = static_cast<double>(getCreatedTime(_asset.ip));
    double uTime = static_cast<double>(getUpdateTime(_asset.ip));

    QJsonObject data{};
    QJsonArray softwares{};

    data["sensor_name"] = _client.ifaceIp;
    data["ip"] = _asset.ip;
    data["mac"] = _asset.mac;
    data["hostname"] = _asset.hostname;
    data["nmap"] = _asset.nmap;
    data["created"] = cTime;
    data["lastseen"] = uTime;
    data["detection_method"] = _asset.adapterId;
    data["description"] = QString("ot asset");
    data["event_type"] = _asset.eventType;

    QJsonObject payload{};

    payload["sensor"] = _asset.ip;
    payload["sensor_network"] = _asset.netSegment;
    payload["uid"] = _client.uID;
    payload["mode"] = _asset.adapterMode;
    payload["data"] = data;

    QJsonDocument doc(payload);
    QString strPayload(doc.toJson(QJsonDocument::Compact));

    QMQTTMessage msg;
    msg.topic = topic;
    msg.payload = strPayload;

    sendMessage(msg);
}

void MqttPublisher::update_asset_source(asset &_asset, double updateTime, bool update)
{
    auto currentTime = static_cast<double>(QDateTime::currentMSecsSinceEpoch()/1000.0);

    auto it = asset_sources.find(_asset.ip);
    if (it == asset_sources.end())
    {
        assetTimes times = std::make_pair(currentTime, updateTime);
        asset_sources.insert (std::pair<QString, assetTimes>(_asset.ip, times));
        setCreateTime(_asset.ip, currentTime);
        setUpdateTime(_asset.ip, updateTime);

        const QString content =
                QLatin1String("Found Asset: ")
                + QLatin1String("Ip: ") + _asset.ip
                + QLatin1String(" Source: ") + _asset.adapterMode
                + QLatin1String(" Host: ") + _asset.hostname
                + QLatin1String(" Total: ") + QString::number(asset_sources.size());
        Logger::getInstance()->write(content);

    }
    else if(update)
    {
        setUpdateTime(_asset.ip, currentTime);
        const QString content =
                QLatin1String("Update Asset: ")
                + QLatin1String("Ip: ") + _asset.ip
                + QLatin1String(" Source: ") + _asset.adapterMode
                + QLatin1String(" Host: ") + _asset.hostname
                + QLatin1String(" Total: ") + QString::number(asset_sources.size());

        Logger::getInstance()->write(content);
    }

    if(!_client.authorized)
    {
        Logger::getInstance()->write(QString("Sensor not authorized..."));
        return;
    }

    publish_asset_source(_asset);
}

void MqttPublisher::sendMessage(QMQTTMessage &msg)
{
    if(m_Mqtt)
    {
        m_Mqtt->sendMessage(msg);
    }
}

void MqttPublisher::run()
{
    publish_asset_loop();
}

double MqttPublisher::getCreatedTime(QString& id)
{
    for (auto & element : asset_sources)
    {
        if(id == element.first)
        {
            return element.second.first;
        }
    }
    return 0.0;
}

double MqttPublisher::getUpdateTime(QString& id)
{
    for (auto & element : asset_sources)
    {
        if(id == element.first)
        {
            return element.second.second;
        }
    }
    return 0.0;
}

void MqttPublisher::setCreateTime(QString &ip, double cTime)
{
    for (auto & element : asset_sources)
    {
        if(ip == element.first)
        {
            element.second.first = cTime;
        }
    }
}

void MqttPublisher::setUpdateTime(QString &ip, double uTime)
{
    for (auto & element : asset_sources)
    {
        if(ip == element.first)
        {
            element.second.second = uTime;
        }
    }
}

void MqttPublisher::stopLoop()
{
    m_stop = true;
}



