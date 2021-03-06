/******************************
 * 
 * Practica_02_PAE Programaci� de Ports
 * i pr�ctica de les instruccions de control de flux:
 * "do ... while", "switch ... case", "if" i "for"
 * UB, 02/2017.
 *****************************/

#include <stdio.h>
#include <stdint.h>
#include "lib_motor.h"


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

#define LED_MOTOR 0x19
#define ENCENDER_LED 1

#define LEFT_DIR 0
#define RIGHT_DIR 1
#define FORWARD_DIR 2
#define BOTH 3
#define ALL 4


char saludo[16] = " FINAL PAE";//max 15 caracteres visibles
char cadena[16];

uint8_t linea = 1;
uint8_t estado=0;
uint8_t estado_anterior = 8;

uint32_t retraso = 200;

//Necesitamos una variable para almacenar los valores de la v (Cuando movemos el JoyStick hacia arriba o hacia abajo)
// Y tambien necesitamos otra variable para saber si los LEDs se enciended de derecha a izquierda o al reves
// (Cuando movemos el joystick hacia la derecha o hacia la izquierda)

#define MAX_SPEED 500
uint8_t velocitat = 0;
uint8_t direccio_LED = 0;


uint8_t detected = 9;
byte encontrar_pared = 0;
byte menu = 1;
byte action = 0;
byte calibra = 0;

byte center_sensor = 15;
byte left_sensor = 15;
byte right_sensor = 15;

byte min_center_sensor = 200;
byte min_left_sensor = 200;
byte min_right_sensor = 200;

byte option = 0;
byte option_sub = 0;
byte calibrar = 0;

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
    // Activamos las interrupciones del timer
    TA0CCTL0 |= CCIE;
    TA0CCTL0 &= ~CCIFG;

}

void init_clock(void)
{

    TA1CTL |= TASSEL__ACLK;
    TA1CCR0 = 32000; //Aquest valor es l'equivalent en hertz mes o menys a un milisegon
    // Activamos las interrupciones del tiemr
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
        if(action){
            if(velocidad > 0){
                velocidad -= 50;
            }
        }else if(calibrar && !calibra){
            calibra = 1;
        }



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
                if(action){
                    if(velocidad < MAX_SPEED){
                        velocidad += 50;
                    }
                }
                break;
            }

    /***********************************************
     * HASTA AQUI BLOQUE CASE
     ***********************************************/

    P5IE |= 0x32;   //interrupciones Joystick y S1 en port 5 reactivadas
}

void drift(void)
{
    struct RxReturn obstacle;
    obstacle = obstacle_detected();
    print_obstacle(obstacle);

    switch(obstacle.StatusPacket[5]){
        case 0:
            detected = FORWARD_DIR;
            move_forward();
            break;
        case 1:
            detected = LEFT_DIR;
            move_forward();
            break;
        case 2:
            detected = LEFT_DIR;
            delay_t(1000*2);
            move_right();
            delay_t(1000*2);
            break;
        case 3:
            detected = LEFT_DIR;
            delay_t(1000*2);
            move_right();
            delay_t(1000*2);
            break;
        case 4:
            detected = FORWARD_DIR;
            move_forward();
            break;
        case 5:
            detected = BOTH;
            move_forward();
            break;
        case 6:
            detected = RIGHT_DIR;
            delay_t(1000*2);
            move_left();
            delay_t(1000*2);
            break;
        case 7:
            detected = ALL;
            move_backward();
            break;
    }
}

void move_to(uint8_t direction)
{
    switch(direction)
    {
    case 0:
        move_forward();
        break;
    case 1:
        move_right();
        break;
    case 2:
        move_left();
        break;
    case 3:
        move_backward();
        break;

    }
}

void changed(void)
{
    struct RxReturn obstacle;
    obstacle = obstacle_detected();
    print_obstacle(obstacle);

    while(obstacle.StatusPacket[5] == 0)
    {
        obstacle = obstacle_detected();
        print_obstacle(obstacle);

        switch(detected)
        {
            case RIGHT_DIR:
                move_to(2);
                break;
            case LEFT_DIR:
                move_to(1);
                break;
            case FORWARD_DIR:
                move_to(0);
                break;
            case BOTH:
                move_to(0);
                break;
            case ALL:
                move_to(3);
                break;
        }
    }

}


