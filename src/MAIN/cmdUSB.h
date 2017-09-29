/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2017 Semtech
*/

#ifndef USBMANAGER_H
#define USBMANAGER_H

#include "mbed.h"
#include "SX1308.h"

#define ATOMICTX 900
#define ATOMICRX 600

#define CMD_HEADER_RX_SIZE 4 /* id + len_msb + len_lsb + address */
#define CMD_HEADER_TX_SIZE 4 /* id + len_msb + len_lsb + status */

#define CMD_DATA_RX_SIZE ATOMICRX
#define CMD_DATA_TX_SIZE (1024 + 16 * 44) /* MAX_FIFO + 16 * METADATA_SIZE_ALIGNED */
#define CMD_LENGTH_MSB     1
#define CMD_LENGTH_LSB     2

#define CMD_ERROR          0
#define CMD_OK             1
#define CMD_K0             0
#define ACK_OK             1
#define ACK_K0             0
#define FWVERSION          0x010a0005
#define ISUARTINTERFACE    1
#define ISUSBINTERFACE     0

#define BAUDRATE 115200

typedef struct {
    char id;
    uint8_t len_msb;
    uint8_t len_lsb;
    uint8_t address;
    uint8_t cmd_data[CMD_DATA_RX_SIZE];
} CmdSettings_t;


class INTERFACE {

public:
    INTERFACE();
    virtual void Init() =0;
    virtual void Receive(uint8_t* Buffer, uint32_t* size) = 0;
    virtual void Transmit(uint8_t* Buffer, uint16_t size) = 0; 
private:

};

class COMUSB : INTERFACE {
public:
    // public methods
    COMUSB();
    virtual void Init();
    virtual void Receive(uint8_t* Buffer, uint32_t* size);
    virtual void Transmit(uint8_t* Buffer, uint16_t size); 
private:
    // private methods
};

class COMUART : public Serial, public INTERFACE{

public:
    // public members
    COMUART(PinName Tx, PinName Rx);
    virtual void Init();
    virtual void Receive(uint8_t* Buffer, uint32_t* size);
    virtual void Transmit(uint8_t* Buffer, uint16_t size); 
private:
    // private methods
};

class CMDMANAGER {

public:
    // public members
    uint8_t BufFromHost[CMD_DATA_RX_SIZE + CMD_HEADER_RX_SIZE];
    uint8_t BufFromHostChunk[64];
    uint8_t BufToHost[CMD_DATA_TX_SIZE + CMD_HEADER_TX_SIZE];
    uint32_t receivelength[5];
    volatile uint32_t count;
    int ActiveCom;
    bool kill;
    INTERFACE * ActiveInterface;

    // public methods
    CMDMANAGER(PinName Tx, PinName Rx);
    void Init();
    void ReceiveCmd();
    void TransmitCmd();
    void InitBufFromHost();
    void InitBufToHost();
    int DecodeCmd();
    bool CheckCmd(char id);

private:
    // private methods
    int Convert2charsToByte(uint8_t a, uint8_t b);
};

extern CMDMANAGER CmdManager;
#endif
