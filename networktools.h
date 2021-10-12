#ifndef PCAPFUNCTIONS_H
#define PCAPFUNCTIONS_H

#include <QtCore>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QtXml>

#include <iostream>
#include "httprequest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>           // strcpy, memset(), and memcpy()
#include <sstream>
#include <iomanip>
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t

#if (defined (Q_OS_LINUX) || defined (Q_OS_MAC))

#include <unistd.h>           // close()
#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_RAW, INET_ADDRSTRLEN
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <net/if.h>           // struct ifreq
#include <sys/ioctl.h>        // macro ioctl is defined
#include <sys/utsname.h>
#include <fcntl.h>
#include <errno.h>            // errno, perror()
#endif

#if defined(Q_OS_WIN)
#define close(s) closesocket(s)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winternl.h>
#include "checksoftwares.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Advapi32.lib")
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#endif


#define IPTOSBUFFERS    12
#define HOST_NAME_MAX   64
#define LOGIN_NAME_MAX	256
#define ERRBUF_SIZE 256
#define ARP_REQUEST 1    // Taken from <linux/if_arp.h>
#define ARP_REPLY 2

#define LastSeenTime 5.0

template <typename T>
bool Contains( std::list<T>& List, const T& Element )
{
    auto it = std::find(List.begin(), List.end(), Element);
    return it != List.end();
}

typedef struct _client otclient;
struct _client {
    QString uID{};
    QString iface{};
    QString ifaceIp{};
    QString ifaceMac{};
    QString netSegment{};   
    QString type{};
    QString hostMqtt{};
    QString hostMqttPort{};
    QString caFilePath{};
    QString clientCrtFilePath{};
    QString clientKeyFilePath{};
    bool authorized{false};
    bool debug{false};
};

typedef struct _asset asset;
struct _asset {
    QString ip{};
    QString mac{};   
    QString category{};
    QString adapterId{};
    QString adapterMode{};
    QString netSegment{};
    QJsonObject otData{};
};

Q_DECLARE_METATYPE(asset)


static std::string intToHexString(int intValue)
{
    std::string hexStr;
    /// integer value to hex-string
    std::stringstream sstream;
    sstream << std::setfill ('0') << std::setw(2) << std::hex << static_cast<int>(intValue);

    hexStr= sstream.str();
    sstream.clear();    //clears out the stream-string
    return hexStr;
}

// Function to split string str using given delimiter
static std::vector<std::string> split(const std::string& str, char delim)
{
    auto i = 0;
    std::vector<std::string> list;

    auto pos = str.find(delim);

    while (pos != std::string::npos)
    {
        list.push_back(str.substr(i, pos - i));
        i = ++pos;
        pos = str.find(delim, pos);
    }

    list.push_back(str.substr(i, str.length()));

    return list;
}

// check if given string is a numeric string or not
static bool isNumber(const std::string& str)
{
    // std::find_first_not_of searches the string for the first character
    // that does not match any of the characters specified in its arguments
    return !str.empty() &&
            (str.find_first_not_of("[0123456789]") == std::string::npos);
}


// Function to validate an IP address
static bool validateIP(std::string ip)
{
    // split the string into tokens
    std::vector<std::string> list = split(ip, '.');

    // if token size is not equal to four
    if (list.size() != 4)
        return false;

    // validate each token
    for (std::string str : list)
    {
        // verify that string is number or not and the numbers
        // are in the valid range
        if (!isNumber(str) || stoi(str) > 255 || stoi(str) < 0)
            return false;
    }

    if(ip == std::string("0.0.0.0"))
        return false;

    return true;
}

static inline QString BoolToString(bool b)
{
    return b ? "true" : "false";
}

static inline bool StringToBool(QString b)
{
    if(b.contains("true"))
        return true;
    else if(b.contains("false"))
        return false;
}

class NetworkTools
{
public:

    /*Port Range	Category
    0 – 1023	System Ports
    1024 – 49151	User Ports
    49152 – 65535	Dynamic Ports*/

