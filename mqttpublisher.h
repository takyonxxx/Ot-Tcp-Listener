#ifndef MQTTPUBLISHER_H
#define MQTTPUBLISHER_H

#include <QObject>
#include "mosqmanager.h"

class MqttPublisher : public QThread
{
    Q_OBJECT
public:
    MqttPublisher(QObject* parent, MosqManager *mqtt, otclient&);
    ~MqttPublisher();

    static MqttPublisher* getInstance();

    void update_asset(const asset & _asset);
    void set_initial_assets(QJsonObject& assets);    
    void stopLoop();
private:

    bool m_stop{false};

    MosqManager *m_Mqtt{};

    QQueue<asset> publish_queue{};   
    QList<asset*> assets{};   

    QThread *threadSendAgent{};

    otclient &_client;

    typedef std::pair<double, double> assetTimes;
    std::map<QString, assetTimes> asset_sources{};


    void publish_asset_loop();       
    void update_asset_source(asset&, double, bool);
    void publish_asset_source(asset&);

    double getCreatedTime(QString&);
    double getUpdateTime(QString&);
    void setCreateTime(QString&, double);
    void setUpdateTime(QString&, double);
    void sendMessage(QMQTTMessage&);

    static MqttPublisher* instance;

protected:
    void run();

};

#endif // MQTTPUBLISHER_H
