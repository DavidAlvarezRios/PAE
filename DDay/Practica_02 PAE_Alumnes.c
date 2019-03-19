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

char saludo[16] = " PRACTICA 3 PAE";//max 15 caracteres visibles
char cadena[16];//Una linea entera con 15 caracteres visibles + uno oculto de terminacion de cadena (codigo ASCII 0)
char borrado[] = "               "; //una linea entera de 15 espacios en blanco

uint8_t linea = 1;
uint8_t estado=0;
uint8_t estado_anterior = 8;
uint16_t count_ms; //contador milisegons
uint32_t retraso = 200;
//uint32_t tiempo = 0;
uint8_t seg,min,hora = 0; // segons minuts hora actuals

uint8_t modifica_hora = 1;
uint8_t modifica_alarma = 0;

uint8_t min_alarma = 1;
uint8_t hora_alarma = 0;
uint8_t seg_alarma = 0;

uint8_t flag_retraso = 0; //0 modificamos retraso, 1 no lo modificamos se cambia con Jstick_Center

uint8_t cambio_estado = 0; //esta variable sirve para cambiar la hora, minuto (0, 1)

//Necesitamos una variable para almacenar los valores de la velocidad (Cuando movemos el JoyStick hacia arriba o hacia abajo)
// Y tambien necesitamos otra variable para saber si los LEDs se enciended de derecha a izquierda o al reves
// (Cuando movemos el joystick hacia la derecha o hacia la izquierda)

uint8_t velocitat = 0;
uint8_t direccio_LED = 0;


/**************************************************************************
 * INICIALIZACI�N DEL CONTROLADOR DE INTERRUPCIONES (NVIC).
 *
 * Sin datos de entrada
 *
 * Sin datos de salida
 *
 **************************************************************************/
void init_interrupciones(){
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

    __enable_interrupt(); //Habilitamos las interrupciones a nivel global del micro.
}

/**********************************************************
* Con esta función inicializaremos y configuraremos el timer ACLK
**********************************************************/
void init_timer(void){

    TA0CTL |= TASSEL__ACLK;
    TA0CCR0 = 32;
    // Activamos las interrupciones del tiemr
    TA0CCTL0 |= CCIE;
    TA0CCTL0 &= ~CCIFG;

    // Establecemos el limite de tiempo del timer a ~1ms


    // Establecemos la frequencia a 2^15Hz


}

// TODO: COMENTAR
void init_clock(void){
    

    TA1CTL |= TASSEL__ACLK;
    TA1CTL |= MC__UP;
    TA1CCR0 = 32000;
    //Definimos el valor del registro TA1CCTL0
    TA1CCTL0 |= CCIE;
    TA1CCTL0 &= ~CCIFG;


    //Definimos el registro TA1CCR0 con el numero de pulsos (1 segundo)


    //definimos el valor del registro TA1CTL

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
        //count_ms +=1;
    }while (count_ms < temps);

    TA0CTL |= MC__STOP; //Parem el timer posant-lo a mode STOP
}

void comprobar_alarma(void)
{
    if(hora == hora_alarma && min == min_alarma)
    {
        sprintf(cadena, "ALARMA!!!"); // Guardamos en cadena la siguiente frase: ALARMA!!!
        escribir(cadena, linea+5);          // Escribimos la cadena al LCD
    }
}

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

void TA0_0_IRQHandler(void)
{
    TA0CCTL0 &= ~CCIE; //inhabilitem la interrupcio
    count_ms += 1; //Aumentamos el contador para saber cu�nto tiempo ha transcurrido en funci�n del tiempo que tarde en llamarse dicha interrupci�n.
    TA0CCTL0 &= ~CCIFG;
    TA0CCTL0 |= CCIE; //Habilitem la interrupció
}

