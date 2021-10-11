#include "qmqtt.h"

#include <QThread>
#include <QDir>
#include <QFileInfo>
#include "logger.h"

#define timeoutMqtt 60
#define timeoutLoopMqtt 120
#define timeoutConnectMqtt 3000

int on_password_check(char *buf, int size, int rwflag, void *userdata)
{
    Q_UNUSED(size);
    Q_UNUSED(rwflag);
    Q_UNUSED(userdata);

    int length = 0;
    if(!buf)
        return 0;
    return length;
}

QMQTT::QMQTT(QObject * parent, otclient& client) : QThread(parent), _client(client)
{
    qRegisterMetaType<QMQTTMessage>("QMQTTMessage");
    mosquitto_lib_init();
    _mosq = mosquitto_new("QMQTT", true, this);
    //mosquitto_log_callback_set(_mosq, onLog);
    //mosquitto_max_inflight_messages_set(_mosq, 1);
    //mosquitto_reconnect_delay_set(_mosq, 3, 60, true);/
    mosquitto_connect_callback_set(_mosq, onConnected);
    mosquitto_disconnect_callback_set(_mosq, onDisconnected);
    mosquitto_message_callback_set(_mosq, onMessage);
    mosquitto_subscribe_callback_set(_mosq, onSubscribed);
}

QMQTT::~QMQTT() {
    _stop = true;
    mosquitto_disconnect(_mosq);
    mosquitto_destroy(_mosq);
    mosquitto_lib_cleanup();
}

int QMQTT::nextMid(){
    mid++;

    if(mid > 65536) {
        mid = 0;
    }

    return mid;
}

void QMQTT::setWill(QString topic, QString payload) {
    _willTopic = topic;
    _willPayload = payload;
}

bool QMQTT::reConnectMqtt()
{
    int rc  = mosquitto_reconnect(_mosq);
    while(rc != MOSQ_ERR_SUCCESS) {
        Logger::getInstance()->write(QString("MQTT Reconnect error: %1").arg (mosquitto_strerror(rc)));
        rc  = mosquitto_reconnect(_mosq);
    }

    Logger::getInstance()->write(QString("MQTT Reconnected..."));

    return true;
}

void QMQTT::set_tls(const char* cacert, const char* certfile, const char* keyfile) {
    int rc = mosquitto_tls_set(_mosq, cacert, NULL, certfile, keyfile, &on_password_check);
    if(rc!= MOSQ_ERR_SUCCESS)
        Logger::getInstance()->write(QString("Error setting tls settings : %1").arg (mosquitto_strerror(rc)));
}

void QMQTT::set_tls_insecure(bool enabled) {
    int value = (enabled?1:0);
    int rc = mosquitto_tls_insecure_set(_mosq, value);
    if(rc != MOSQ_ERR_SUCCESS)
        Logger::getInstance()->write(QString("TLS insecure set failed : %1").arg (mosquitto_strerror(rc)));
}

void QMQTT::pauseLoop()
{
    _paused = true;
}

void QMQTT::resumeLoop()
{
    _paused = false;
}

