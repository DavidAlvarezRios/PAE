/*
 * lib_UART.c
 *
 *  Created on: 7 de maig 2019
 *      Author: mat.aules
 */


#include "lib_UART.h"


byte DatoLeido_UART = 0;
//Variables P4
uint8_t si = 1;
uint8_t no = 0;


uint8_t count_timeout = 0;
uint16_t count_ms; //contador milisegons

char borrado[] = "               "; //una linea entera de 15 espacios en blanco

//uint8_t ID_2 = 0x02;
//uint8_t ID_3 = 0x03;

byte Byte_Recibido = 1;

/**************************************************************************
 * INICIALIZACI�N DE LA PANTALLA LCD.
 *
 * Sin datos de entrada
 *
 * Sin datos de salida
 *
 **************************************************************************/
void init_LCD(void)
{
    halLcdInit(); //Inicializar y configurar la pantallita
    halLcdClearScreenBkg(); //Borrar la pantalla, rellenando con el color de fondo
}

/**************************************************************************
 * BORRAR LINEA
 *
 * Datos de entrada: Linea, indica la linea a borrar
 *
 * Sin datos de salida
 *
 **************************************************************************/
void borrar(uint8_t Linea)
{
    halLcdPrintLine(borrado, Linea, NORMAL_TEXT); //escribimos una linea en blanco
}

/**************************************************************************
 * ESCRIBIR LINEA
 *
 * Datos de entrada: Linea, indica la linea del LCD donde escribir
 *                   String, la cadena de caracteres que vamos a escribir
 *
 * Sin datos de salida
 *
 **************************************************************************/
void escribir(char String[], uint8_t Linea)

{
    halLcdPrintLine(String, Linea, NORMAL_TEXT); //Enviamos la String al LCD, sobreescribiendo la Linea indicada.
}


/*Activa el sentit RX
 *
 *Dades d'entrada: cap
 *
 *Dades de sortida: cap
 *
 **/
void sentit_dades_rx(void)
{
    P3OUT &= ~BIT0;
}

/*Activa el sentit TX
 *
 *Dades d'entrada: cap
 *
 *Dades de sortida: cap
 *
 **/
void sentit_dades_tx(void)
{
    P3OUT |= BIT0;
}

/*Funci� que escriu un byte al buffer
 *
 *Dades d'entrada: Byte amb la dada
 *
 *Dades de sortida: cap
 *
 */

void TxUAC2(byte bTxdData)
{
    while(!TXD2_READY); // Espera a que estigui preparat el buffer de transmissi�
    UCA2TXBUF = bTxdData;

}

void timeOut_Reset(void)
{
    count_timeout = 0;
}

uint8_t TimeOut_check(uint8_t check)
{
    if(count_timeout >= check)
    {
        return 1;
    }else{
        return 0;
    }

}

void Activa_TimerA1_TimeOut(void)
{
    TA1CTL |= MC__UP; //seleccionem el mode UP i iniciem el timer
    //TA1CTL |= MC__STOP; //Parem el timer posant-lo a mode STOP
}

void Apaga_TimerA1_TimeOut(void)
{
    TA1CTL |= MC__STOP; //Parem el timer posant-lo a mode STOP
}


/**
 * Funcio per enviar les dades
 *
 * Dades d'entrada: bID, BParameterLength, bInstruction, Parametros[16]
 *
 * Dades de sortida: byte, Longitud del paquet.
 */
byte TxPacket(byte bID, byte bParameterLength, byte bInstruction, byte Parametros[16])
{
    byte bCount,bCheckSum,bPacketLength;
    byte TxBuffer[32];
    sentit_dades_tx(); //El pin P3.0 (DIRECTION_PORT) el posem a 1 (Transmetre)
    TxBuffer[0] = 0xff; //Primers 2 bytes que indiquen inici de trama FF, FF.
    TxBuffer[1] = 0xff;
    TxBuffer[2] = bID; //ID del m�dul al que volem enviar el missatge
    TxBuffer[3] = bParameterLength+2; //Length(Parameter,Instruction,Checksum)
    TxBuffer[4] = bInstruction; //Instrucci� que enviem al M�dul
    for(bCount = 0; bCount < bParameterLength; bCount++) //Comencem a generar la trama que hem d�enviar
    {
        TxBuffer[bCount+5] = Parametros[bCount];
    }
    bCheckSum = 0;
    bPacketLength = bParameterLength+4+2;
    for(bCount = 2; bCount < bPacketLength-1; bCount++) //C�lcul del checksum
    {
        bCheckSum += TxBuffer[bCount];
    }
    TxBuffer[bCount] = ~bCheckSum; //Escriu el Checksum (complement a 1)
    for(bCount = 0; bCount < bPacketLength; bCount++) //Aquest bucle �s el que envia la trama al M�dul Robot
    {
        TxUAC2(TxBuffer[bCount]);
    }
    while( (UCA2STATW&UCBUSY)); //Espera fins que s�ha transm�s el �ltim byte
    sentit_dades_rx(); //Posem la l�nia de dades en Rx perqu� el m�dul Dynamixel envia resposta
    return(bPacketLength);
}