    bool SetSocketBlockingEnabled(int fd, bool blocking)
    {
        if (fd < 0) return false;

#ifdef _WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? false : true;
#else
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(fd, F_SETFL, flags) == 0) ? false : true;
#endif
    }

    void getDeviceInfo (QString &device, QString &ip, QString &mac, QString &mask)
    {

        bool found = false;
        foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces())
        {
            unsigned int flags = interface.flags();
            bool isUp = (bool)(flags & QNetworkInterface::IsUp);
            bool isRunning = (bool)(flags & QNetworkInterface::IsRunning);
            bool isLoopback = (bool)(flags & QNetworkInterface::IsLoopBack);
            bool isP2P = (bool)(flags & QNetworkInterface::IsPointToPoint);

            if ( !interface.isValid() || !isUp || !isRunning || isLoopback || isP2P ) continue;

            foreach (QNetworkAddressEntry entry, interface.addressEntries())
            {
                // Ignore local host
                if ( entry.ip() == QHostAddress::LocalHost ) continue;

                // Ignore non-ipv4 addresses
                if ( !entry.ip().toIPv4Address() ) continue;

                if ( !found && interface.hardwareAddress() != "00:00:00:00:00:00" && entry.ip().toString().contains(".")
                     && !interface.humanReadableName().contains("VM"))
                {
                    device = interface.humanReadableName();
                    ip = entry.ip().toString();
                    mac = interface.hardwareAddress();
                    mask =  entry.netmask().toString();
                    found = true;
                }
            }
        }
    }

    QString getLocalHostName()
    {
        return QHostInfo::localHostName();
    }

    QString getHostName(QString &ip)
    {
        struct sockaddr_in sa;    /* input */
        socklen_t len;         /* input */
        char hbuf[NI_MAXHOST];

        memset(&sa, 0, sizeof(struct sockaddr_in));

        /* For IPv4*/
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());
        len = sizeof(struct sockaddr_in);

        if (getnameinfo((struct sockaddr *) &sa, len, hbuf, sizeof(hbuf),
                        NULL, 0, NI_NAMEREQD)) {
            return QString();
        }
        return hbuf;
    }

    QString geNetSegment(QString ifaceIp, QString mask)
    {
        QString ip{};
        std::string s = ifaceIp.toStdString();
        char *c_ip = const_cast<char*>(s.c_str());
        struct in_addr ipaddress, subnetmask;

        inet_pton(AF_INET, c_ip, &ipaddress);
        inet_pton(AF_INET, mask.toStdString().c_str(), &subnetmask);

        unsigned long first_ip = ntohl(ipaddress.s_addr & subnetmask.s_addr);
        //unsigned long last_ip = ntohl(ipaddress.s_addr | ~(subnetmask.s_addr));

        unsigned long long_address = htonl(first_ip);
        struct in_addr addr;
        addr.s_addr = long_address;
        ip = QString(inet_ntoa(addr));
        return ip;
    }

    bool getVendor(const std::string& mac, std::string& vendor)
    {
        try
        {
            auto query = "http://api.macvendors.com/" + mac;
            http::Request request(query);
            const http::Response response = request.send("GET");
            vendor = std::string(response.body.begin(), response.body.end());
        }
        catch (const std::exception& e)
        {
            std::cerr << "Request failed, error: " << e.what() << '\n';
            return false;
        }

        return true;
    }

    static inline QString changeTrCharacters(QString text){
        text.replace("ö", "o");
        text.replace("Ö", "O");
        text.replace("ü", "u");
        text.replace("Ü", "U");
        text.replace("ı", "i");
        text.replace("İ", "I");
        text.replace("ğ", "g");
        text.replace("Ğ", "G");
        text.replace("ş", "s");
        text.replace("Ş", "S");
        text.replace("ç", "c");
        text.replace("Ç", "C");
        return text;
    }


    static inline QString RemoveSpecials(QString qstr)
    {
        auto str = qstr.toStdString();
        int i=0,len=str.length();
        while(i<len)
        {
            char c=str[i];
            if (isprint( static_cast<unsigned char>( c ) ))
            {
                ++i;
            }
            else
            {
                str.erase(i,1);
                --len;
            }
        }
        return str.c_str();
    }

    static QString cleanQString(QString toClean) {

        QString toReturn =  RemoveSpecials(changeTrCharacters(toClean));
        return toReturn;
    }

    char *iptos(u_long in)
    {
        static char output[IPTOSBUFFERS][3*4+3+1];
        static short which;
        u_char *p;

        p = (u_char *)&in;
        which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
        sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        return output[which];
    }


    QJsonArray getInstalledSoftware()
    {
        QJsonArray softwares{};

#if defined (Q_OS_WIN)

        auto sws = CheckSoftwares::GetSoftwares();
        for(auto sw : sws)
        {
            softwares.append(cleanQString(sw.toString()));
        }

#elif defined (Q_OS_LINUX)

#define SHELLSCRIPT "\
#/bin/bash \n\
    find /usr/share/applications -type f -name \*.desktop | while read file; do\n\
    fullname=$(cat $file | grep FullName.en_ | sed -ne 's/^.*=//' -e '1 p')\n\
    name=$(cat $file | grep -i Name | sed -ne 's/^.*=//' -e '1 p')\n\
    echo name=$name\n\
    done\n\
    "
        auto res = QString(ExecShell(SHELLSCRIPT).c_str()).remove("\n");

        QStringList sw_list = res.split( "name=" );
        for(auto &sw: sw_list )
        {
            sw = cleanQString(sw);
            if(!sw.isEmpty() && !softwares.contains(sw) && !sw.startsWith("#"))
            {
                softwares.append(sw);
            }
        }

#elif defined (Q_OS_MAC)
        QString p_stdout{};
        QStringList pyCommand;
        pyCommand << "-xml" << "SPApplicationsDataType";

        QProcess _process{};
        _process.start("system_profiler", pyCommand);
        _process.waitForFinished();
        p_stdout = _process.readAllStandardOutput();
        _process.close();
        p_stdout = p_stdout.simplified();

        QDomDocument dom;
        QString error;

        int line, column;

        if(!dom.setContent(p_stdout, &error, &line, &column)) {
            return softwares;
        }

        QDomNodeList nodes = dom.elementsByTagName("dict");
        for(int i = 0; i < nodes.count(); i++)
        {
            QDomNode elm = nodes.at(i);
            if(elm.isElement())
            {
                QDomElement iChild = elm.firstChildElement("string");

                if (iChild.tagName().isEmpty())
                    break;

                softwares.append(cleanQString(iChild.text()));
            }
        }

#endif

        return softwares;
    }

    std::string get_IP_from_int(unsigned long int a) {
        std::string IP = "";
        for (int i = 0; i < 4; ++i)
        {
            IP = std::to_string(a & 0xFF) + IP;
            a = a >> 8;
            if (i < 3) {
                IP = "." + IP;
            }
        }
        return IP;
    }

    unsigned long int get_int_from_IP(std::string IP) {
        unsigned long int val = 0;
        size_t pos = 0;
        int count = 3;
        while ((pos = IP.find(".")) != std::string::npos) {
            int token = std::stoi(IP.substr(0, pos));
            val += (token << (8 * count));
            count--;
            IP.erase(0, pos + 1);
        }
        return val;
    }

    u_long get_ipmask(char *iface) {
        int sd;
        struct ifreq ifr;
        struct in_addr inaddr;

        if((sd=socket(AF_INET,SOCK_DGRAM,0))<0) {
            perror("socket");
            return 0;
        }
        bcopy(iface,ifr.ifr_name,IFNAMSIZ);
        if(ioctl(sd,SIOCGIFNETMASK,&ifr)<0) {
            perror("ioctl(SIOCGIFADDR)");
            return 0;
        }
#ifdef __linux__
        bcopy(&ifr.ifr_netmask.sa_data[2],&inaddr,sizeof(struct in_addr));
#elif defined(__OpenBSD__)||defined(__FreeBSD__)||defined(__NetBSD__)
        bcopy(&ifr.ifr_addr.sa_data[2],&inaddr,sizeof(struct in_addr));
#endif
        close (sd);
        return inaddr.s_addr;
    }