void follow_left(byte left, byte center, byte right)
{
    if(!encontrar_pared)
    {
        move_forward();
        delay_t(500);
        if(center > center_sensor || (center > center_sensor && left > left_sensor ))
        {
            encontrar_pared = 1;

        }
    }else{

        if(center < center_sensor && left < left_sensor && right < right_sensor)
        {
            delay_t(1000);
            move_left();
            delay_t(500);
            move_forward();

        }

        else if(center > center_sensor || (center > center_sensor && left > left_sensor ))
        {
            delay_t(1000);
            move_right();
            delay_t(1000);
            move_forward();
            if(center > 150)
            {
                move_backward();
            }

        }

        else if(left > left_sensor)
        {
            move_forward();
            delay_t(1000);
            if(left > 150)
            {
                move_right();
                delay_t(500);
            }
            //comprobar distancia, si se acerca mucho, se gira a la derecha.
        }
    }
}

void follow_right(byte left, byte center, byte right)
{
    if(!encontrar_pared)
    {
        move_forward();
        delay_t(500);
        if(center > center_sensor || (center > center_sensor && right > right_sensor ))
        {
            encontrar_pared = 1;

        }
    }else{

        if(center < center_sensor && left < left_sensor && right < right_sensor)
        {
            delay_t(1000);
            move_right();
            delay_t(500);
            move_forward();

        }

        else if(center > center_sensor || (center > center_sensor && right > right_sensor ))
        {
            delay_t(1500);
            move_left();
            delay_t(1500);
            move_forward();
            if(center > 150)
            {
                move_backward();
            }

        }
        else if(right > right_sensor)
        {
            move_forward();
            delay_t(1500);
            if(right > 150)
            {
                move_left();
                delay_t(500);
            }
            //comprobar distancia, si se acerca mucho, se gira a la derecha.
        }
    }
}

void set_distance_center(byte distance)
{
    center_sensor = distance;
    //min_center_sensor = min_center_sensor - distance;
}

void set_distance_left(byte distance)
{
    left_sensor = distance;
    //min_left_sensor = min_left_sensor - distance;
}

void set_distance_right(byte distance)
{
    right_sensor = distance;
    //min_right_sensor = min_right_sensor - distance;
}

void menu_principal(void)
{
    sprintf(cadena,"Calibrar");
    if(option == 0){
        halLcdPrintLine(cadena, linea+3, INVERT_TEXT); //escribimos saludo en la primera linea
    }else{
        halLcdPrintLine(cadena, linea+3, NORMAL_TEXT); //escribimos saludo en la primera linea
    }
    sprintf(cadena,"Follow Left");
    if(option == 1){
        halLcdPrintLine(cadena, linea+4, INVERT_TEXT); //escribimos saludo en la primera linea
    }else{
        halLcdPrintLine(cadena, linea+4, NORMAL_TEXT); //escribimos saludo en la primera linea
    }
    sprintf(cadena,"Follow Right");
    if(option == 2){
        halLcdPrintLine(cadena, linea+5, INVERT_TEXT); //escribimos saludo en la primera linea
    }else{
        halLcdPrintLine(cadena, linea+5, NORMAL_TEXT); //escribimos saludo en la primera linea
    }

}

void borrar_menu(void)
{
    borrar(linea+3);
    borrar(linea+4);
    borrar(linea+5);
}

void sub_menu(void)
{
    sprintf(cadena,"Calibrar Left");
    if(option_sub == 0){
        halLcdPrintLine(cadena, linea+3, INVERT_TEXT); //escribimos saludo en la primera linea
    }else{
        halLcdPrintLine(cadena, linea+3, NORMAL_TEXT); //escribimos saludo en la primera linea
    }
    sprintf(cadena,"Calibrar Center");
    if(option_sub == 1){
        halLcdPrintLine(cadena, linea+4, INVERT_TEXT); //escribimos saludo en la primera linea
    }else{
        halLcdPrintLine(cadena, linea+4, NORMAL_TEXT); //escribimos saludo en la primera linea
    }
    sprintf(cadena,"Calibrar Right");
    if(option_sub == 2){
        halLcdPrintLine(cadena, linea+5, INVERT_TEXT); //escribimos saludo en la primera linea
    }else{
        halLcdPrintLine(cadena, linea+5, NORMAL_TEXT); //escribimos saludo en la primera linea
    }
    sprintf(cadena,"Atras");
    if(option_sub == 3){
        halLcdPrintLine(cadena, linea+6, INVERT_TEXT); //escribimos saludo en la primera linea
    }else{
        halLcdPrintLine(cadena, linea+6, NORMAL_TEXT); //escribimos saludo en la primera linea
    }
}

void borrar_sub_menu(void)
{
    borrar(linea+3);
    borrar(linea+4);
    borrar(linea+5);
    borrar(linea+6);
}

void borrar_lineas(void){
    borrar(linea+2);
    borrar(linea+3);
    borrar(linea+4);
    borrar(linea+5);
    borrar(linea+6);
    borrar(linea+7);
}

void borrar_sensores(void)
{
    borrar(3);
    borrar(4);
    borrar(5);
}

