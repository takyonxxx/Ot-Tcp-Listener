#include "sniffer.h"
#include "logger.h"
#include "pythonscript.h"

Sniffer::Sniffer(QObject *parent, otclient& client, QString &id, QString &mode)
    : QThread(parent), _client(client), m_id(id), m_mode(mode)
{
    pythonScript = PythonScript::getInstance();

    if(segmentIpList.length() == 0)
    {
        char* mask = (char *)"255.255.255.0";
        std::string s = _client.ifaceIp.toStdString();
        char *c_ip = const_cast<char*>(s.c_str());
        struct in_addr ipaddress, subnetmask;

        inet_pton(AF_INET, c_ip, &ipaddress);
        inet_pton(AF_INET, mask, &subnetmask);

        unsigned long first_ip = ntohl(ipaddress.s_addr & subnetmask.s_addr);
        unsigned long last_ip = ntohl(ipaddress.s_addr | ~(subnetmask.s_addr));

        for (unsigned long ip = first_ip; ip <= last_ip; ++ip) {
            unsigned long long_address = htonl(ip);
            struct in_addr addr;
            addr.s_addr = long_address;
            auto sip = inet_ntoa(addr);
            segmentIpList.append(sip);
        }

        //pthread_create( &snifferSendArpThread, nullptr, &Sniffer::arpLoop, this);
    }

    //std::string filter_exp = "tcp and (tcp[13] == 18 or tcp[13] == 2)";
    std::string filter_exp = "tcp";
    StartPcap();
    SetPcapFilter(filter_exp);
    Logger::getInstance()->write(QString("New Adapter [%1] added to sensor.").arg("mirror"));
}

Sniffer::~Sniffer()
{
    m_stop = true;
}

void Sniffer::stopLoop()
{
    m_stop = true;
}

void Sniffer::run()
{
    if(!handle)
        return;

    while (true)
    {
        if (m_stop)
            break;

        if(handle)
        {
            MonitorPcap(handle);
        }

        msleep(SnifferGetPacketPeriod);
    }
}

void Sniffer::StartPcap()
{
    // Create a packet capture handle for the specified interface
    handle = pcap_open_live(_client.iface.toStdString().c_str() , ETHER_MAX_LEN , 1 , 1000 , error_buffer);//pcap_create( device, error_buffer );
    if( handle == NULL )
    {
        fprintf( stderr, "Unable to create pcap for interface %s (%s).\n", _client.iface.toStdString().c_str(), error_buffer );
        exit(EXIT_FAILURE);
    }

    // Set non-blocking
    if( pcap_setnonblock(handle, 1, error_buffer)!= 0 )
    {
        fprintf(stderr, "Error placing pcap interface in non-blocking mode.\n");
        exit(EXIT_FAILURE);
    }

    // Set ethernet link-layer header type
    if( pcap_set_datalink( handle, DLT_EN10MB ) )
    {
        pcap_perror( handle, "Set datalink failed" );
        exit(EXIT_FAILURE);
    }

    //fprintf(stderr, "Sniffer started with device %s\n", _client.iface.toStdString().c_str());
    Logger::getInstance()->write(QString("Sniffer started with device %1").arg (_client.iface));
}

void Sniffer::SetPcapFilter(std::string filter_exp)
{
    u_long ipmask;
    if((ipmask=networkTools.get_ipmask((char*)_client.iface.toStdString().c_str()))==0)
    {
        fprintf(stderr,"Can not get ip mask\n");
        exit(EXIT_FAILURE);
    }

    //char filter_exp[] = "port 10515";

    struct bpf_program pgm;
    if (pcap_compile(handle, &pgm, filter_exp.c_str(), 0, static_cast<bpf_u_int32>(ipmask)) == -1)
    {
        fprintf(stderr,"Bad filter - %s\n", pcap_geterr(handle));
        exit(EXIT_FAILURE);
    }

    if (pcap_setfilter(handle, &pgm) == -1)
    {
        fprintf(stderr,"Error setting filter - %s\n", pcap_geterr(handle));
        exit(EXIT_FAILURE);
    }

    //fprintf(stderr, "Set filter [%s] for Sniffer with device %s\n", filter_exp.c_str(), _client.iface.toStdString().c_str());
    Logger::getInstance()->write(QString("Set filter [%1] for Sniffer with device %2").arg (filter_exp.c_str()).arg(_client.iface));
}