#if defined(Q_OS_WIN)

    bool isPortOpen(char *ip, int port)
    {
        //QCoreApplication::processEvents();

        struct timeval Timeout;
        Timeout.tv_sec  = 0;
        Timeout.tv_usec = 100000;

        struct sockaddr_in address;

        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        address.sin_addr.s_addr = inet_addr(ip);
        address.sin_port = htons(port);
        address.sin_family = AF_INET;

        //set the socket in non-blocking

        if (SetSocketBlockingEnabled(sock, false))
        {
            printf("ioctlsocket failed set the socket in non-blocking\n");
        }

        if(connect(sock,(struct sockaddr *)&address,sizeof(address))==false)
        {
            return false;
        }

        // restart the socket mode

        if (SetSocketBlockingEnabled(sock, true))
        {
            printf("ioctlsocket failed restart the socket mode\n");
        }

        fd_set Write, Err;
        FD_ZERO(&Write);
        FD_ZERO(&Err);
        FD_SET(sock, &Write);
        FD_SET(sock, &Err);

        // check if the socket is ready
        select(0,NULL,&Write,&Err,&Timeout);
        if(FD_ISSET(sock, &Write))
        {
            close(sock);
            return true;
        }
        close(sock);
        return false;
    }

    void getLoginName(char *username)
    {
        //char username[LOGIN_NAME_MAX];
        DWORD size = UNLEN + 1;
        GetUserNameA( username, &size );
    }

    void getOsInfo(char *osinfo)
    {
        int ret = 0.0;
        NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
        OSVERSIONINFOEXW osInfo;

        *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

        if (NULL != RtlGetVersion)
        {
            osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            RtlGetVersion(&osInfo);
            ret = osInfo.dwMajorVersion;

            sprintf(osinfo, "Microsoft Windows [Version %u.%u.%u]",
                    osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);
        }
    }

    QStringList getOpenPorts(char* ip)
    {
        QStringList result {};
        // Declare and initialize variables
        PMIB_TCPTABLE pTcpTable;
        DWORD dwSize = 0;
        DWORD dwRetVal = 0;

        char szLocalAddr[128];
        char szRemoteAddr[128];

        struct in_addr IpAddr;

        int i;

        pTcpTable = (MIB_TCPTABLE *) MALLOC(sizeof (MIB_TCPTABLE));
        if (pTcpTable == NULL) {
            printf("Error allocating memory\n");
            return result;
        }

        dwSize = sizeof (MIB_TCPTABLE);
        // Make an initial call to GetTcpTable to
        // get the necessary size into the dwSize variable
        if ((dwRetVal = GetTcpTable(pTcpTable, &dwSize, TRUE)) ==
                ERROR_INSUFFICIENT_BUFFER) {
            FREE(pTcpTable);
            pTcpTable = (MIB_TCPTABLE *) MALLOC(dwSize);
            if (pTcpTable == NULL) {
                printf("Error allocating memory\n");
                return result;
            }
        }
        // Make a second call to GetTcpTable to get
        // the actual data we require
        if ((dwRetVal = GetTcpTable(pTcpTable, &dwSize, TRUE)) == NO_ERROR) {
            for (i = 0; i < (int) pTcpTable->dwNumEntries; i++) {
                IpAddr.S_un.S_addr = (u_long) pTcpTable->table[i].dwLocalAddr;
                strcpy_s(szLocalAddr, sizeof (szLocalAddr), inet_ntoa(IpAddr));
                IpAddr.S_un.S_addr = (u_long) pTcpTable->table[i].dwRemoteAddr;
                strcpy_s(szRemoteAddr, sizeof (szRemoteAddr), inet_ntoa(IpAddr));

                auto sIp = QString(ip);
                auto sLocalIp = QString(szLocalAddr);
                switch (pTcpTable->table[i].dwState) {

                case MIB_TCP_STATE_LISTEN:

                    if (!sLocalIp.contains(QString("127.0.0.1")))
                    {
                        auto port = QString::number(ntohs((u_short)pTcpTable->table[i].dwLocalPort));
                        result.append(port);
                    }
                    break;
                case MIB_TCP_STATE_ESTAB:
                    break;
                default:
                    break;
                }

            }
        } else {
            printf("\tGetTcpTable failed with %d\n", dwRetVal);
            FREE(pTcpTable);
            return result;
        }

        if (pTcpTable != NULL) {
            FREE(pTcpTable);
            pTcpTable = NULL;
        }

        return result;
    }

