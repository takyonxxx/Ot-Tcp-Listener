#include "otservice.h"

OtService::OtService(int &argc, char **argv)
    : QtService<QCoreApplication>(argc, argv, "OtService")
{
    setServiceDescription("This is ot service. ");
    setServiceFlags(QtServiceBase::CanBeSuspended);
    setStartupType(QtServiceController::AutoStartup);
    QString currentPath;

    currentPath = QtService::instance()->serviceFolder().replace("\\","/");
    std::string f = currentPath.toStdString();
    currentPath = f.substr(0, f.find_last_of( "\\/" )).c_str();
    QString fileName = currentPath + "/ot_log.txt";

    logger = new Logger(nullptr, fileName, true, true);
    ot_listener = new OtListener(nullptr, currentPath);
}

OtService::~OtService()
{
    ot_listener->stop();
    delete ot_listener;
    delete logger;
}

void OtService::start()
{
    ot_listener->init();    
    logger->write("Ot service started...");    
}

void OtService::pause()
{
    ot_listener->pause();
    logger->write("Ot service paused...");
}

void OtService::resume()
{
    ot_listener->resume();
    logger->write("Ot service resumed...");
}

void OtService::stop()
{
    ot_listener->stop();
    logger->write("Ot service stopped...");
}
