#include "mosqmanager.h"
#include "logger.h"
#include "adapter.h"
#define HeartbeatPeriod 3000

MosqManager* MosqManager::instance = nullptr;

MosqManager* MosqManager::getInstance()
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

MosqManager::MosqManager(QObject *parent, otclient& client)
    : QThread(parent), m_stop(false), _client(client)
{
    instance = this;

    _mqtt = new QMQTT(this, _client);

    if(_mqtt)
    {
        connect(_mqtt, &QMQTT::connected, this, &MosqManager::connected);
        connect(_mqtt, &QMQTT::connectionError, this, &MosqManager::connectionError);
        connect(_mqtt, &QMQTT::disconnected, this, &MosqManager::disconnected);
        connect(_mqtt, &QMQTT::subscribed, this, &MosqManager::subscribed);
        connect(_mqtt, &QMQTT::gotMessage, this, &MosqManager::onMessageReceived);
        connectMqtt(_client.uID);
    }
}

MosqManager::~MosqManager()
{
    if(_mqtt)
    {
        auto topic = QString("sensor/heartBeat");
        unsubscribe(topic);

        topic = QString("server_replay");
        unsubscribe(topic);

        topic = QString("sensor_initial");
        unsubscribe(topic);

        topic = QString("uid");
        unsubscribe(topic);

        topic = QString("sensor_settings");
        unsubscribe(topic);

        topic = QString("global_settings");
        unsubscribe(topic);

        disconnectMqtt();

        _mqtt = nullptr;
    }
}

void MosqManager::run()
{
    if(_mqtt)
    {
        QString topic = "sensor/heartBeat";

        while (true)
        {           
            if (m_stop)
                break;

            if (!subed.contains(topic))
                continue;

            if(!m_connected)
                continue;

            if(_paused)
                continue;

            sendHeartBeat();

            QThread::msleep(HeartbeatPeriod);
        }
    }
}

void MosqManager::sendHeartBeat()
{
    QString topic = "sensor/heartBeat";

    QJsonObject data{};
    data["data"] = "nodata";
    QJsonObject payload{};
    payload["data"] = data;
    payload["mode"] = _client.type;
    payload["sensor"] = _client.ifaceIp;
    payload["uid"] = _client.uID;
    payload["sensor_network"] = _client.netSegment;

    QJsonDocument doc(payload);
    QString strPayload(doc.toJson(QJsonDocument::Compact));

    QMQTTMessage msg;
    msg.topic = topic;
    msg.payload = strPayload;
    //printf("%s : %s\n", msg.topic.toUtf8().toStdString().c_str(), msg.payload.toUtf8().toStdString().c_str());
    sendMessage(msg);
}

void MosqManager::stopLoop()
{
    m_stop = true;
    m_connected = false;
}

void MosqManager::subscribe(QString &topic)
{
    _mqtt->subscribe(topic);
    subed.append(topic);
}

void MosqManager::unsubscribe(QString &topic)
{
    _mqtt->unsubscribe(topic);
    subed.removeOne(topic);
}

void MosqManager::disconnectMqtt()
{    
    _mqtt->disconnectMqtt();
    m_stop = true;
    subed.clear();
    m_connected = false;
}

bool MosqManager::connectMqtt(QString &uid)
{
    return(_mqtt->connectMqtt(uid, "", ""));
}

void MosqManager::subscribed(int mid)
{
    Logger::getInstance()->write(QString("Topic %1 has been subscribed. ( %2 )").arg (subed[mid - 1]).arg(mid));
    if(subed[mid - 1].contains("sensor_initial"))
    {
        sendInitial();
        //Logger::getInstance()->write(QString("sendInitial..."));
    }
}

void MosqManager::disconnected()
{
}

void MosqManager::on_log(int level, const char *str)
{
    printf("[%d] %s\n", level, str);
}

void MosqManager::connected()
{
    auto topic = QString("sensor/heartBeat");
    subscribe(topic);

    topic = QString("server_replay");
    subscribe(topic);

    topic = QString("sensor_initial");
    subscribe(topic);

    topic = QString("uid");
    subscribe(topic);

    topic = QString("sensor_settings");
    subscribe(topic);

    topic = QString("global_settings");
    subscribe(topic);

    Logger::getInstance()->write(QString("Mqtt client connected on %1 %2").arg (_client.hostMqtt).arg(_client.hostMqttPort));
    start();

    m_connected = true;
}

void MosqManager::connectionError()
{
    m_stop = true;
    m_connected = false;

    if(_mqtt)
    {
        _mqtt = nullptr;     
        _mqtt = new QMQTT(this, _client);

        if(_mqtt)
        {
            connect(_mqtt, &QMQTT::connected, this, &MosqManager::connected);
            connect(_mqtt, &QMQTT::connectionError, this, &MosqManager::connectionError);
            connect(_mqtt, &QMQTT::disconnected, this, &MosqManager::disconnected);
            connect(_mqtt, &QMQTT::subscribed, this, &MosqManager::subscribed);
            connect(_mqtt, &QMQTT::gotMessage, this, &MosqManager::onMessageReceived);
            connectMqtt(_client.uID);
            m_stop = false;
        }
    }
}

void MosqManager::sendInitial()
{
    QString topic = "sensor/sensor_initial";

    QJsonObject data{};
    data["data"] = "nodata";
    QJsonObject payload{};

    payload["sensor"] = _client.ifaceIp;
    payload["sensor_network"] = _client.netSegment;
    payload["uid"] = _client.uID;
    payload["mode"] = "sensor";
    payload["data"] = data;

    QJsonDocument doc(payload);
    QString strPayload(doc.toJson(QJsonDocument::Compact));

    QMQTTMessage msg;
    msg.topic = topic;
    msg.payload = strPayload;

    sendMessage(msg);
}

void MosqManager::onMessageReceived(QMQTTMessage msg)
{
    emit mqttReceivedMessage(msg);
}

void MosqManager::sendMessage(QMQTTMessage msg)
{
    if(!_mqtt)
    {
        Logger::getInstance()->write(QString("Mqtt not started..."));
        return;
    }
    _mqtt->publish(msg.topic, msg.payload);
}

void MosqManager::pauseLoop()
{
    _paused = true;
    if(_mqtt)
        _mqtt->pauseLoop();
}

void MosqManager::resumeLoop()
{
    _paused = false;
    if(_mqtt)
        _mqtt->resumeLoop();
}
