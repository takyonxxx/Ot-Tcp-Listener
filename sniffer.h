#ifndef SNIFFER_H
#define SNIFFER_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include "mqttpublisher.h"
#include "arp.h"
#include "helper.h"
#include "modbus.h"


#define BUFSIZE 2048
#define SnifferSendArpPeriod 60
#define SnifferGetPacketPeriod 10

class PythonScript;

class Sniffer: public QThread
{
public:
    Sniffer(QObject* parent, otclient&, QString&, QString&);
    ~Sniffer();

    void stopLoop();

private:
    void ProcessPacket(char*, int);
    void StartPcap();
    void SetPcapFilter(std::string);
    void processArpPacket(struct pcap_pkthdr const* header, const u_char*);
    void processIpPacket(struct pcap_pkthdr const* header, const u_char*);    
    void MonitorPcap( pcap_t *);
    static void *arpLoop(void * this_ptr);

    QJsonObject _data{};

    NetworkTools networkTools;
    pcap_t *handle{nullptr};
    char error_buffer[PCAP_ERRBUF_SIZE]; /* Size defined in pcap.h */   
    bool m_stop{false};
    Arp arp_manager{};
    otclient &_client;
    QString m_id{};
    QString m_mode{};
    QStringList segmentIpList{};
    pthread_t snifferSendArpThread{};
    PythonScript *pythonScript{};

protected:
    void run();
};

#endif // SNIFFER_H