void Sniffer::MonitorPcap( pcap_t * pcap )
{
    if(m_stop)
        return;

    struct pcap_pkthdr * packetHeader;
    const u_char * packet;
    int packetCount = pcap_next_ex( pcap, &packetHeader, &packet );

    if( packetCount < 0 )
        return;

    if( packetHeader->caplen >= sizeof( struct ether_header ) )
    {
        struct sniff_ethernet *eth_header{};

        //the ethernet header is always the same (14 bytes)
        eth_header = (struct sniff_ethernet *) packet;
        auto type = ntohs(eth_header->ether_type);
        QString hexType = QString("%1").arg(type, 4, 16, QLatin1Char( '0' ));

        if(type == ETHERTYPE_IP)//ip
        {
            processIpPacket(packetHeader, packet);
        }
        else if(type == ETHERTYPE_ARP)//arp layer-2
        {
            processArpPacket(packetHeader, packet);
        }
    }
}

void Sniffer::analyzeModbusPacket(const u_char* packet, int length)
{
    if (length < 5)
    {
        //qWarning() << "Packet too short:" << length << "bytes";
        return;
    }

    int function = packet[0];
    QString functionName;

    QDebug debug = qDebug();
    debug.noquote();

    switch (function & 127)
    {
    case 1:
        functionName = "Read Coil Status";
        break;

    case 2:
        functionName = "Read Input Status";
        break;

    case 3:
        functionName = "Read Holding Registers";
        break;

    case 4:
        functionName = "Read Input Registers";
        break;

    case 5:
        functionName = "Force Single Coil";
        break;

    case 6:
        functionName = "Preset Single Register";
        break;

    case 8:
        functionName = "Diagnostics";
        break;

    case 15:
        functionName = "Force Multiple Coils";
        break;

    case 16:
        functionName = "Preset Multiple Registers";
        break;

    default:
        functionName = "Unknown function";
        break;
    }

    bool exception = function & 128;

    if (exception)
    {
        debug << "Execption reply for " + functionName;
    }
    else
    {
        if (length < 8)
        {
            //qWarning() << "Packet too short:" << length << "bytes";
            return;
        }

        int address = (packet[1] << 8 | packet[2]);
        int points = (packet[3] << 8 | packet[4]);

        if (function == 3 || function == 4)
        {
            if (length > 8)
            {
                debug << functionName + "\nReply\nData:\n";

                for (int i = 0; i < packet[1] / 2; i++)
                {
                    quint16 value = ((packet[2 + i * 2] << 8) | packet[2 + i * 2 + 1]);
                    debug << "" << (m_lastAddress + i) << ":" << (quint16) value << "(" << (qint16) value << ")\n";
                }
            }
            else
            {
                debug << functionName + "\n";
                debug << "  Start:" << address << "\n  Length:" << points;
                m_lastAddress = address;
            }
        }
        else if (function == 8)
        {
            debug << functionName + "\n";
            quint16 subfunction = ((packet[1] << 8) | packet[2]);
            debug << "  Subfunction:" << subfunction << "\n. Length:" << length;

        }
        else if (function == 16)
        {
            if (length > 8)
            {
                debug << functionName + "\n";
                debug << "  Start:" << address << "\n  Length:" << points << "\nData:\n";

                for (int i = 0; i < packet[5] / 2; i++)
                {
                    quint16 value = ((packet[6 + i * 2] << 8) | packet[6 + i * 2 + 1]);
                    debug << "" << (address + i) << ":" << (quint16) value << "(" << (qint16) value << ")\n";
                }
            }
            else
            {
                debug << functionName + " Reply OK\n";
            }
        }
        else
        {
            debug << functionName;
        }
    }
}


