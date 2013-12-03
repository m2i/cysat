/*
 * File:   mem.c
 * Author: Jake Drahos
 *
 *
 * Source file for memory management.
 *
 * These functions allow access to PPM flash and FRAM over SPI
 *
 * Created on November 11, 2013, 5:43 PM
 */



#include <FreeRTOS.h>
#include <task.h>
#include <system.h>
#include <PPS.h>
#include <mem.h>
#include <pic24.h>
#include <spi.h>
#include <uart.h>
#include <system.h>
/**********************************
 *
 * Register Defines
 *
 * These define the TX/RX Register to use with SPI for each memory module
 *
 * These are based off of the PPM doc and the pinout on the CySat website
 *
 *    - FRAM is on SPI1
 *    - PPM FLASH is on SPI3
 *
 ***********************************/

//NOTE: THESE ARE WEIRD. I NEED TO DO POINTER STUFFS FOR THEM TO WORK
//OR MAKE MACROS. PROBABLY THE MACROS

#define FRAM_SPI_BUF SPI1BUF
#define FRAM_SPI_CON1 SPI1CON1
#define FRAM_SPI_CON2 SPI1CON2
#define FRAM_SPI_STAT SPI1STAT

#define FRAM_SPI_STATbits SPI1STATbits
#define FRAM_SPI_CON1bits SPI1CON1bits
#define FRAM_SPI_CON2bits SPI1CON2bits

#define FLASH_SPI_BUF SPI3BUF
#define FLASH_SPI_CON1 SPI3CON1
#define FLASH_SPI_CON2 SPI3CON2
#define FLASH_SPI_STAT SPI3STAT

#define FLASH_SPI_STATbits SPI3STATbits
#define FLASH_SPI_CON1bits SPI3CON1bits
#define FLASH_SPI_CON2bits SPI3CON2bits

#define SPI_MASTER 0x0120	// select 8-bit master mode, CKE=1, CKP=0
/********************************
 * Control Pin Defines
 *
 * These set the /CS pins etc.
 *    - FLASH /CS on RD13
 *    - FLASH /WP on RD6
 *
 *    - FRAM1 /CS on RP22 = RD3
 *    - FRAM1 /WP on RP2  = RD8
 */
#define FLASH_CS_TRIS TRISDbits.TRISD13
#define FLASH_CS LATDbits.LATD13
#define FLASH_WP_TRIS TRISDbits.TRISD6
#define FLASH_WP LATDbits.LATD6

#define FRAM1_CS_TRIS TRISDbits.TRISD3
#define FRAM1_CS LATDbits.LATD3
#define FRAM1_WP_TRIS TRISDbits.TRISD8
#define FRAM1_WP LATDbits.LATD8

/********************************
 * PPS Mapping
 *
 * Make any changes at top of vSetupMem
 *      These are assuming "5th Board Mem" refers to the Aux. Board FRAM
 *    - SDO1 is on RP19 = RG8
 *    - SDI1 is on RP26 = RG7
 *    - SCK1OUT is on RP21 = RG6
 *
 *    - SDO3 is on RP11 = RD0
 *    - SDI3 is on RP3 = RD10
 *    - SCK3OUT is on RP12 = RD11
 */
#define FLASH_SDO LATDbits.LATD0
#define FLASH_SDI PORTDbits.RD10
#define FLASH_SCK LATDbits.LATD11
#define FLASH_SDO_TRIS TRISDbits.TRISD0
#define FLASH_SDI_TRIS TRISDbits.TRISD10
#define FLASH_SCK_TRIS TRISDbits.TRISD11

#define FRAM_SDO LATGbits.LATG8
#define FRAM_SDI PORTGbits.RG7
#define FRAM_SCK LATGbits.LATG6
#define FRAM_SDO_TRIS TRISGbits.TRISG8
#define FRAM_SDI_TRIS TRISGbits.TRISG7
#define FRAM_SCK_TRIS TRISGbits.TRISG6

