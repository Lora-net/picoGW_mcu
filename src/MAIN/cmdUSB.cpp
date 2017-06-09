/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2017 Semtech

*/

#include "CmdUSB.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "loragw_hal.h"
#include "loragw_reg.h"
#include "mbed.h"
#include "board.h"

/*
/Class INTERFACE definition
*/

INTERFACE::INTERFACE()
{
}

/*
/Class COMUSB definition
*/

COMUSB::COMUSB() : INTERFACE()
{
}
void COMUSB::Init() {
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
    MX_USB_DEVICE_Init();
}

void COMUSB::Receive(uint8_t * Buffer, uint32_t * size) {
    CDC_Receive_FSP(Buffer,size);// wait interrrupt manage in HAL_PCD_DataOutStageCallback
}

void COMUSB::Transmit(uint8_t * Buffer, uint16_t size) {
    if ((size % 64) == 0) { // for ZLP
        size = size + 1;
    } else {
        size = size;
    }
    while (CDC_Transmit_FS(Buffer, size) != USBD_OK){
    }
}

/*
/Class COMUART definition
*/

COMUART::COMUART(PinName Tx, PinName Rx) : Serial(Tx, Rx), INTERFACE()
{
    //do nothing
}

void COMUART::Init() {
    baud(BAUDRATE);
}

void COMUART::Receive(uint8_t * Buffer, uint32_t * size) {
    uint16_t localSize = 0;
    uint16_t cmdLength = 0;

    while (localSize < CMD_HEADER_RX_SIZE) {
        if (readable() == true) {
            Buffer[localSize]= getc();
            localSize++;
        }
    }
    localSize = 0;
    cmdLength = (Buffer[CMD_LENGTH_MSB] << 8) + Buffer[CMD_LENGTH_LSB];
    while (localSize < cmdLength){
        if (readable() == true) {
            Buffer[localSize + CMD_HEADER_RX_SIZE ]= getc();
            localSize++;
        }
    }
    *size = (uint32_t)(cmdLength + CMD_HEADER_RX_SIZE);
}

void COMUART::Transmit(uint8_t * Buffer, uint16_t size) {
    if ((size % 64) == 0) { // for ZLP Keep the same way than for USB transfer
        size = size + 1;
    } else {
        size = size;
    }
    for (int g = 0; g < size; g++){
        putc(Buffer[g]);
    }
} 

#ifdef  USE_UART 
CMDMANAGER::CMDMANAGER(PinName Tx, PinName Rx) 
{
    kill = false;
    ActiveInterface = (INTERFACE *) new COMUART (Tx, Rx);
    ActiveCom = ISUARTINTERFACE;
}
#else
CMDMANAGER::CMDMANAGER(PinName Tx, PinName Rx)
{
    kill = false;
    ActiveInterface = (INTERFACE *) new COMUSB ();
    ActiveCom = ISUSBINTERFACE;
}
#endif

void CMDMANAGER::InitBufFromHost() {
    memset(BufFromHost, 0, sizeof BufFromHost);
    memset(BufFromHostChunk, 0, sizeof BufFromHostChunk);
    receivelength[0] = 0;
    count = 1; 
}

void CMDMANAGER::InitBufToHost() {
    memset(BufToHost, 0, sizeof BufToHost);
}

void CMDMANAGER::Init() {
    ActiveInterface->Init();
}

void CMDMANAGER::ReceiveCmd (){
    InitBufFromHost();
    if (ActiveCom == ISUARTINTERFACE)
    {
        ActiveInterface->Receive(&BufFromHost[0], &receivelength[0]);
    } else {
        ActiveInterface->Receive(&BufFromHostChunk[0], &receivelength[0]);
        count = 1;
        while (count > 0) {// wait until receive cmd 
        }
        count = 1;
    }
    InitBufToHost();
}

