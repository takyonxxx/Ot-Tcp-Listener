#ifndef MYSERVICE_H
#define MYSERVICE_H

#include <qtservice.h>
#include <QCoreApplication>
#include <QObject>
#include "logger.h"
#include "otlistener.h"

class OtService : public QtService<QCoreApplication>
{
public:
    OtService(int &argc, char **argv);
    ~OtService();
    void start();
    void pause();
    void resume();
    void stop();

private:
    Logger *logger{nullptr};
    OtListener *ot_listener{nullptr};
};

#endif // MYSERVICE_H
