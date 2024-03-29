#include "sniffer.h"
#include "logger.h"

Sniffer::Sniffer(QObject *parent, otclient& client)
    : QThread(parent), _client(client)
{
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

void Sniffer::processIpPacket(struct pcap_pkthdr const* header, const u_char* packet)
{
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
        //memcpy(payload, packet + offset, payload_len);
        for (int i = 0; i < payload_len; ++i)
        {
            payload[i] = (packet + offset)[i];
        }

        QString packet_type{};

        if (srcPort == PORT_MBTCP  && dstPort!=PORT_MBTCP )
            packet_type = "Modbus Response Packet";
        else if ( srcPort != PORT_MBTCP  &&  dstPort == PORT_MBTCP)
            packet_type = "Modbus Query Packet";

        _data= QJsonObject();
        _data["srcIp"] = srcIp;
        _data["dstIp"] = dstIp;
        _data["srcMac"] = srcMac;
        _data["dstMac"] = dstMac;
        _data["srcPort"] = QString::number(srcPort);
        _data["dstPort"] = QString::number(dstPort);
        _data["packet_type"] = packet_type;
        _data["packet_len"] = QString::number(packet_len);

        Modbus modbus{};
        QJsonObject modbus_data{};
        modbus.processModbusPacket(payload, payload_len, modbus_data);

        QJsonObject j_data(_data);
        for (auto it = modbus_data.constBegin(); it != modbus_data.constEnd(); it++) {
            j_data.insert(it.key(), it.value());
        }

        qDebug() << "Source Ip" << "\t" << j_data.value("srcIp").toString();
        qDebug() << "Dest Ip" << "\t" << j_data.value("dstIp").toString();
        qDebug() << "Source Mac" << "\t" << j_data.value("srcMac").toString();
        qDebug() << "Dest Mac" << "\t" << j_data.value("dstMac").toString();
        qDebug() << "Source Port" << "\t" << j_data.value("srcPort").toString();
        qDebug() << "Dest Port" << "\t" << j_data.value("dstPort").toString();
        qDebug() << "Packet Type" << "\t" << j_data.value("packet_type").toString();
        qDebug() << "Packet Lenght" << "\t" << j_data.value("packet_len").toString();
        //qDebug() << "Modbus" << "\t" << j_data.value("modbus").toString();

        auto splitted = j_data.value("modbus").toString().split(";");
        foreach(const QString& value, splitted)
        {
            qDebug() << "\t" << value;
        }
        qDebug() << "";
    }

}

void Sniffer::processArpPacket(struct pcap_pkthdr const* header, const u_char* packet)
{
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