void CMDMANAGER::TransmitCmd (){
    uint16_t size;
    size = (uint16_t)((BufToHost[CMD_LENGTH_MSB] << 8) + BufToHost[CMD_LENGTH_LSB] + CMD_HEADER_TX_SIZE);
    ActiveInterface->Transmit(BufToHost, size);
    /* Check if a reset has been requested */
    if (kill == true) {
        wait_ms(200);
        NVIC_SystemReset();
    }
}

/********************************************************/
/*   cmd name   |      description                      */
/*------------------------------------------------------*/
/*  r           |Read register                          */
/*  s           |Read long burst First packet           */
/*  t           |Read long burst Middle packet          */
/*  u           |Read long burst End packet             */
/*  p           |Read atomic burst packet               */
/*  w           |Write register                         */
/*  x           |Write long burst First packet          */
/*  y           |Write long burst Middle packet         */
/*  z           |Write long burst End packet            */
/*  a           |Write atomic burst packet              */
/*------------------------------------------------------*/
/*  b           |lgw_receive cmd                        */
/*  c           |lgw_rxrf_setconf cmd                   */
/*  d           |int lgw_rxif_setconf_cmd               */
/*  f           |int lgw_send cmd                       */
/*  h           |lgw_txgain_setconf                     */
/*  q           |lgw_get_trigcnt                        */
/*  i           |lgw_board_setconf                      */
/*  j           |lgw_mcu_commit_radio_calibration       */
/*  l           |lgw_check_fw_version                   */
/*  m           |Reset SX1308 and STM32                 */
/*  n           |Jump to bootloader                     */
/********************************************************/
int CMDMANAGER::DecodeCmd() {
    int i = 0;
    int adressreg;
    int val;
    int size;
    int x;
    CmdSettings_t cmdSettings_FromHost;

    if (BufFromHost[0] == 0) {
        return (CMD_ERROR);
    }

    cmdSettings_FromHost.id = BufFromHost[0];

    if (CheckCmd(cmdSettings_FromHost.id) == false) {
        BufToHost[0] = 'k';
        BufToHost[1] = 0;
        BufToHost[2] = 0;
        BufToHost[3] = ACK_K0;
        return(CMD_K0);
    }

    switch (cmdSettings_FromHost.id) {

        case 'r': { // cmd Read register
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                val = Sx1308.spiRead(adressreg);
                BufToHost[0] = 'r';
                BufToHost[1] = 0;
                BufToHost[2] = 1;
                BufToHost[3] = ACK_OK;
                BufToHost[4] = val;
                return(CMD_OK);
            }
        case 's': { // cmd Read burst register first
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                size = (cmdSettings_FromHost.cmd_data[0] << 8) + cmdSettings_FromHost.cmd_data[1];
                BufToHost[0] = 's';
                BufToHost[1] = cmdSettings_FromHost.cmd_data[0];
                BufToHost[2] = cmdSettings_FromHost.cmd_data[1];
                BufToHost[3] = ACK_OK;
                Sx1308.spiReadBurstF(adressreg, &BufToHost[4 + 0], size);
                return(CMD_OK);
            }
        case 't': { // cmd Read burst register middle
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];

                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                size = (cmdSettings_FromHost.cmd_data[0] << 8) + cmdSettings_FromHost.cmd_data[1];

                BufToHost[0] = 't';
                BufToHost[1] = cmdSettings_FromHost.cmd_data[0];
                BufToHost[2] = cmdSettings_FromHost.cmd_data[1];
                BufToHost[3] = ACK_OK;
                Sx1308.spiReadBurstM(adressreg, &BufToHost[4 + 0], size);
                return(CMD_OK);
            }
        case 'u': { // cmd Read burst register end
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                size = (cmdSettings_FromHost.cmd_data[0] << 8) + cmdSettings_FromHost.cmd_data[1];
                BufToHost[0] = 'u';
                BufToHost[1] = cmdSettings_FromHost.cmd_data[0];
                BufToHost[2] = cmdSettings_FromHost.cmd_data[1];
                BufToHost[3] = ACK_OK;
                Sx1308.spiReadBurstE(adressreg, &BufToHost[4 + 0], size);
                return(CMD_OK);
            }
        case 'p': { // cmd Read burst register atomic
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                size = (cmdSettings_FromHost.cmd_data[0] << 8) + cmdSettings_FromHost.cmd_data[1];
                BufToHost[0] = 'p';
                BufToHost[1] = cmdSettings_FromHost.cmd_data[0];
                BufToHost[2] = cmdSettings_FromHost.cmd_data[1];
                BufToHost[3] = ACK_OK;
                Sx1308.spiReadBurst(adressreg, &BufToHost[4 + 0], size);
                return(CMD_OK);
            }
        case 'w': { // cmd write register
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                val = cmdSettings_FromHost.cmd_data[0];
                Sx1308.spiWrite(adressreg, val);
                BufToHost[0] = 'w';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                return(CMD_OK);
            }
        case 'x': { // cmd write burst register
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                size = cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8);
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                Sx1308.spiWriteBurstF(adressreg, &cmdSettings_FromHost.cmd_data[0], size);
                BufToHost[0] = 'x';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                return(CMD_OK);
            }
        case 'y': { // cmd write burst register
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                size = cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8);
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                Sx1308.spiWriteBurstM(adressreg, &cmdSettings_FromHost.cmd_data[0], size);
                BufToHost[0] = 'y';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                return(CMD_OK);
            }
        case 'z': { // cmd write burst register
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                size = cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8);
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                Sx1308.spiWriteBurstE(adressreg, &cmdSettings_FromHost.cmd_data[0], size);
                BufToHost[0] = 'z';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                return(CMD_OK);
            }
        case 'a': { // cmd write burst atomic register
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                size = cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8);
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                Sx1308.spiWriteBurst(adressreg, &cmdSettings_FromHost.cmd_data[0], size);
                BufToHost[0] = 'a';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                return(CMD_OK);
            }
        case 'b': { // lgw_receive
                static struct lgw_pkt_rx_s pkt_data[16]; //16 max packets TBU
                int nbpacket = 0;
                int j = 0;
                int sizeatomic = sizeof(lgw_pkt_rx_s) / sizeof(uint8_t);
                int cptalc = 0;
                int pt = 0;
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                adressreg = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                nbpacket = lgw_receive(cmdSettings_FromHost.cmd_data[0], &pkt_data[0]);
                BufToHost[0] = 'b';
                BufToHost[3] = ((nbpacket >= 0) ? ACK_OK : ACK_K0); 
                BufToHost[4] = nbpacket;
                for (j = 0; j < nbpacket; j++) {
                    for (i = 0; i < (pkt_data[j].size + (sizeatomic - 256)); i++) {
                        BufToHost[5 + i + pt] = *((uint8_t *)(&pkt_data[j]) + i);
                        cptalc++;
                    }
                    pt = cptalc;
                }
                cptalc = cptalc + 1; // + 1 for nbpacket
                BufToHost[1] = (uint8_t)((cptalc >> 8) & 0xFF);
                BufToHost[2] = (uint8_t)((cptalc >> 0) & 0xFF);
                return(CMD_OK);
            }
        case 'c': { // lgw_rxrf_setconf
                uint8_t rf_chain;
                uint8_t conf[20];
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                rf_chain = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    conf[i] = BufFromHost[4 + i];
                }
                x = lgw_rxrf_setconf(rf_chain, *(lgw_conf_rxrf_s *)conf);
                BufToHost[0] = 'c';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ((x == 0) ? ACK_OK : ACK_K0);
                return(CMD_OK);
            }
        case 'h': { // lgw_txgain_setconf
                uint8_t conf[(LGW_MULTI_NB * TX_GAIN_LUT_SIZE_MAX) + 4];
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    conf[i] = BufFromHost[4 + i];
                }
                x = lgw_txgain_setconf((lgw_tx_gain_lut_s *)conf);
                BufToHost[0] = 'h';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ((x == 0) ? ACK_OK : ACK_K0);
                return(CMD_OK);
            }
        case 'd': { // lgw_rxif_setconf
                uint8_t if_chain;
                uint8_t conf[(sizeof(struct lgw_conf_rxif_s) / sizeof(uint8_t))];
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                if_chain = BufFromHost[3];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    conf[i] = BufFromHost[4 + i];
                }
                x = lgw_rxif_setconf(if_chain, *(lgw_conf_rxif_s *)conf);
                BufToHost[0] = 'd';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ((x == 0) ? ACK_OK : ACK_K0);
                return(CMD_OK);
            }
        case 'f': { // lgw_send
                uint32_t count_us;
                int32_t txcontinuous;
                uint8_t conf[(sizeof(struct lgw_pkt_tx_s) / sizeof(uint8_t))];
                Timer timer_tx_timeout;

                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    conf[i] = BufFromHost[4 + i];
                }

                /* Switch off SX1308 correlators to reduce power consumption during transmit */
