/******************************
 * 
 * Practica_02_PAE Programaci� de Ports
 * i pr�ctica de les instruccions de control de flux:
 * "do ... while", "switch ... case", "if" i "for"
 * UB, 02/2017.
 *****************************/

#include <msp432p401r.h>
#include <stdio.h>
#include <stdint.h>
#include "lib_PAE2.h" //Libreria grafica + configuracion reloj MSP432


#define S1 1
#define S2 2
#define Jstick_Left 3
#define Jstick_Right 4
#define Jstick_Up 5
#define Jstick_Down 6
#define Jstick_Center 7


//ESTATS MOVIMENT LED
#define STOP 0
#define MOV_RIGHT 1
#define MOV_LEFT 2

//ID'S DELS MOTOR
#define MOTOR_2 0x02
#define MOTOR_3 0x03
#define SENSOR 0x64

// INSTRUCCIONS MOTOR
#define WRITE 0x03
#define READ 0x02
#define FORWARD 0x01
#define BACKWARD 0x00
#define LEFT 0x00
#define RIGHT 0x02
#define CENTER 0x01

// ID'S SENSORS
#define SENSOR_LEFT 0
#define SENSOR_CENTER 1
#define SENSOR_RIGHT 2

// DIRECCIONS MOTOR
#define SENTIT_CW 0x04
#define SENTIT_CCW 0x00

// VELOCITATS
#define MOVING_SPEED 0x20

#define LED_MOTOR 0x19
#define ENCENDER_LED 1

char saludo[16] = " PRACTICA 4 PAE";//max 15 caracteres visibles
char cadena[16];//Una linea entera con 15 caracteres visibles + uno oculto de terminacion de cadena (codigo ASCII 0)
char borrado[] = "               "; //una linea entera de 15 espacios en blanco


typedef uint8_t byte;
#define TXD2_READY (UCA2IFG & UCTXIFG)

uint8_t linea = 1;
uint8_t estado=0;
uint8_t estado_anterior = 8;
uint16_t count_ms; //contador milisegons
uint32_t retraso = 200;
//uint32_t tiempo = 0;
uint8_t seg,min,hora = 0; // segons minuts hora actuals

uint8_t Parametros[16]; //Parametres de les instruccions del motor.

//Flags que utilitzarem per saber si modifiquem el rellotge o la alarma
uint8_t modifica_hora = 1;
uint8_t modifica_alarma = 0;

uint8_t min_alarma = 1; //Posem la alarma per defecte que soni a les 00:01:00
uint8_t hora_alarma = 0;
uint8_t seg_alarma = 0;

uint8_t flag_retraso = 0; //0 modificamos retraso, 1 no lo modificamos se cambia con Jstick_Center

uint8_t cambio_estado = 0; //esta variable sirve para cambiar la hora, minuto (0, 1)

//Necesitamos una variable para almacenar los valores de la velocidad (Cuando movemos el JoyStick hacia arriba o hacia abajo)
// Y tambien necesitamos otra variable para saber si los LEDs se enciended de derecha a izquierda o al reves
// (Cuando movemos el joystick hacia la derecha o hacia la izquierda)

uint8_t velocitat = 0;
uint8_t direccio_LED = 0;

//Variables P4
uint8_t si = 1;
uint8_t no = 0;

byte DatoLeido_UART = 0;

uint8_t count_timeout = 0;


//uint8_t ID_2 = 0x02;
//uint8_t ID_3 = 0x03;

byte Byte_Recibido = 1;

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

/**************************************************************************
 * INICIALIZACI�N DEL CONTROLADOR DE INTERRUPCIONES (NVIC).
 *
 * Sin datos de entrada
 *
 * Sin datos de salida
 *
 **************************************************************************/