void main(void)
{

    WDTCTL = WDTPW+WDTHOLD;         // Paramos el watchdog timer

    //Inicializaciones:
    init_ucs_24MHz();       //Ajustes del clock (Unified Clock System)
    init_botons();          //Configuramos botones y leds
    init_timer();           //Inicialitzem el timer
    init_clock();
    init_uart();
    init_interrupciones();  //Configurar y activar las interrupciones de los botones//Inicialitzem la UART
    init_LCD();//Inicialitzem la pantalla LCD

    halLcdPrintLine(saludo,linea, INVERT_TEXT); //escribimos saludo en la primera linea
    linea++; //Aumentamos el valor de linea y con ello pasamos a la linea siguiente


    //struct RxReturn obstacle;
    struct RxReturn distances;

    encontrar_pared = 0;
    //Bucle principal (infinito):

    //change_detection_distance();
    do
    {

        //drift();
        //changed();
        //byte left = distances.StatusPacket[5];
        //byte center = distances.StatusPacket[6];
        //byte right = distances.StatusPacket[7];

        //follow_left(left, center, right, obstacles);
        //follow_right(left, center, right);

        if (estado_anterior != estado)  
        {
            //sprintf(cadena,"Estado %02d", estado);  // Guardamos en cadena la siguiente frase: Estado "valor del estado",
                                                    //con formato decimal, 2 cifras, rellenando con 0 a la izquierda.
            //escribir(cadena,linea); // Escribimos la cadena al LCD

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

            if(menu){
                menu_principal();
            }else if(calibrar){
                sub_menu();
            }


            switch(estado){
            case S1:

                if(menu)
                {
                    //menu_principal();
                    switch(option)
                    {
                    case 0:
                        calibrar = 1;
                        menu = 0;
                        borrar_menu();
                        sub_menu();
                        break;
                    case 1:
                        menu = 0;
                        action = 1;
                        borrar_menu();
                        while(estado != Jstick_Center)
                        {

                            distances = read_sensors();
                            byte left = distances.StatusPacket[5];
                            byte center = distances.StatusPacket[6];
                            byte right = distances.StatusPacket[7];
                            print_distance(left, center, right);

                            follow_left(left, center, right);
                        }
                        action = 0;
                        menu = 1;

                        break;
                    case 2:

                        menu=0;
                        borrar_menu();
                        while(estado != Jstick_Center)
                        {

                            distances = read_sensors();
                            byte left = distances.StatusPacket[5];
                            byte center = distances.StatusPacket[6];
                            byte right = distances.StatusPacket[7];
                            print_distance(left, center, right);

                            follow_right(left, center, right);
                        }
                        menu = 1;
                        break;

                    }

                }else if(calibrar){
                    sub_menu();
                    byte left;
                    byte center;
                    byte right;
                    action = 0;
                    switch(option_sub){
                    case 0:
                        borrar_sub_menu();
                        while(!calibra){

                            distances = read_sensors();
                            left = distances.StatusPacket[5];
                            print_distance_sensor(left, 1);


                        }
                        borrar(4);
                        set_distance_left(left);
                        calibra = 0;
                        sub_menu();
                        break;
                    case 1:
                        borrar_sub_menu();
                        while(!calibra){

                            distances = read_sensors();
                            center = distances.StatusPacket[6];
                            print_distance_sensor(center, 2);

                        }
                        borrar(4);
                        set_distance_center(center);
                        calibra = 0;
                        sub_menu();
                        break;
                    case 2:
                        borrar_sub_menu();
                        while(!calibra){

                            distances = read_sensors();
                            right = distances.StatusPacket[7];
                            print_distance_sensor(right, 3);
                        }
                        borrar(4);
                        set_distance_right(right);
                        calibra = 0;
                        sub_menu();
                        break;
                    case 3:
                        calibrar = 0;
                        menu = 1;
                        borrar_sub_menu();
                        menu_principal();
                        break;
                    }

                }

                break;
            case S2:
                //decrease_speed();
                break;
            case Jstick_Center:
                stop();
                if(menu){
                    borrar_sensores();
                }
                break;
            case Jstick_Left:
                
                break;
            case Jstick_Right:
                
                break;
            case Jstick_Up:
                if(menu){
                    option = option - 1;
                    if(option == 255){
                        option = 0;
                    }
                }else if(calibrar){
                    option_sub = option_sub - 1;
                    if(option_sub == 255){
                        option_sub = 0;
                    }
                }

                break;
            case Jstick_Down:
                if(menu)
                    option = (option + 1)%3;
                else if(calibrar)
                    option_sub = (option_sub + 1)%4;
                
                break;
            default:
                break;
           }
        }

    }while(1); //Condicion para que el bucle sea infinito
}