#ifdef V2
                HSCLKEN = 0;
#else
                lgw_reg_w(LGW_CLKHS_EN, 0);
#endif

                /* Send packet */
                Sx1308.txongoing = 1;
                Sx1308.waittxend = 1;
                x = lgw_send(*(lgw_pkt_tx_s *)conf);
                if (x < 0) {
                    //pc.printf("lgw_send() failed\n");
                }
                
                /* In case of TX continuous, return the answer immediatly */
                lgw_reg_r(LGW_TX_MODE, &txcontinuous); // to switch off the timeout in case of tx continuous
                if (txcontinuous == 1) {
                    BufToHost[0] = 'f';
                    BufToHost[1] = 0;
                    BufToHost[2] = 0;
                    BufToHost[3] = ACK_OK;
                    return(CMD_OK);
                }

                /* Wait for TX_DONE interrupt, or 10 seconds timeout */
                timer_tx_timeout.reset();
                timer_tx_timeout.start();
                while (Sx1308.waittxend && (timer_tx_timeout.read() < (float)10.0)) {
                }
                timer_tx_timeout.stop();

                /* Align SX1308 internal counter and STM32 counter */
                if (Sx1308.firsttx == true) {
                    lgw_get_trigcnt(&count_us);
                    Sx1308.offtmstpstm32ref = (Sx1308.timerstm32ref.read_us() - count_us) + 60;
                    Sx1308.firsttx = false;
                }

                /* reset Sx1308 */
                Sx1308.dig_reset();

                /* Switch SX1308 correlators back on  */
