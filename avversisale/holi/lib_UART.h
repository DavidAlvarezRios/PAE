/*
 * lib_UART.h
 *
 *  Created on: 7 de maig 2019
 *      Author: mat.aules
 */

#ifndef LIB_UART_H_
#define LIB_UART_H_

#include <msp432p401r.h>
#include "lib_PAE2.h" //Libreria grafica + configuracion reloj MSP432
typedef uint8_t byte;

struct RxReturn
{
    byte StatusPacket[32];
    uint8_t time_out;
    uint8_t checkSum;
};

struct Data{
    byte dades[3];
    uint8_t time_out;
    uint8_t checkSum;
};


#define TXD2_READY (UCA2IFG & UCTXIFG)


void init_LCD(void);
void borrar(uint8_t Linea);
void escribir(char String[], uint8_t Linea);
void sentit_dades_rx(void);
void sentit_dades_tx(void);
void TxUAC2(byte bTxdData);
void timeOut_Reset(void);
uint8_t TimeOut_check(uint8_t check);
void Activa_TimerA1_TimeOut(void);
void Apaga_TimerA1_TimeOut(void);
byte TxPacket(byte bID, byte bParameterLength, byte bInstruction, byte Parametros[16]);
struct RxReturn RxPacket(void);
void delay_t (uint32_t temps);


#endif /* LIB_UART_H_ */