bool QMQTT::connectMqtt(QString clientId, QString username, QString password)
{
    _stop = false;
    _clientId = clientId;
    _username = username;
    _password = password;
    _connected = false;

    mosquitto_reinitialise(_mosq, _clientId.toStdString().c_str(), true, this);
    mosquitto_connect_callback_set(_mosq, onConnected);
    mosquitto_disconnect_callback_set(_mosq, onDisconnected);
    mosquitto_message_callback_set(_mosq, onMessage);
    mosquitto_subscribe_callback_set(_mosq, onSubscribed);
    //mosquitto_log_callback_set(_mosq, onLog);

    QString caFilePath = _client.caFilePath;
    QString clientCrtFilePath = _client.clientCrtFilePath;
    QString clientKeyFilePath = _client.clientKeyFilePath;

    const QString content =
            QString("Setting mqtt tls ... : ") + "\n"
            + QString("   %1").arg(caFilePath) + "\n"
            + QString("   %1").arg(clientCrtFilePath) + "\n"
            + QString("   %1").arg(clientKeyFilePath) ;

    Logger::getInstance()->write(content);

    set_tls(caFilePath.toStdString().c_str(), clientCrtFilePath.toStdString().c_str(), clientKeyFilePath.toStdString().c_str());
    set_tls_insecure(true);

    if(!_username.isEmpty() && !_password.isEmpty()) {
        mosquitto_username_pw_set(_mosq, _username.toStdString().c_str(), _password.toStdString().c_str());
    }

    if(!_willTopic.isEmpty() && !_willPayload.isEmpty()) {
        mosquitto_will_set(_mosq, _willTopic.toStdString().c_str(), (int)_willPayload.length(), _willPayload.toStdString().c_str(), 0, false);
    }

    Logger::getInstance()->write(QString("Connecting to mqtt : %1 port : %2").arg (_client.hostMqtt).arg(_client.hostMqttPort));

    int rc = mosquitto_connect(_mosq, _client.hostMqtt.toStdString().c_str(),  _client.hostMqttPort.toInt(), timeoutMqtt);

    while(rc != MOSQ_ERR_SUCCESS) {
        Logger::getInstance()->write(QString("Connect error : %1").arg (mosquitto_strerror(rc)));
        Logger::getInstance()->write(QString("Connecting to mqtt : %1 port : %2").arg (_client.hostMqtt).arg(_client.hostMqttPort));
        rc = mosquitto_connect(_mosq, _client.hostMqtt.toStdString().c_str(),  _client.hostMqttPort.toInt(), timeoutMqtt);
        QThread::msleep(timeoutConnectMqtt);
    }
    _connected = true;
    start();

    return _connected;
}

void QMQTT::publish(QString topic, int qos, bool retain) {
    publish(topic, "", qos,retain);
}

void QMQTT::publish(QString topic, QString payload, int qos, bool retain) {
    int mid = nextMid();
    mosquitto_publish(_mosq, &mid, (char*)topic.toStdString().c_str(), (int)payload.length(), (char*)payload.toStdString().c_str(), qos, retain);
}

void QMQTT::subscribe(QString topic, int qos) {
    int mid = nextMid();
    mosquitto_subscribe(_mosq, &mid, (char*)topic.toStdString().c_str(), qos);
}

void QMQTT::unsubscribe(QString topic) {
    int mid = nextMid();
    mosquitto_unsubscribe(_mosq, &mid, (char*)topic.toStdString().c_str());
}

void QMQTT::update() {
    int rc1 = mosquitto_loop(_mosq, 0, 1);
    if (rc1 != MOSQ_ERR_SUCCESS) {
        Logger::getInstance()->write(QString("Loop error : %1").arg (mosquitto_strerror(rc1)));
        int rc2 = mosquitto_reconnect(_mosq);
        if (rc2 != MOSQ_ERR_SUCCESS) {
            Logger::getInstance()->write(QString("Reconnect error : %1").arg (mosquitto_strerror(rc2)));
        }
    }
}

void QMQTT::run()
{
    do
    {
        if(_stop)
            break;

        if(_paused)
            continue;

        int rc = mosquitto_loop(_mosq, 0, 1);
        if (rc != MOSQ_ERR_SUCCESS) {
            Logger::getInstance()->write(QString("MQTT Loop error. Trying to reconnect."));
            disconnectMqtt();
            emitConnectionError();
        }

        QThread::msleep(timeoutLoopMqtt);
    }
    while(_connected);
}

bool QMQTT::isConnected() {
    return _connected;
}

void QMQTT::disconnectMqtt() {

    _stop = true;
    _connected = false;

    mosquitto_disconnect(_mosq);
    mosquitto_destroy(_mosq);
    mosquitto_lib_cleanup();
}

/*void QMQTT::startLoop()
{
    mosquitto_loop_forever(_mosq, -1, 1);
}*/