#ifdef V2
                HSCLKEN = 1;
#else
                lgw_reg_w(LGW_CLKHS_EN, 1);
#endif

                /* restart SX1308 */
                x = lgw_start();
                if (x < 0) {
                    //pc.printf("lgw_start() failed\n");
                }

                /* Send command answer */
                BufToHost[0] = 'f';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ((x == 0) ? ACK_OK : ACK_K0);

                return(CMD_OK);
            }
        case 'q': { // lgw_get_trigcnt
                uint32_t timestamp;
                x = lgw_get_trigcnt(&timestamp);
                timestamp += Sx1308.offtmstpstm32;
                BufToHost[0] = 'q';
                BufToHost[1] = 0;
                BufToHost[2] = 4;
                BufToHost[3] = ((x == 0) ? ACK_OK : ACK_K0);
                BufToHost[4] = (uint8_t)(timestamp >> 24);
                BufToHost[5] = (uint8_t)((timestamp & 0x00FF0000) >> 16);
                BufToHost[6] = (uint8_t)((timestamp & 0x0000FF00) >> 8);
                BufToHost[7] = (uint8_t)((timestamp & 0x000000FF));
                return(CMD_OK);
            }
        case 'i': { // lgw_board_setconf
                uint8_t  conf[(sizeof(struct lgw_conf_board_s) / sizeof(uint8_t))];
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    conf[i] = BufFromHost[4 + i];
                }

                x = lgw_board_setconf(*(lgw_conf_board_s *)conf);
                BufToHost[0] = 'i';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ((x == 0) ? ACK_OK : ACK_K0);
                return(CMD_OK);
            }
        case 'j': { // lgw_calibration_snapshot
                lgw_calibration_offset_transfer(BufFromHost[4], BufFromHost[5]);
                BufToHost[0] = 'j';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                return(CMD_OK);
            }
        case 'l': { // lgw_mcu_commit_radio_calibration
                int fwfromhost;
                cmdSettings_FromHost.len_msb = BufFromHost[1];
                cmdSettings_FromHost.len_lsb = BufFromHost[2];
                for (i = 0; i < cmdSettings_FromHost.len_lsb + (cmdSettings_FromHost.len_msb << 8); i++) {
                    cmdSettings_FromHost.cmd_data[i] = BufFromHost[4 + i];
                }
                fwfromhost = (BufFromHost[4] << 24) + (BufFromHost[5] << 16) + (BufFromHost[6] << 8) + (BufFromHost[7]);
                BufToHost[0] = 'l';
                BufToHost[1] = 0;
                BufToHost[2] = 8;
                if (fwfromhost == FWVERSION) {
                    BufToHost[3] = ACK_OK;
                } else {
                    BufToHost[3] = ACK_K0;
                }
                BufToHost[4] = *(uint8_t *)0x1fff7a18;   //unique STM32 register base adresse
                BufToHost[5] = *(uint8_t *)0x1fff7a19;
                BufToHost[6] = *(uint8_t *)0x1fff7a1a;
                BufToHost[7] = *(uint8_t *)0x1fff7a1b;
                BufToHost[8] = *(uint8_t *)0x1fff7a10;
                BufToHost[9] = *(uint8_t *)0x1fff7a11;
                BufToHost[10] = *(uint8_t *)0x1fff7a12;
                BufToHost[11] = *(uint8_t *)0x1fff7a13;
                return(CMD_OK);
            }
        case 'm': { // Reset SX1308 and STM32
                /* reset SX1308 */
                lgw_soft_reset();
                /* Prepare command answer */
                BufToHost[0] = 'm';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                /* Indicate that STM32 reset has to be triggered */
                kill = true;
                return(CMD_OK);
            }
        case 'n': { // Jump to bootloader to allow reflashing (DFU, UART bootloader...)
                FLASH_Prog(DATA_EEPROM_BASE, GOTO_BOOTLOADER);
                BufToHost[0] = 'n';
                BufToHost[1] = 0;
                BufToHost[2] = 0;
                BufToHost[3] = ACK_OK;
                kill = true;
                return(CMD_OK);
            }
        default:
            BufToHost[0] = 'k';
            BufToHost[1] = 0;
            BufToHost[2] = 0;
            BufToHost[3] = ACK_K0;
            return(CMD_K0);
    }
}

