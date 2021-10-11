#include "adapter.h"

Adapter::Adapter(QObject *parent, QString id, QString mode, otclient &client)
    : QObject(parent),  _client(client), m_id(id), m_mode(mode)
{        
    qRegisterMetaType<asset>("asset");

#if (defined (Q_OS_LINUX))
    if(m_mode == "ip")
    {
        sniffer = new Sniffer(this, _client, m_id, m_mode);
        sniffer->start();
    }
#endif
}

Adapter::~Adapter()
{

#if (defined (Q_OS_LINUX))
    if (sniffer)
    {
        sniffer->stopLoop();
        sniffer->quit(); //Tell the thread to abort
        sniffer->wait(500); //We have to wait again here!
    }
#endif  
}

void Adapter::stopLoop()
{

#if (defined (Q_OS_LINUX))
    if (sniffer)
    {
        sniffer->stopLoop();
        sniffer->quit(); //Tell the thread to abort
        sniffer->wait(500); //We have to wait again here!
    }
#endif
}

QString Adapter::getKey() const
{
    return m_id;
}