void init_interrupciones()
{
    // Configuracion al estilo MSP430 "clasico":
    // --> Enable Port 4 interrupt on the NVIC.
    // Segun el Datasheet (Tabla "6-39. NVIC Interrupts", apartado "6.7.2 Device-Level User Interrupts"),
    // la interrupcion del puerto 4 es la User ISR numero 38.
    // Segun el Technical Reference Manual, apartado "2.4.3 NVIC Registers",
    // hay 2 registros de habilitacion ISER0 y ISER1, cada uno para 32 interrupciones (0..31, y 32..63, resp.),
    // accesibles mediante la estructura NVIC->ISER[x], con x = 0 o x = 1.
    // Asimismo, hay 2 registros para deshabilitarlas: ICERx, y dos registros para limpiarlas: ICPRx.

    //Int. port 3 = 37 corresponde al bit 5 del segundo registro ISER1:
    NVIC->ICPR[1] |= BIT5; //Primero, me aseguro de que no quede ninguna interrupcion residual pendiente para este puerto,
    NVIC->ISER[1] |= BIT5; //y habilito las interrupciones del puerto
    //Int. port 4 = 38 corresponde al bit 6 del segundo registro ISERx:
    NVIC->ICPR[1] |= BIT6; //Primero, me aseguro de que no quede ninguna interrupcion residual pendiente para este puerto,
    NVIC->ISER[1] |= BIT6; //y habilito las interrupciones del puerto
    //Int. port 5 = 39 corresponde al bit 7 del segundo registro ISERx:
    NVIC->ICPR[1] |= BIT7; //Primero, me aseguro de que no quede ninguna interrupcion residual pendiente para este puerto,
    NVIC->ISER[1] |= BIT7; //y habilito las interrupciones del puerto

    NVIC->ICPR[0] |= BIT8; //Primero nos aseguramos de que no quede ninguna interrupcion residual para el timer TA0
    NVIC->ISER[0] |= BIT8; //Habilitamos las interrupciones a nivel NVIC
    
    NVIC->ICPR[0] |= BITA; //Primero nos aseguramos de que no quede ninguna interrupcion residual para el timer TA1
    NVIC->ISER[0] |= BITA; //Habilitamos las interrupciones a nivel NVIC

    NVIC->ICPR[0] |= BIT(18); // Aseguramos que no queda ninguna interrupcion residual
    NVIC->ISER[0] |= BIT(18); //Habilitamos las interrupciones a nivel NVIC para el motor AX-S1

    __enable_interrupt(); //Habilitamos las interrupciones a nivel global del micro.
}

/**********************************************************
* Con esta función inicializaremos y configuraremos el timer ACLK
**********************************************************/
void init_timer(void)
{

    TA0CTL |= TASSEL__ACLK;
    TA0CCR0 = 32; //Aquest valor es l'equivalent en hertz mes o menys a un milisegon
    // Activamos las interrupciones del tiemr
    TA0CCTL0 |= CCIE;
    TA0CCTL0 &= ~CCIFG;

}

/**
 * Incializamos el timer T1 en modo UP
 */
void init_clock(void){
    

    TA1CTL |= TASSEL__ACLK;
    TA1CTL |= MC__UP; //Lo configuramos en modo UP, es decir, cuenta hasta el valor en TA1CCR0 y llama a la interrupcion
    TA1CCR0 = 32000; //Aquest valor es l'equivalent en hertz mes o menys a un segon
    //Definimos el valor del registro TA1CCTL0
    TA1CCTL0 |= CCIE;
    TA1CCTL0 &= ~CCIFG;



}

void init_uart(void)
{
    UCA2CTLW0 |= UCSWRST; //Fem un reset de la USCI, desactiva la USCI
    UCA2CTLW0 |= UCSSEL__SMCLK; //UCSYNC=0 mode as�ncron
    //UCMODEx=0;  //seleccionem mode UART
    //UCSPB=0;    //nomes 1 stop bit
    //UC7BIT=0;   //8 bits de dades
    //UCMSB=0;    //bit de menys pes primer
    //UCPAR=x;    //ja que no es fa servir bit de paritat
    //UCPEN=0;    //sense bit de parita
    //UCSYNC = 0; //mode UART de USCI
    //Triem SMCLK (24MHz) com a font del clock BRCLK
    UCA2MCTLW = UCOS16; // Necessitem sobre-mostreig => bit 0 = UCOS16 = 1
    UCA2BRW = 3; //Prescaler de BRCLK fixat a 3. Com SMCLK va a24MHz,
    //volem un baud rate de 500kb/s i fem sobre-mostreig de 16
    //el rellotge de la UART ha de ser de 8MHz (24MHz/3).
    //Configurem els pins de la UART
    P3SEL0 |= BIT2 | BIT3; //I/O funci�: P3.3 = UART2TX, P3.2 = UART2RX
    P3SEL1 &= ~ (BIT2 | BIT3);
    //Configurem pin de selecci� del sentit de les dades Transmissi�/Recepeci�
    P3SEL0 &= ~BIT0; //Port P3.0 com GPIO
    P3SEL1 &= ~BIT0;
    P3DIR |= BIT0; //Port P3.0 com sortida (Data Direction selector Tx/Rx)
    P3OUT &= ~BIT0; //Inicialitzem Sentit Dades a 0 (Rx)
    UCA2CTLW0 &= ~UCSWRST; //Reactivem la l�nia de comunicacions s�rie
    UCA2IE |= UCRXIE; //Aix� nom�s s�ha d�activar quan tinguem la rutina de recepci�
}

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
 * 					 String, la cadena de caracteres que vamos a escribir
 * 
 * Sin datos de salida
 * 
 **************************************************************************/
