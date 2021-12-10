#include <QCoreApplication>
#include "otservice.h"

int main(int argc, char *argv[])
{
    OtService service(argc, argv);
    return service.exec();

    /*QCoreApplication a(argc, argv);

    return a.exec();*/
}
