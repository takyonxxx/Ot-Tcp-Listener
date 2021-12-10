#include "modbus.h"

Modbus::Modbus()
{
}

Modbus::~Modbus()
{
}

QString Modbus::analyzeModbusPacket(const u_char* packet, int length, QString id)
{
    if (length < 5)
    {
        //qWarning() << "Packet too short:" << length << "bytes";
        return QString();
    }

    int function = packet[0];
    QString functionName;

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

    QStringList reply{};
    reply.append("Function:" + functionName);


    bool exception = function & 128;

    if (exception)
    {
    }
    else
    {
        if (length < 8)
        {
            return QString();
        }

        int address = (packet[1] << 8 | packet[2]);
        int points = (packet[3] << 8 | packet[4]);

        if (function == 3 || function == 4)
        {
            if (length > 8)
            {
                for (int i = 0; i < packet[1] / 2; i++)
                {
                    quint16 value = ((packet[2 + i * 2] << 8) | packet[2 + i * 2 + 1]);
                    reply.append(QString::number(m_lastAddress + i) + ":" + QString::number((qint16) value));
                }
            }
            else
            {
                reply.append("Start:" + QString::number(address) + " Length:" + QString::number(points));
                m_lastAddress = address;
            }
        }
        else if (function == 8)
        {
            quint16 subfunction = ((packet[1] << 8) | packet[2]);
            reply.append("Subfunction:" + QString::number(subfunction) + " Length:" + QString::number(length));
        }
        else if (function == 16)
        {
            if (length > 8)
            {
                for (int i = 0; i < packet[5] / 2; i++)
                {
                    quint16 value = ((packet[6 + i * 2] << 8) | packet[6 + i * 2 + 1]);
                    reply.append(QString::number(address + i) + ":" + QString::number((qint16) value));
                }
            }
        }

        return reply.join(";");
    }
}


void Modbus::processModbusPacket(const u_char* packet, int length, QJsonObject &modbus_data)
{
    auto modbus_tcp = (struct sniff_modbus_tcp*) (packet);
    auto modbus_tcp_lenght = htons(modbus_tcp->lenght);
    auto modbus_transaction_id = QString::number(htons(modbus_tcp->transaction_id));

    int remainig_len = length - ( modbus_tcp_lenght + sizeof(struct sniff_modbus_tcp) -1);

    if(remainig_len < 0)
        return;

    int offset =  sizeof(struct sniff_modbus_tcp);
    auto temp=(u_char*)malloc(modbus_tcp_lenght-1);
    for (int i = 0; i < modbus_tcp_lenght-1; ++i)
    {
        temp[i] = (packet + offset)[i];
    }

    auto databuf = QByteArray((char*)temp, modbus_tcp_lenght - 1);
    auto reply = analyzeModbusPacket(temp, modbus_tcp_lenght-1, modbus_transaction_id);

    data.append(QString("Transaction Id:%1").arg(modbus_transaction_id));
    data.append(QString("Lenght:%1").arg( QString::number(modbus_tcp_lenght - 1)));
    data.append(QString("Hex:%1").arg( QString(databuf.toHex(':'))));
    if(!reply.isEmpty())
        data.append(reply);

    if(remainig_len > 0)
    {
        int offset =  modbus_tcp_lenght + sizeof(struct sniff_modbus_tcp) -1;
        unsigned char* remainig=(unsigned char*)malloc( remainig_len);
        for (int i = 0; i < remainig_len; ++i)
        {
            remainig[i] = (packet + offset)[i];
        }
        processModbusPacket(remainig, remainig_len, modbus_data);
    }
    else
    {
        modbus_data["modbus"] = data.join(";");
        data.clear();
    }
}