void escribir(char String[], uint8_t Linea)

{
	halLcdPrintLine(String, Linea, NORMAL_TEXT); //Enviamos la String al LCD, sobreescribiendo la Linea indicada.
}

/**************************************************************************
 * INICIALIZACI�N DE LOS BOTONES & LEDS DEL BOOSTERPACK MK II.
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 * 
 **************************************************************************/
void init_botons(void)
{
    //Configuramos botones y leds
    //***************************

    //Leds RGB del MK II:
      P2DIR |= 0x50;  //Pines P2.4 (G), 2.6 (R) como salidas Led (RGB)
      P5DIR |= 0x40;  //Pin P5.6 (B)como salida Led (RGB)
      P2OUT &= 0xAF;  //Inicializamos Led RGB a 0 (apagados)
      P5OUT &= ~0x40; //Inicializamos Led RGB a 0 (apagados)

    //Boton S1 del MK II:
      P5SEL0 &= ~0x02;   //Pin P5.1 como I/O digital,
      P5SEL1 &= ~0x02;   //Pin P5.1 como I/O digital,
      P5DIR &= ~0x02; //Pin P5.1 como entrada
      P5IES &= ~0x02;   // con transicion L->H
      P5IE |= 0x02;     //Interrupciones activadas en P5.1,
      P5IFG = 0;    //Limpiamos todos los flags de las interrupciones del puerto 5
      //P5REN: Ya hay una resistencia de pullup en la placa MK II

    //Boton S2 del MK II:
      P3SEL0 &= ~0x20;   //Pin P3.5 como I/O digital,
      P3SEL1 &= ~0x20;   //Pin P3.5 como I/O digital,
      P3DIR &= ~0x20; //Pin P3.5 como entrada
      P3IES &= ~0x20;   // con transicion L->H
      P3IE |= 0x20;   //Interrupciones activadas en P3.5
      P3IFG = 0;  //Limpiamos todos los flags de las interrupciones del puerto 3
      //P3REN: Ya hay una resistencia de pullup en la placa MK II

    //Configuramos los GPIOs del joystick del MK II:
      P4DIR &= ~(BIT1 + BIT5 + BIT7);   //Pines P4.1, 4.5 y 4.7 como entrades,
      P4SEL0 &= ~(BIT1 + BIT5 + BIT7);  //Pines P4.1, 4.5 y 4.7 como I/O digitales,
      P4SEL1 &= ~(BIT1 + BIT5 + BIT7);
      P4REN |= BIT1 + BIT5 + BIT7;  //con resistencia activada
      P4OUT |= BIT1 + BIT5 + BIT7;  // de pull-up
      P4IE |= BIT1 + BIT5 + BIT7;   //Interrupciones activadas en P4.1, 4.5 y 4.7,
      P4IES &= ~(BIT1 + BIT5 + BIT7);   //las interrupciones se generaran con transicion L->H
      P4IFG = 0;    //Limpiamos todos los flags de las interrupciones del puerto 4

      P5DIR &= ~(BIT4 + BIT5);  //Pines P5.4 y 5.5 como entrades,
      P5SEL0 &= ~(BIT4 + BIT5); //Pines P5.4 y 5.5 como I/O digitales,
      P5SEL1 &= ~(BIT4 + BIT5);
      P5IE |= BIT4 + BIT5;  //Interrupciones activadas en 5.4 y 5.5,
      P5IES &= ~(BIT4 + BIT5);  //las interrupciones se generaran con transicion L->H
      P5IFG = 0;    //Limpiamos todos los flags de las interrupciones del puerto 4
    // - Ya hay una resistencia de pullup en la placa MK II
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

/*****************************************************************************
 * Funcion que comprueba si se ha llegado a la hora establecida para imprimir
 * el aviso por la pantalla
 *
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/
void comprobar_alarma(void)
{
    if(hora == hora_alarma && min == min_alarma)
    {
        sprintf(cadena, "ALARMA!!!"); // Guardamos en cadena la siguiente frase: ALARMA!!!
        escribir(cadena, linea+5);          // Escribimos la cadena al LCD
    }
}


/*****************************************************************************
 * Funcion que contiene el codigo para imprimir en el LCD el tiempo.
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/
void imprimir_temps(void)
{
    if(seg >= 60)
    {
        seg = 0;
        min++;
    }
    if(min >= 60)
    {
        min = 0;
        hora++;
    }
    if(hora >= 24){
        hora = 0;
    }


    sprintf(cadena, "t: %02d:%02d:%02d", hora, min, seg); // Mostrem per pantalla l'hora actual
    escribir(cadena, linea+2); // Escribim a la pantalla.
    comprobar_alarma();

}

/*****************************************************************************
 * Funcion que decrementa en 1 la hora o el minuto del reloj, segun la flag cambio_estado
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/
void incrementar_hora(void){

    if(cambio_estado == 0)
    {
        hora++;
    }
    if(cambio_estado == 1)
    {
        min++;
    }



}

/*****************************************************************************
 * Funcion que decrementa en 1 la hora o el minuto del reloj, segun la flag cambio_estado
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/

void decrementar_hora(void)
{

    if(cambio_estado == 1)
    {
        if(min > 0)
        {
            min--;
        }

    }

    if(cambio_estado == 0)
    {
        if(hora > 0)
        {
            hora--;
        }
    }
}

/*****************************************************************************
 * Funcion que incrementa en 1 la hora o el minuto de la alarma, segun la flag cambio_estado
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/
void incrementar_alarma(void)
{

    if(cambio_estado == 0)
    {
        if(hora_alarma < 24){
            hora_alarma++;
        }
    }

    if(cambio_estado == 1)
    {
        if(min_alarma < 60)
        {
            min_alarma++;
        }
    }

    if(min_alarma == 60){
        min_alarma = 0;
        hora_alarma++;
    }

    if(hora_alarma == 24){
        hora_alarma = 0;
    }
    
}

/*****************************************************************************
 * Funcion que decrementa en 1 la hora o el minuto de la alarma, segun la flag cambio_estado
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/

void decrementar_alarma(void)
{

    if(cambio_estado == 0)
    {
        if(hora_alarma > 0){
            hora_alarma--;
        }
    }

    if(cambio_estado == 1)
    {
        if(min_alarma > 0)
        {
            min_alarma--;
        }
    }
}





/*****************************************************************************
 * CONFIGURACI�N DEL PUERTO 7. A REALIZAR POR EL ALUMNO
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/
void config_P7_LEDS (void)
{

	//A RELLENAR POR EL ALUMNO

    // Tots els LEDs configurats com I/O
    P7SEL0 = 0x00;
    P7SEL1 = 0x00;
    //Tots els LEDs com sortida
    P7DIR = 0xFF;
    // Apagats de inici
    P7OUT = 0x00;

}



/**************************************************************************
 * RUTINAS DE GESTION DE LOS BOTONES:
 * Mediante estas rutinas, se detectar� qu� bot�n se ha pulsado
 * 		 
 * Sin Datos de entrada
 * 
 * Sin datos de salida
 * 
 * Actualizar el valor de la variable global estado
 * 
 **************************************************************************/

//ISR para las interrupciones del puerto 3:
void PORT3_IRQHandler(void){//interrupcion del pulsador S2
	uint8_t flag = P3IV; //guardamos el vector de interrupciones. De paso, al acceder a este vector, se limpia automaticamente.
    P3IE &= 0xDF;  //interrupciones del boton S2 en port 3 desactivadas
    estado_anterior=0;

    /**********************************************************+
        A RELLENAR POR EL ALUMNO
	Para gestionar los estados:
    Boton S1, estado = 1
    Boton S2, estado = 2
    Joystick left, estado = 3
    Joystick right, estado = 4
    Joystick up, estado = 5
    Joystick down, estado = 6
    Joystick center, estado = 7
    ***********************************************************/

    switch(flag){
    case 0x0C: //S2
        estado = S2;
        break;
    }

    P3IE |= 0x20;   //interrupciones S2 en port 3 reactivadas
}

//ISR para las interrupciones del puerto 4:
void PORT4_IRQHandler(void){  //interrupci�n de los botones. Actualiza el valor de la variable global estado.
	uint8_t flag = P4IV; //guardamos el vector de interrupciones. De paso, al acceder a este vector, se limpia automaticamente.
	P4IE &= 0x5D; 	//interrupciones Joystick en port 4 desactivadas
	estado_anterior=0;
	
    /**********************************************************+
        A RELLENAR POR EL ALUMNO BLOQUE switch ... case
	Para gestionar los estados:
    Boton S1, estado = 1
    Boton S2, estado = 2
    Joystick left, estado = 3
    Joystick right, estado = 4
    Joystick up, estado = 5
    Joystick down, estado = 6
    Joystick center, estado = 7
    ***********************************************************/
	
	switch(flag){
	        case 0x04: //pin 1
	            estado = Jstick_Center; //center
	            break;
	        case 0x0C: //pin 5
	            estado = Jstick_Right; //right
	            break;
	        case 0x10: //pin 7
	            estado = Jstick_Left; //Left
	            break;
	        }
	/***********************************************
   	 * HASTA AQUI BLOQUE CASE
   	 ***********************************************/	
	
	P4IE |= 0xA2; 	//interrupciones Joystick en port 4 reactivadas
}

//ISR para las interrupciones del puerto 5:
void PORT5_IRQHandler(void){  //interrupci�n de los botones. Actualiza el valor de la variable global estado.
	uint8_t flag = P5IV; //guardamos el vector de interrupciones. De paso, al acceder a este vector, se limpia automaticamente.
	P5IE &= 0xCD;   //interrupciones Joystick y S1 en port 5 desactivadas
	estado_anterior=0;

    /**********************************************************+
        A RELLENAR POR EL ALUMNO BLOQUE switch ... case
	Para gestionar los estados:
    Boton S1, estado = 1
    Boton S2, estado = 2
    Joystick left, estado = 3
    Joystick right, estado = 4
    Joystick up, estado = 5
    Joystick down, estado = 6
    Joystick center, estado = 7
    ***********************************************************/
	/*
	if (estado_anterior != estado){
	        switch (estado){

	        case 1:
	            P2OUT |= 0x50;
	            P5OUT |= 0x20;
	            break;
	        case 0x0A:
	            P2OUT |= 0x20;
	            P5OUT |= 0x20;
	            break;
	        case 0x0C:
	            P2OUT |= 0x10;
	            P5OUT |= 0x20;
	            break;
	        }
	    }
	   */

    switch(flag){
            case 0x0C:
                estado = Jstick_Down; //DOWN
                break;
            case 0x0A:
                estado = Jstick_Up; //UP
                break;
            case 0x04:
                estado = S1; //S1
                break;
            }

    /***********************************************
     * HASTA AQUI BLOQUE CASE
     ***********************************************/

    P5IE |= 0x32;   //interrupciones Joystick y S1 en port 5 reactivadas
}


/*****************************************************************************
 * Gestión de la interrupcion del timer 0
 * 
 * Sin datos de entrada
 * 
 * Sin datos de salida
 *  
 ****************************************************************************/
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
    TA1CCTL0 &=~CCIE;//Inhabilitem la interrupci0
    seg += 1; //contador segundos
    count_timeout += 1; //augmentem el comptador del time out.
    TA1CCTL0 &= ~CCIFG;
    TA1CCTL0 |= CCIE;//Habilitem la interrupcio
}

