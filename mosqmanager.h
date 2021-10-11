#ifndef MOSQMANAGER_H
#define MOSQMANAGER_H

#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "qmqtt.h"

class MosqManager : public QThread
{
    Q_OBJECT

public:
    MosqManager(QObject* parent, otclient&);
    ~MosqManager();

    static MosqManager* getInstance();

protected:
    void run();

private:
    /*Aes aes{};
    EVP_CIPHER_CTX *en, *de;*/

    bool m_connected{false};
    bool m_stop{false};
    bool _paused{false};
    otclient &_client;
    QMQTT* _mqtt {};
    QStringList subed{};

    void subscribe(QString &topic);
    void unsubscribe(QString &topic);
    void stopLoop();

public:
    void disconnectMqtt();
    bool connectMqtt(QString &);
    void sendMessage(QMQTTMessage );
    void sendInitial();
    void sendHeartBeat();   
    void pauseLoop();
    void resumeLoop();

signals:
    void mqttReceivedMessage(QMQTTMessage);
    void mqttStarted();


public slots:    
    void subscribed(int);
    void connected();
    void connectionError();
    void disconnected();
    void onMessageReceived(QMQTTMessage);
    void on_log(int level, const char *str);

private:
    static MosqManager* instance;

};

#endif // MOSQMANAGER_H