struct RxReturn RxPacket(void){
    struct RxReturn respuesta;
    byte bCount,bLenght,bChecksum;
    byte Rx_time_out = 0;
    sentit_dades_rx();
    timeOut_Reset();
    Activa_TimerA1_TimeOut();
    for(bCount=0;bCount < 4; bCount ++){
        Byte_Recibido = 0;
        timeOut_Reset();
        while (Byte_Recibido == 0){
            Rx_time_out = TimeOut_check(100);
            if(Rx_time_out){

                break;
            }
        }
        if(Rx_time_out){
            break;
        }
        respuesta.StatusPacket[bCount] = DatoLeido_UART;

    }
    bLenght = respuesta.StatusPacket[3];
    if (!Rx_time_out){
        for(bCount = 4; bCount < 4+bLenght; bCount ++){
            timeOut_Reset();
            Byte_Recibido = 0;
            while( Byte_Recibido == 0){
                Rx_time_out = TimeOut_check(100);
                if(Rx_time_out) break;
            }
            if(Rx_time_out) break;
            respuesta.StatusPacket[bCount] = DatoLeido_UART;
        }
        bChecksum = 0;

        for (bCount=2; bCount < bLenght + 2; bCount ++){
            bChecksum += respuesta.StatusPacket[bCount];
        }

        respuesta.StatusPacket[bCount+1] = ~bChecksum;
    }

    return respuesta;


}
/**************************************************************************
 * DELAY - A CONFIGURAR POR EL ALUMNO - amb timers
 *
 * Datos de entrada: Tiempo de retraso. 1 segundo equivale a un retraso de 1000000 (aprox)
 *
 * Sin datos de salida
 *
 **************************************************************************/
void delay_t (uint32_t temps)
{
   //volatile uint32_t i;
    TA0CTL |= MC__UP; //seleccionem el mode UP i iniciem el timer
    count_ms = 0;    //posem el comptador a 0
    do{

    }while (count_ms < temps);

    TA0CTL |= MC__STOP; //Parem el timer posant-lo a mode STOP
}

void EUSCIA2_IRQHandler(void)
{ //interrupcion de recepcion en la UART A2
    UCA2IE &= ~UCRXIE; //Interrupciones desactivadas en RX
    DatoLeido_UART = UCA2RXBUF;
    Byte_Recibido=si;
    UCA2IE |= UCRXIE; //Interrupciones reactivadas en RX
}

void TA0_0_IRQHandler(void)
{
    TA0CCTL0 &= ~CCIE; //inhabilitem la interrupcio
    count_ms += 1; //Aumentamos el contador para saber cu�nto tiempo ha transcurrido en funci�n del tiempo que tarde en llamarse dicha interrupci�n.
    TA0CCTL0 &= ~CCIFG;
    TA0CCTL0 |= CCIE; //Habilitem la interrupció
}

/*****************************************************************************
 * Gestión de la interrupcion del timer 1
 *
 * Sin datos de entrada
 *
 * Sin datos de salida
 *
 ****************************************************************************/
void TA1_0_IRQHandler(void)
{
    TA1CCTL0 &= ~CCIE; //inhabilitem la interrupcio
    count_timeout += 1; //Aumentamos el contador para saber cu�nto tiempo ha transcurrido en funci�n del tiempo que tarde en llamarse dicha interrupci�n.
    TA1CCTL0 &= ~CCIFG;
    TA1CCTL0 |= CCIE; //Habilitem la interrupció
}