void Sniffer::processModbusPacket(const u_char* packet, int length)
{
    auto modbus_tcp = (struct sniff_modbus_tcp*) (packet);
    auto modbus_tcp_lenght = htons(modbus_tcp->lenght);

    int remainig_len = length - ( modbus_tcp_lenght + sizeof(struct sniff_modbus_tcp) -1);

    if(remainig_len < 0)
        return;

    int offset =  sizeof(struct sniff_modbus_tcp);
    auto temp=(u_char*)malloc(modbus_tcp_lenght-1);
    //memcpy(temp, packet + offset, modbus_tcp_lenght-1);
    for (int i = 0; i < modbus_tcp_lenght-1; ++i)
    {
        temp[i] = (packet + offset)[i];
    }
    /*for(int i=0; i<modbus_tcp_lenght-1; i++) {
        fprintf(stdout, "%02x ",temp[i]);
    }
    puts("");*/

    analyzeModbusPacket(temp, modbus_tcp_lenght-1);

    if(remainig_len > 0)
    {
        int offset =  modbus_tcp_lenght + sizeof(struct sniff_modbus_tcp) -1;
        unsigned char* remainig=(unsigned char*)malloc( remainig_len);
        //memcpy(remainig, packet + offset, remainig_len);
        for (int i = 0; i < remainig_len; ++i)
        {
            remainig[i] = (packet + offset)[i];
        }
        processModbusPacket(remainig, remainig_len);
    }
}

void Sniffer::processIpPacket(struct pcap_pkthdr const* header, const u_char* packet)
{    
    QMutexLocker locker(mutex);
    struct sniff_ethernet *eth_header{};

    //the ethernet header is always the same (14 bytes)
    eth_header = (struct sniff_ethernet *) packet;

    //internet protocol
    auto ip = (struct sniff_ip*)(packet + sizeof(struct ether_header));

    auto size_ip = IP_HL(ip)*4;
    if (size_ip < 20) {
        printf("   * Invalid IP header length: %u bytes\n", size_ip);
        return;
    }

    auto srcIp = QString(inet_ntoa(ip->ip_src));
    auto dstIp = QString(inet_ntoa(ip->ip_dst));

    auto srcMac = QString(ether_ntoa((const struct ether_addr*)eth_header->ether_shost));
    auto dstMac = QString(ether_ntoa((const struct ether_addr*)eth_header->ether_dhost));

    //transmission control protocol
    auto tcp = (struct sniff_tcp*)(packet + sizeof(struct ether_header) + size_ip);
    //auto tcp = (struct tcphdr*) (packet + sizeof(struct ether_header)+sizeof(struct ip));

    auto size_tcp = TH_OFF(tcp)*4;
    if (size_tcp < 20) {
        printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
        return;
    }

    //The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.
    auto srcPort = htons(tcp->th_sport);
    auto dstPort = htons(tcp->th_dport);
    int packet_len = header->len;
    int payload_len = packet_len - (sizeof(struct ether_header) + size_ip + size_tcp);

    // modbus
    if ((dstPort==PORT_MBTCP || srcPort == PORT_MBTCP) && payload_len > 0)
    {

        int offset = sizeof(struct ether_header) + size_ip + size_tcp;
        unsigned char* payload=(unsigned char*)malloc(payload_len);
        memcpy(payload, packet + offset, payload_len);

        QString packet_type{};

        if (srcPort == PORT_MBTCP  && dstPort!=PORT_MBTCP )
            packet_type = "MODBUS_RESPONSE_PACKET";
        else if ( srcPort != PORT_MBTCP  &&  dstPort == PORT_MBTCP)
            packet_type = "MODBUS_QUERY_PACKET";

        processModbusPacket(payload, payload_len);
    }

}

