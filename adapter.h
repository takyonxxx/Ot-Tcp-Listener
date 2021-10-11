#ifndef ARPSNIFFER_H
#define ARPSNIFFER_H

#include <QThread>
#if (defined (Q_OS_LINUX))
#include "sniffer.h"
#endif


class PythonScript;

class Adapter : public QObject
{
    Q_OBJECT

public:
    Adapter(QObject* parent, QString, QString ,otclient&);
    ~Adapter();

    QString getKey() const;
    void stopLoop();
private:
#if (defined (Q_OS_LINUX))
    Sniffer *sniffer{};
#endif

    otclient &_client;

    QString m_id{};
    QString m_mode{};

protected:
    void run();
};

#endif // ARPSNIFFER_H
