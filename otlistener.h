#ifndef OTLISTENER_H
#define OTLISTENER_H

#include <QObject>
#include "mqttpublisher.h"
#include "adapter.h"
#include "logger.h"


class OtListener : public QObject
{
    Q_OBJECT
public:
    OtListener(QObject* parent, QString &_currentPath);
    ~OtListener();
     void init();
     void pause();
     void resume();
     void stop();
     void clean();

private:
    bool m_authorized{false};
    MqttPublisher *mqttPublisher{};
    MosqManager *mosqManager{};
    NetworkTools networkTools{};

    otclient _ot_client{};
    QString settingsFile{};
    QString currentPath{};

    void loadSettings();
    void saveSettings();

    typedef std::pair<QString, QString> adapterId;
    QList<Adapter*> adapterList{};

    Adapter* getAdapter(QString&);

private slots:
    void receivedMessage(QMQTTMessage);
};

#endif // OTLISTENER_H