#elif (defined (Q_OS_LINUX) || defined (Q_OS_MAC))

    static std::string ExecShell(const char* cmd) {
        char buffer[128];
        std::string result = "";
        FILE* pipe = popen(cmd, "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
        return result;
    }

    QStringList getOpenPorts(char* ip)
    {
        //QCoreApplication::processEvents();
        QStringList result {};
        QString  command("nmap");
        QString p_stdout{};
        QStringList attr;
        attr << "-sT" << "-oX" << "-" << ip ;

        QProcess _process{};
        _process.start(command, attr);
        _process.waitForFinished();
        p_stdout = _process.readAllStandardOutput();
        _process.close();

        if (p_stdout.size() == 0)
            return result;

        QDomDocument dom;

        if(!dom.setContent(p_stdout)) {
            return result;
        }

        QDomNodeList nodes = dom.elementsByTagName("port");
        for(int i = 0; i < nodes.count(); i++)
        {
            QDomNode elm = nodes.at(i);
            if(elm.isElement())
            {
                if( elm.hasAttributes() )
                {
                    QDomNamedNodeMap map = elm.attributes();
                    for( int i = 0 ; i < map.length() ; ++i )
                    {
                        if(!(map.item(i).isNull()))
                        {
                            QDomNode debug = map.item(i);
                            QDomAttr attr = debug.toAttr();
                            if(!attr.isNull())
                            {
                                if (attr.name().contains("portid"))
                                {
                                    result.append(attr.value());
                                }
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

    void getLoginName(char *username)
    {
        //char username[LOGIN_NAME_MAX];
        getlogin_r(username, LOGIN_NAME_MAX);
    }

    void getOsInfo(char *osinfo)
    {
        struct utsname details;

        int ret = uname(&details);

        if (ret == 0)
        {
            /*printf("sysname: %s\n", details.sysname);
            printf("nodename: %s\n", details.nodename);
            printf("release: %s\n", details.release);
            printf("version: %s\n", details.version);
            printf("machine: %s\n", details.machine);*/
            sprintf(osinfo, "%s %s",details.sysname, details.release);
        }
    }

    u_int getIpMask(char *iface) {
        int sd;
        struct ifreq ifr;
        struct in_addr inaddr;

        if((sd=socket(AF_INET,SOCK_DGRAM,0))<0) {
            perror("socket");
            return 0;
        }
        bcopy(iface,ifr.ifr_name,IFNAMSIZ);
        if(ioctl(sd,SIOCGIFNETMASK,&ifr)<0) {
            perror("ioctl(SIOCGIFADDR)");
            return 0;
        }

#if defined (Q_OS_LINUX)
        bcopy(&ifr.ifr_netmask.sa_data[2],&inaddr,sizeof(struct in_addr));
#elif defined (Q_OS_MAC)
        bcopy(&ifr.ifr_addr.sa_data[2],&inaddr,sizeof(struct in_addr));
#endif

        close (sd);
        return inaddr.s_addr;
    }

#endif
};
#endif // PCAPFUNCTIONS_H