void vSetupMem() {
    iPPSOutput(OUT_PIN_PPS_RP19, OUT_FN_PPS_SDO1);
    iPPSOutput(OUT_PIN_PPS_RP21, OUT_FN_PPS_SCK1OUT);
    iPPSInput(IN_FN_PPS_SDI1, IN_PIN_PPS_RP26);

    iPPSOutput(OUT_PIN_PPS_RP11, OUT_FN_PPS_SDO3);
    iPPSOutput(OUT_PIN_PPS_RP12, OUT_FN_PPS_SCK3OUT);
    iPPSInput(IN_FN_PPS_SDI3, IN_PIN_PPS_RP3);

    TRISDbits.TRISD0 = 0; // SDO3
    TRISDbits.TRISD11 = 0;// SCK3OUT

    TRISGbits.TRISG8 = 0; //SDO1
    TRISGbits.TRISG6 = 0; //SCK1OUT


    FLASH_SDO_TRIS = 0;
    FLASH_SDI_TRIS = 1;
    FLASH_SCK_TRIS = 0;
    FLASH_SCK = 0;

    FLASH_WP_TRIS = 0;
    FLASH_CS_TRIS = 0;
    FLASH_WP = 1;
    FLASH_CS = 1;

    FLASH_SPI_STATbits.SPIEN = 0;
    FLASH_SPI_CON1 = SPI_MASTER;
    FLASH_SPI_STATbits.SPIEN = 1;

    FRAM_SDO_TRIS = 0;
    FRAM_SDI_TRIS = 1;
    FRAM_SCK_TRIS = 0;
    FRAM_SCK = 0;

    FRAM1_WP_TRIS = 0;
    FRAM1_CS_TRIS = 0;
    FRAM1_WP = 1;
    FRAM1_CS = 1;\

    FRAM_SPI_STATbits.SPIEN = 0;
    FRAM_SPI_CON1 = SPI_MASTER;
    FRAM_SPI_STATbits.SPIEN = 1;
}


/**********************************
 * SPI OPCODES
***********************************/

#define FRAM_WREN 0b00000110
#define FRAM_WRDI 0b00000100
#define FRAM_RDSR 0b00000101
#define FRAM_READ 0b00000011
#define FRAM_FSTRD 0b00001011
#define FRAM_WRITE 0b00000010
#define FRAM_SLEEP 0b10111001
#define FRAM_RDID 0b10011111
#define FRAM_SNR 0b11000011

#define FLASH_WREN FRAM_WREN
#define FLASH_WRDI FRAM_WRDI
#define FLASH_RDSR FRAM_RDSR
#define FLASH_READ_DB2 0b00011011
#define FLASH_READ_DB1 0b00001011
#define FLASH_READ_DB0 0b00000011
#define FLASH_READ FLASH_READ_DB0 //use 0 dummy bytes as default read
#define FLASH_FSTRD FLASH_READ_DB1
#define FLASH_WRITE FRAM_WRITE
#define FLASH_ERASE_4KB 0b00100000
#define FLASH_ERASE_32KB 0b01010010
#define FLASH_ERASE_64KB 0b11011000
#define FLASH_ERASE FLASH_ERASE_4KB
#define FLASH_RDID FRAM_RDID
#define FLASH_SNR FRAM_SNR
#define FLASH_PROTECT 0x36
#define FLASH_UNPROTECT 0x39

inline static unsigned char prvWriteFlashByte(unsigned char byte) {
    FLASH_SPI_BUF = byte;
    while (!FLASH_SPI_STATbits.SPIRBF);
    return FLASH_SPI_BUF;
}
inline static unsigned char prvWriteFRAMByte(unsigned char byte) {
    FRAM_SPI_BUF = byte;
    while (!FRAM_SPI_STATbits.SPIRBF);
    return FRAM_SPI_BUF;
}
inline static void prvFlashWREN() {
    FLASH_CS = 0;
    prvWriteFlashByte(FLASH_WREN);
    FLASH_CS = 1;
}
inline static void prvFlashWRDI() {
    FLASH_CS = 0;
    prvWriteFlashByte(FLASH_WRDI);
    FLASH_CS = 1;
}
inline static void prvFRAMWREN() {
    FLASH_CS = 0;
    prvWriteFlashByte(FRAM_WREN);
    FLASH_CS = 1;
}
inline static void prvFRAMWRDI() {
    FLASH_CS = 0;
    prvWriteFlashByte(FRAM_WRDI);
    FLASH_CS = 1;
}
static void prvFlashProtect(unsigned char * address) {
    FLASH_CS = 0;
    prvWriteFlashByte(FLASH_PROTECT);
    int i;
    for (i = 0; i < 3; i++) {
        prvWriteFlashByte(address[i]);
    }
    FLASH_CS = 1;
}
static void prvFlashUnprotect(unsigned char * address) {
    FLASH_CS = 0;
    prvWriteFlashByte(FLASH_UNPROTECT);
    int i;
    for (i = 0; i < 3; i++) {
        prvWriteFlashByte(address[i]);
    }
    FLASH_CS = 1;
}