void Sniffer::processArpPacket(struct pcap_pkthdr const* header, const u_char* packet)
{
    QMutexLocker locker(mutex);

    struct ether_arp * arp_packet{};

    arp_packet = (struct ether_arp *)(packet + sizeof(struct ether_header));
    struct arphdr arpheader = arp_packet->ea_hdr;
    auto type = arpheader.ar_op;


    if(ntohs(type) == ARP_REQUEST || ntohs(type) == ARP_REPLY)
    {
        if (ntohs(arpheader.ar_pro) != ETHERTYPE_IP) //? "IPv4
            return;

        auto sip = std::to_string(arp_packet->arp_spa[0]) + "."
                + std::to_string(arp_packet->arp_spa[1]) + "."
                + std::to_string(arp_packet->arp_spa[2]) + "."
                + std::to_string(arp_packet->arp_spa[3]);

        auto smac = intToHexString(arp_packet->arp_sha[0]) + ":" +
                intToHexString(arp_packet->arp_sha[1]) + ":" +
                intToHexString(arp_packet->arp_sha[2]) + ":" +
                intToHexString(arp_packet->arp_sha[3]) + ":" +
                intToHexString(arp_packet->arp_sha[4]) + ":" +
                intToHexString(arp_packet->arp_sha[5]);

        auto srcIp = QString(sip.c_str());
        auto srcMac = QString(smac.c_str());

        if(validateIP(srcIp.toStdString()) && _client.ifaceIp != srcIp)
        {
            asset m_asset{};
            QString hostname{};
            QJsonObject vendor{};

            m_asset.ip = srcIp;
            m_asset.mac = srcMac;
            m_asset.adapterId = m_id;
            m_asset.adapterMode = m_mode;
            m_asset.eventType = "arp packet";

            auto it = asset_hostnames.find(m_asset.ip);
            if (it == asset_hostnames.end())
            {
                auto hostname = QString(networkTools.getHostName(m_asset.ip));
                if(!hostname.isEmpty())
                {
                    asset_hostnames.insert (std::pair<QString, QString>(m_asset.ip, hostname));
                    m_asset.hostname = hostname;
                }
            }
            else
            {
                hostname = it->second;
            }

            it = asset_vendors.find(m_asset.ip);
            if (it == asset_vendors.end())
            {
                std::string sVendor;

                networkTools.getVendor(m_asset.mac.toStdString(), sVendor);
                if(!sVendor.empty() && !QString(sVendor.c_str()).contains("errors"))
                {
                    vendor[m_asset.mac] = sVendor.c_str();
                    m_asset.nmap["vendor"] = vendor;
                    asset_vendors.insert (std::pair<QString, QString>(m_asset.ip, sVendor.c_str()));
                }
            }
            else
            {
                vendor[m_asset.mac] = it->second;
                m_asset.nmap["vendor"] = vendor;
            }

            MqttPublisher::getInstance()->update_asset(m_asset);
        }
    }
}

void *Sniffer::arpLoop(void * this_ptr)
{
    Sniffer* obj_ptr = static_cast<Sniffer*>(this_ptr);

    while (true)
    {
        if (obj_ptr->m_stop) break;

        //sudo tcpdump -nni enp4s0 arp

        for (int x = 0; x <= obj_ptr->segmentIpList .count()-1; x++)
        {
            auto ip = obj_ptr->segmentIpList [x];
            if (!validateIP(ip.toStdString())) continue;
            obj_ptr->arp_manager.sendArpPacket((char*)obj_ptr->_client.ifaceIp.toStdString().c_str(),
                                               (char*)ip.toStdString().c_str(),
                                               (char*)obj_ptr->_client.iface.toStdString().c_str());
        }
        Logger::getInstance()->write(QString("Send arp packet ..."));
        sleep(SnifferSendArpPeriod);
    }
    return 0;
}
