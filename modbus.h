#ifndef MODBUS_H
#define MODBUS_H

#include <QThread>
#include <QJsonObject>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>


/* Modbus protocol function codes */
#define READ_COILS                  1
#define READ_DISCRETE_INPUTS        2
#define READ_HOLDING_REGS           3
#define READ_INPUT_REGS             4
#define WRITE_SINGLE_COIL           5
#define WRITE_SINGLE_REG            6
#define READ_EXCEPT_STAT            7
#define DIAGNOSTICS                 8
#define GET_COMM_EVENT_CTRS         11
#define GET_COMM_EVENT_LOG          12
#define WRITE_MULT_COILS            15
#define WRITE_MULT_REGS             16
#define REPORT_SLAVE_ID             17
#define READ_FILE_RECORD            20
#define WRITE_FILE_RECORD           21
#define MASK_WRITE_REG              22
#define READ_WRITE_REG              23
#define READ_FIFO_QUEUE             24
#define ENCAP_INTERFACE_TRANSP      43
#define UNITY_SCHNEIDER             90


struct sniff_modbus_tcp{
    u_short transaction_id;
    u_short protocol_identifier;
    u_short lenght;
    u_char unit_identifier;
}__attribute__((packed));
struct sniff_modbus{
    u_char function_code;
}__attribute__((packed));

class Modbus
{
public:
    Modbus();
    ~Modbus();
    void processModbusPacket(const u_char*, int length, QJsonObject &modbus_data);
    QString analyzeModbusPacket(const u_char*, int length, QString id);

private:
    QStringList data{};
    quint16 m_lastAddress{};
};


#endif // MODBUS_H
