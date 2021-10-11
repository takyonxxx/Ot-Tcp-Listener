#include <QCoreApplication>
#include "otservice.h"
#include  <stdio.h>
#include  <signal.h>
#include  <stdlib.h>

void INThandler(int);
int main(int argc, char *argv[])
{
    signal(SIGINT, INThandler);
    OtService service(argc, argv);
    return service.exec();

    /*QCoreApplication a(argc, argv);

    return a.exec();*/
}
void  INThandler(int sig)
{
    signal(sig, SIG_IGN);
    printf("You hit Ctrl-C, exiting");
    system("ps aux | grep otlistener| awk '{print $2}'|xargs sudo kill -9");

    exit(0);
}
