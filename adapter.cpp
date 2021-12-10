#include "adapter.h"

Adapter::Adapter(QObject *parent, otclient &client)
    : QObject(parent),  _client(client)
{
#if (defined (Q_OS_LINUX))    
        sniffer = new Sniffer(this, _client);
        sniffer->start();
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

void Adapter::setPause(bool _pause)
{
    if (_pause && sniffer)
    {
        sniffer->stopLoop();
        sniffer->quit(); //Tell the thread to abort
        sniffer->wait(500); //We have to wait again here!
    }
    else if (!_pause && sniffer)
    {
        sniffer->start();
    }
}