void EUSCIA2_IRQHandler(void)
{ //interrupcion de recepcion en la UART A2
    UCA2IE &= ~UCRXIE; //Interrupciones desactivadas en RX
    DatoLeido_UART = UCA2RXBUF;
    Byte_Recibido=si;
    UCA2IE |= UCRXIE; //Interrupciones reactivadas en RX
}

void mov_right(uint32_t temps)
{

    int i;
    for(i = 0; i < 8; i++){
        P7OUT = (1<<i);
        delay_t(temps);
    }
    //delay_t(temps);
    P7OUT = 0;


}

void mov_left(uint32_t temps)
{
    int i;
    for(i =8; i >=0; i--){
        P7OUT = (1<<i);
        delay_t(temps);
    }
    //delay_t(temps);
    P7OUT = 0;

}

//FUNCIONES P4

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

byte TimeOut_check(uint8_t check)
{
    return count_timeout >= check;
}

void Activa_TimerA1_TimeOut()
{
     TA1CTL |= 0x0200;
     TA1CCTL0 |= 0x0010;
     TA1CCR0 = 24000;
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

    byte bCount, bLength, bCheck, bPacketLength;
    bLength = bParameterLength + 2;

    byte TxBuffer[32];

    sentit_dades_tx();

    //Posem al buffer les primeres dades que hem d'enviar
    TxBuffer[0] = 0xff; //Posicions 0 i 1 indiquen el start
    TxBuffer[1] = 0xff;
    TxBuffer[2] = bID; //ID del motor
    TxBuffer[3] = bLength; //Numero d'instruccions
    TxBuffer[4] = bInstruction; //ID de la instrucci�

    for(bCount = 0; bCount < bParameterLength; bCount++)
    {
        //Afegim a la trama els parametres adicionals
        TxBuffer[bCount+5] = Parametros[bCount];
    }

    bCheck = 0;
    bPacketLength = bParameterLength + 4 + 2;
    //Ara calculem el check sum que es la suma de tots els parametres
    for(bCount = 2; bCount < bPacketLength; bCount++)
    {
        //Comencem a dos perque no comptem els 0xff
        bCheck += TxBuffer[bCount];
    }

    TxBuffer[bCount] = ~bCheck; //En complement a 1
    //Ara enviem les dades

    for(bCount = 0; bCount < bPacketLength; bCount++)
    {
        TxUAC2(TxBuffer[bCount]);
    }
    while((UCA2STATW&UCBUSY));
    sentit_dades_rx();
    return (bPacketLength);

}

