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

SOURCES += \
        adapter.cpp \
        logger.cpp \
        main.cpp \
        mosqmanager.cpp \
        mqttpublisher.cpp \
        otlistener.cpp \
        otservice.cpp \
        pythonscript.cpp \
        qmqtt.cpp

HEADERS += \
    adapter.h \
    emb.h \
    helper.h \
    httprequest.h \
    logger.h \
    mosqmanager.h \
    mqttpublisher.h \
    networktools.h \
    otlistener.h \
    otservice.h \
    arp.h \
    pythonscript.h \
    qmqtt.h

include($$PWD/qtservice/src/qtservice.pri)


unix:!mac{
    SOURCES += sniffer.cpp arp.h
    HEADERS += sniffer.h
}

RESOURCES += \
    resources.qrc

greaterThan(QT_MAJOR_VERSION, 4) {
    TARGET_ARCH=$${QT_ARCH}
} else {
    TARGET_ARCH=$${QMAKE_HOST.arch}
}

macx{
    message("Macx enabled")
    QMAKE_INCDIR += /usr/local/opt/mosquitto/include
    LIBS += -L/usr/local/opt/mosquitto/lib -lmosquitto
}

unix:!macx{
    message("Unix enabled")
    QMAKE_INCDIR += $$PWD/libs/Mosquitto/x64/include
    QMAKE_INCDIR += /usr/include/python3.8
    QMAKE_LIBDIR += /usr/lib /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu
    LIBS += -lmosquitto
    LIBS += -lpython3.8
    LIBS += -lpcap
}

#brew install mosquitto
#brew install qt5
#brew link qt5 --force

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