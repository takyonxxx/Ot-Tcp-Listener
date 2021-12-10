QT -= gui
QT += core network xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 console
CONFIG -= app_bundle
TARGET = otlistener

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



include($$PWD/qtservice/src/qtservice.pri)

RESOURCES += \
    resources.qrc

QMAKE_LIBDIR += /usr/lib /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu
LIBS += -lmosquitto
LIBS += -lpcap

#sudo apt install -y qt5-default
#sudo apt install -y mosquitto-clients
#sudo apt install -y libmosquitto-dev
#sudo apt-get install -y libpcap-dev

#Usefull:
#sudo arpsend -D -e 192.168.10.10 wlp5s0
#sudo tcpdump -i eth0 -n udp port 10514 -T cnfp
#sudo tcpdump -nni eth0 port 10514 -w netflow.pcap
#sudo tcpreplay -i wlp5s0 netflow.pcap
#sudo tcpdump -nni wlp5s0 arp

#sudo kill $(lsof -t -i:9000)

HEADERS += \
    adapter.h \
    arp.h \
    helper.h \
    httprequest.h \
    logger.h \
    modbus.h \
    networktools.h \
    otlistener.h \
    otservice.h \
    sniffer.h

SOURCES += \
    adapter.cpp \
    logger.cpp \
    main.cpp \
    modbus.cpp \
    otlistener.cpp \
    otservice.cpp \
    sniffer.cpp


