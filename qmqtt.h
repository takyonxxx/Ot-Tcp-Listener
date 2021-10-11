#include <QObject>
#include <QString>
#include <QTimer>

#include <mosquitto.h>
#include "networktools.h"

struct QMQTTMessage {
    QString topic;
    QString payload;
};

class QMQTT : public QThread {
    Q_OBJECT
public:   
    QMQTT(QObject * parent, otclient&);
    ~QMQTT();

    void setWill(QString willTopic, QString willPayload = "");
    bool connectMqtt(QString clientId, QString username = "", QString password = "");
    bool reConnectMqtt();
    void publish(QString topic, int qos = 0, bool retain = false);
    void publish(QString topic, QString payload, int qos = 0, bool retain = false);
    void subscribe(QString topic, int qos = 0);
    void unsubscribe(QString topic);
    bool isConnected();
    void disconnectMqtt();
    void set_tls(const char* cacert, const char* certfile, const char* keyfile);
    void set_tls_insecure(bool enabled);

    otclient &_client;

    static void onConnected(struct mosquitto *mosq, void *obj, int result)
    {
        Q_UNUSED(mosq);
        auto connected = (result == MOSQ_ERR_SUCCESS);
        if(connected){
            auto qmqtt = (QMQTT *)(obj);
            qmqtt->emitConnected();
        }
    }
    static void onDisconnected(struct mosquitto *mosq, void *obj, int result)
    {
        Q_UNUSED(mosq);
        Q_UNUSED(result);
        auto qmqtt = (QMQTT *)(obj);
        qmqtt->emitDisconnected();
    }

    static void onSubscribed(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
    {
        Q_UNUSED(mosq);
        Q_UNUSED(qos_count);
        Q_UNUSED(granted_qos);
        auto qmqtt = (QMQTT *)(obj);
        qmqtt->emitSubscribed(mid);;
    }

    static void onMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
    {
        Q_UNUSED(mosq);
        QMQTTMessage msg;
        msg.topic = message->topic;
        auto payload = std::string((char*)message->payload, (uint)message->payloadlen);
        msg.payload = QString::fromStdString(payload);

        auto qmqtt = (QMQTT *)(obj);
        qmqtt->emitMessage(msg);
    }

    static void onLog(struct mosquitto *mosq, void *obj, int level, const char *str)
    {
        Q_UNUSED(mosq);
        Q_UNUSED(obj);
        Q_UNUSED(level);
        printf("%s\n", str);
    }

    void emitConnected(){
        _connected = true;
        emit connected();
    }
    void emitConnectionError(){
        emit connectionError();
    }
    void emitDisconnected(){
        _connected = false;
        emit disconnected();
    }
    void emitSubscribed(int mid){
        _connected = true;
        emit subscribed(mid);
    }
    void emitMessage(QMQTTMessage message){
        emit gotMessage(message);
    }

    void pauseLoop();
    void resumeLoop();

private:
    struct mosquitto *_mosq;
    bool _stop{false};
    QString _clientId;
    QString _username;
    QString _password;
    bool _connected{false};
    bool _paused{false};
    QString _willTopic;
    QString _willPayload;

    int mid = 0;
    int nextMid();

private slots:
    void update();

protected:
    void run();


signals:
    void connected();
    void disconnected();
    void connectionError();
    void subscribed(int);
    void gotMessage(QMQTTMessage);

};
