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
    Adapter(QObject* parent, otclient&);
    ~Adapter();

    void stopLoop();
    void setPause(bool);
private:
#if (defined (Q_OS_LINUX))
    Sniffer *sniffer{};
#endif

    otclient &_client;

protected:
    void run();
};

#endif // ARPSNIFFER_H