bool CMDMANAGER::CheckCmd(char id) {
    switch (id) {
        case 'r': /* read register */
        case 's': /* read burst - first chunk */
        case 't': /* read burst - middle chunk */
        case 'u': /* read burst - end chunk */
        case 'p': /* read burst - atomic */
        case 'w': /* write register */
        case 'x': /* write burst - first chunk */
        case 'y': /* write burst - middle chunk */
        case 'z': /* write burst - end chunk */
        case 'a': /* write burst - atomic */
        case 'b': /* lgw_receive */
        case 'c': /* lgw_rxrf_setconf */
        case 'd': /* lgw_rxif_setconf */
        case 'f': /* lgw_send */
        case 'h': /* lgw_txgain_setconf */
        case 'q': /* lgw_get_trigcnt */
        case 'i': /* lgw_board_setconf */
        case 'j': /* lgw_mcu_commit_radio_calibration */
        case 'l': /* lgw_check_fw_version */
        case 'm': /* reset STM32 */
        case 'n': /* Go to Bootloader */
            return true;
        default:
            return false;
    }
}

int CMDMANAGER::Convert2charsToByte(uint8_t a, uint8_t  b) {
    if (a > 96) {
        a = a - 87;
    } else {
        a = a - 48;
    }
    if (b > 96) {
        b = b - 87;
    } else {
        b = b - 48;
    }
    return(b + (a << 4));
}
