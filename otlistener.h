#ifndef OTLISTENER_H
#define OTLISTENER_H

#include <QObject>
#include "adapter.h"
#include "logger.h"


class OtListener : public QObject
{
    Q_OBJECT
public:
    OtListener(QObject* parent);
    ~OtListener();
     void init();
     void stop();
     void setPause(bool);

private:   
    NetworkTools networkTools{};

    otclient _ot_client{};
    QList<Adapter*> adapterList{};
};

#endif // OTLISTENER_H