struct RxReturn RxPacket(void)
{
    struct RxReturn resposta;

    byte bCount, bLength, bChecksum;
    byte Rx_time_out = 0;

    sentit_dades_rx(); //Posem el sentit de les dades en rebent dades

    Activa_TimerA1_TimeOut();

    for(bCount = 0; bCount < 4; bCount++)
    {
        timeOut_Reset();
        Byte_Recibido=0;
        while(!Byte_Recibido)
        {
            Rx_time_out = TimeOut_check(1000);
            if(Rx_time_out) break;
        }
        if(Rx_time_out) break;

        resposta.StatusPacket[bCount] = DatoLeido_UART;
    }
    if(!Rx_time_out)    return resposta;

    bLength = DatoLeido_UART + 4;
    for(bCount = 0; bCount < bLength; bCount++)
    {
        timeOut_Reset();
        Byte_Recibido=0;
        while(!Byte_Recibido)
        {
            resposta.time_out= TimeOut_check(1000);
            if(resposta.time_out)  break;
        }
        if(resposta.time_out)  return resposta;
        resposta.StatusPacket[bCount] = DatoLeido_UART;

        if(resposta.time_out)  return resposta;

        bChecksum = 0;
        for(bCount = 2; bCount < bLength-1; bCount++)
        {
            bChecksum+= resposta.StatusPacket[bCount];
        }

        bChecksum = ~bChecksum;
        if(bChecksum != resposta.StatusPacket[bCount-1])    resposta.checkSum = 1;

    }