void TA1_0_IRQHandler(void)
{
    TA1CCTL0 &=~CCIE;//Inhabilitem la interrupci0
    seg += 1; //contador segundos
    TA1CCTL0 &= ~CCIFG;
    TA1CCTL0 |= CCIE;//Habilitem la interrupcio
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


void ApagarRGB_Launchpad(){
    P2OUT &= ~0x50;
    P5OUT &= ~0x40;
}

void main(void)
{

    WDTCTL = WDTPW+WDTHOLD;         // Paramos el watchdog timer

    //Inicializaciones:
    init_ucs_24MHz();       //Ajustes del clock (Unified Clock System)
    init_botons();          //Configuramos botones y leds
    init_LCD();
    config_P7_LEDS(); //Inicialitzem els LEDs que es troben al port 7
    init_timer();
    init_clock(); // Inicializamos la pantalla

    init_interrupciones();  //Configurar y activar las interrupciones de los botones
    halLcdPrintLine(saludo,linea, INVERT_TEXT); //escribimos saludo en la primera linea
    linea++;                    //Aumentamos el valor de linea y con ello pasamos a la linea siguiente

    //Bucle principal (infinito):
    do
    {
        imprimir_temps();
        comprobar_alarma();

    if (estado_anterior != estado)          // Dependiendo del valor del estado se encender� un LED u otro.
    {
        sprintf(cadena,"Estado %02d", estado);  // Guardamos en cadena la siguiente frase: Estado "valor del estado",
                                                //con formato decimal, 2 cifras, rellenando con 0 a la izquierda.
        escribir(cadena,linea); // Escribimos la cadena al LCD

        sprintf(cadena, "a: %02d:%02d:%02d", hora_alarma, min_alarma,seg_alarma); // Guardamos en cadena la siguiente frase: alarma hora:min
        escribir(cadena, linea+3);          // Escribimos la cadena al LCD
        estado_anterior = estado; // Actualizamos el valor de estado_anterior, para que no est� siempre escribiendo.

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

            if(modifica_alarma == 0){
                modifica_alarma = 1;
                modifica_hora = 0;

            }else{
                modifica_alarma = 0;
                modifica_hora = 1;
            }
            


            break;

        case S2:
            direccio_LED = STOP;

            //APAGUEM ELS 3 LEDS RGB
            P2OUT &= ~0x40; // APAGUEM LED RED
            P2OUT &= ~0x10; // APAGUEM LED GREEN
            P5OUT &= ~0x40; //APAGUEM LED BLUE

            //seg++;

            if (cambio_estado == 0){
                cambio_estado = 1;
            }else{
                cambio_estado = 0;
            }

            borrar(linea+5);
            break;

        case Jstick_Center:

            //INVERTIM ESTAT: ENCES -> APAGAT O APAGAT -> ENCES
            //INVERTIM LED RED I GREEN
            P2OUT ^= 0x40;
            P2OUT ^= 0x10;
            //INVERTIM LED BLUE
            P5OUT ^= 0x40;

            if(flag_retraso == 0)
            {
                flag_retraso = 1;
            }else{
                flag_retraso = 0;
            }

            break;

        case Jstick_Left:

            //direccio_LED = MOV_LEFT;

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



            if(flag_retraso == 0)
            {

                if(retraso < 1000)
                {
                    retraso += 10;
                }
            }else{
                if(modifica_hora)
                {
                    incrementar_hora();
                    //modificar_alarma = 0;
                }
                else if(modifica_alarma)
                {
                    incrementar_alarma();
                    //modifica_hora = 0;
                }
            }


            break;

        case Jstick_Down:
            //ENCENEM BLUE I GREEN I APAGUEM RED

            P2OUT &= ~0x40; //APAGUEM RED
            P2OUT |= 0x10; //ENCENEM GREEN
            P5OUT |= 0x40;  //ENCENEM BLUE



            if(flag_retraso == 0){

                if(retraso > 0)
                {
                    retraso -= 10;
                }
            }else{
                if(modifica_hora){
                    decrementar_hora();
                }
                else if(modifica_alarma)
                {
                    decrementar_alarma();
                }
            }

            break;

        default:
            break;
       }

        sprintf(cadena, "retras: %04d", retraso); // Mostramos por pantalla el retraso actual. MAX 3000
        escribir(cadena, linea+1);

        if(flag_retraso == 0)
        {
            borrar(linea+4);
            sprintf(cadena, "canviant delay"); // Guardamos en cadena la siguiente frase: alarma hora:min
            escribir(cadena, linea+4);
        }else{
            if(modifica_alarma)
            {
                borrar(linea+4);
                sprintf(cadena, "modifica alarma"); // Guardamos en cadena la siguiente frase: alarma hora:min
                escribir(cadena, linea+4);          // Escribimos la cadena al LCD
            }

            if(modifica_hora)
            {
                borrar(linea+4);
                sprintf(cadena, "modifica temps"); // Guardamos en cadena la siguiente frase: alarma hora:min
                escribir(cadena, linea+4);          // Escribimos la cadena al LCD
            }
        }



        if(cambio_estado == 1){

            borrar(linea+6);
            sprintf(cadena, "canviant mins.."); // Guardamos en cadena la siguiente frase: alarma hora:min
            escribir(cadena, linea+6);          // Escribimos la cadena al LCD
        }else{

            borrar(linea+6);
            sprintf(cadena, "canviant hora.."); // Guardamos en cadena la siguiente frase: alarma hora:min
            escribir(cadena, linea+6);          // Escribimos la cadena al LCD
        }


    }

    }while(1); //Condicion para que el bucle sea infinito
}