void vFlashErase(char * address) {
    FLASH_CS = 0;
    //TODO implement this if needed. Right now Flash is probably not used
    FLASH_CS = 1;
}

void vFlashWrite(unsigned char * address, int length, unsigned char * bytes) {
    prvFlashWREN();
    prvFlashUnprotect(address);
    FLASH_CS = 0;
    prvWriteFlashByte(FLASH_WRITE);
    prvWriteFlashByte(address[0]);
    prvWriteFlashByte(address[1]);
    prvWriteFlashByte(address[2]);
    int i;
    for (i = 0; i< length; i++) {
        prvWriteFlashByte(bytes[i]);
    }
    FLASH_CS = 1;
    prvFlashProtect(address);
    prvFlashWRDI();
}

void vFlashRead(unsigned char * address, int length, unsigned char * buffer) {
    FLASH_CS = 0;
    prvWriteFlashByte(FLASH_READ);
    prvWriteFlashByte(address[0]);
    prvWriteFlashByte(address[1]);
    prvWriteFlashByte(address[2]);
    int i;
    for (i = 0; i< length; i++) {
        buffer[i] = prvWriteFlashByte(0xff);
    }
    FLASH_CS = 1;
}
void vFlashReadId(unsigned char * buffer) {
    FLASH_CS = 0;
    prvWriteFlashByte(FLASH_RDID);
    int i;
    for (i = 0; i < 4; i++){
        buffer[i] = prvWriteFlashByte(0xff);
    }
    FLASH_CS = 1;
}

void vFRAMReadId(unsigned char * buffer) {
    FRAM1_CS = 0;
    prvWriteFRAMByte(FRAM_RDID);
    int i;
    for (i = 0; i < 4; i++){
        buffer[i] = prvWriteFRAMByte(0xff);
    }
    FRAM1_CS = 1;
}
void vFRAMRead(unsigned char * address, int length, unsigned char * buffer) {
    FRAM1_CS = 0;
    prvWriteFlashByte(FRAM_READ);
    prvWriteFlashByte(address[0]);
    prvWriteFlashByte(address[1]);
    prvWriteFlashByte(address[2]);
    int i;
    for (i = 0; i< length; i++) {
        buffer[i] = prvWriteFRAMByte(0xff);
    }
    FRAM1_CS = 1;
}
void vFRAMWrite(unsigned char * address, int length, unsigned char * bytes) {
    prvFRAMWREN();
    FRAM1_CS = 0;
    prvWriteFRAMByte(FLASH_WRITE);
    prvWriteFRAMByte(address[0]);
    prvWriteFRAMByte(address[1]);
    prvWriteFRAMByte(address[2]);
    int i;
    for (i = 0; i< length; i++) {
        prvWriteFRAMByte(bytes[i]);
    }
    FRAM1_CS = 1;
    prvFRAMWRDI();
}

static void prvTestTask() {
    vConsolePrint("Flash Test Task Started!\r\n");

    unsigned char buffer[4] = {0,0,0,0};
    vFlashReadId(buffer);
    vConsolePrintf("Flash MFG Id: %d\r\n", buffer[0]);
    unsigned char address[3] = {0,0,0};
    vFlashRead(address, 4, buffer);
    vConsolePrintf("The first 4 Bytes in Flash: %d %d %d %d\r\n", buffer[0], buffer[1], buffer[2], buffer[3]);
    vConsolePrintf("Looping RDID Command to FRAM1...\r\n");
    for(;;) {
        vFRAMReadId(buffer);
    }
}

void vStartMemTestTask() {
    xTaskCreate(prvTestTask, NULL, configMINIMAL_STACK_SIZE, NULL, systemPRIORITY_TEST, NULL);
}