    return resposta;

}


void moure_motor(uint8_t id, uint8_t sentit, uint16_t velocitat)
{
    byte parametres[3];
    parametres[0]=MOVING_SPEED;
    parametres[1]=velocitat & 0xff;
    parametres[2]=(velocitat>>8) & 0xff;
    parametres[2]|=sentit;
    TxPacket(id,0x03,WRITE,parametres);
}


void moure_robot(uint8_t sentit, uint16_t velocitat)
{
    if(sentit == FORWARD)
    {
        moure_motor(MOTOR_2, SENTIT_CW, velocitat);
        moure_motor(MOTOR_3, SENTIT_CCW, velocitat);
    }
    else
    {
        moure_motor(MOTOR_2, SENTIT_CCW, velocitat);
        moure_motor(MOTOR_3, SENTIT_CW, velocitat);
    }
}


void activate_led(void) {
    byte parametres[2];
    parametres[0] = 0x19;
    parametres[1] = 0x01;
    TxPacket(MOTOR_2,0x02,WRITE,parametres);
    TxPacket(MOTOR_3,0x02,WRITE,parametres);
}









void ApagarRGB_Launchpad()
{
    P2OUT &= ~0x50;
    P5OUT &= ~0x40;
}

void main(void)
{

    WDTCTL = WDTPW+WDTHOLD;         // Paramos el watchdog timer

    //Inicializaciones:
    init_ucs_24MHz();       //Ajustes del clock (Unified Clock System)
    init_uart();            //Inicialitzem la UART
    init_botons();          //Configuramos botones y leds
    init_LCD();             //Inicialitzem la pantalla LCD
    config_P7_LEDS();       //Inicialitzem els LEDs que es troben al port 7
    init_timer();           //Inicialitzem el timer
    init_clock();           //Inicialitzem el clock

    init_interrupciones();  //Configurar y activar las interrupciones de los botones
    halLcdPrintLine(saludo,linea, INVERT_TEXT); //escribimos saludo en la primera linea
    linea++; //Aumentamos el valor de linea y con ello pasamos a la linea siguiente

    //Bucle principal (infinito):
    do
    {

    if (estado_anterior != estado)          // Dependiendo del valor del estado se encender� un LED u otro.
    {
        sprintf(cadena,"Estado %02d", estado);  // Guardamos en cadena la siguiente frase: Estado "valor del estado",
                                                //con formato decimal, 2 cifras, rellenando con 0 a la izquierda.
        escribir(cadena,linea); // Escribimos la cadena al LCD

        estado_anterior = estado; // Actualizamos el valor de estado_anterior, para que no est� siempre escribiendo.

        moure_motor(MOTOR_2, SENTIT_CCW, MOVING_SPEED);

        /**********************************************************+
            A RELLENAR POR EL ALUMNO BLOQUE switch ... case
        Para gestionar las acciones:
        Boton S1, estado = 1
        Boton S2, estado = 2
        Joystick left, estado = 3
        Joystick right, estado = 4
        Joystick up, estado = 5
        Joystick down, estado = 6
        Joystick center, estado = 7
        ***********************************************************/
        switch(estado){

        case S1:
            //ENCENEM ELS 3 LEDS RGB

            P2OUT |= 0x40; //RED
            P2OUT |= 0x10; //GREEN
            P5OUT |= 0x40;  //BLUE

            break;

        case S2:
            direccio_LED = STOP;

            //APAGUEM ELS 3 LEDS RGB
            P2OUT &= ~0x40; // APAGUEM LED RED
            P2OUT &= ~0x10; // APAGUEM LED GREEN
            P5OUT &= ~0x40; //APAGUEM LED BLUE
            activate_led();
            //seg++;

            break;

        case Jstick_Center:

            //INVERTIM ESTAT: ENCES -> APAGAT O APAGAT -> ENCES
            //INVERTIM LED RED I GREEN
            P2OUT ^= 0x40;
            P2OUT ^= 0x10;
            //INVERTIM LED BLUE
            P5OUT ^= 0x40;

            break;

        case Jstick_Left:

            //ENCENEM ELS 3 LEDS RGB
            P2OUT |= 0x40; //RED
            P2OUT |= 0x10; //GREEN
            P5OUT |= 0x40;  //BLUE
            mov_left(retraso);
            //direccio_LED = MOV_LEFT;

            break;

        case Jstick_Right:
            
            //ApagarRGB_Launchpad();
            //direccio_LED = MOV_RIGHT;

            //APAGUEM LED BLUE I ENCENEM RED I GREEN
            P2OUT |= 0x40; //ENCENEM RED
            P2OUT |= 0x10; //ENCENEM GREEN
            P5OUT &= ~0x40;  //APAGUEM BLUE
            mov_right(retraso);
            //direccio_LED = MOV_RIGHT;

            break;

        case Jstick_Up:
            //ENCENEM BLUE I APAGUEM RED I GREEN
            
            P2OUT |= 0x40; //ENCENEM RED
            P2OUT &= ~0x10; //APAGUEM GREEN
            P5OUT |= 0x40;  //ENCENEM BLUE

            break;

        case Jstick_Down:
            //ENCENEM BLUE I GREEN I APAGUEM RED

            P2OUT &= ~0x40; //APAGUEM RED
            P2OUT |= 0x10; //ENCENEM GREEN
            P5OUT |= 0x40;  //ENCENEM BLUE

            break;

        default:
            break;
       }

    }

    }while(1); //Condicion para que el bucle sea infinito
}


