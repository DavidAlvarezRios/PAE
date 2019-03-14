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
uint16_t count_ms;
uint32_t retraso = 1000;
uint32_t tiempo = 0;
uint8_t seg,min,hora = 0;
uint8_t alarma_ho,alarma_min = 0;
uint8_t selec_alarma_ho,selec_alarma_min = 0;
uint8_t pos_actual,pos_alarma=0;
uint8_t estado_alarma = 0;
uint8_t suena_alarma = 0;
uint8_t modo_robot = 0;
uint16_t freq = 24000;
float retras_sumat = 0.0;

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

    __enable_interrupt(); //Habilitamos las interrupciones a nivel global del micro.
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
 * DELAY - A CONFIGURAR POR EL ALUMNO - con bucle while
 *
 * Datos de entrada: Tiempo de retraso. 1 segundo equivale a un retraso de 1000000 (aprox)
 *
 * Sin datos de salida
 *
 **************************************************************************/
void delay_t (uint32_t temps)
{
   volatile uint32_t i;


   TA0CTL |= 0x0210; //Habilitamos un modo diferente al STOP

   i = 0;

   do
   {
       i++;
   }while( i < temps);

   TA0CTL |= 0x0200; //Volvemos al stop.


}

void reloj(void){
    seg += 1;
    if(seg == 60){
        seg = 0;
        min += 1;
        if(min == 60){
            min = 0;
            min += 1;
            if(hora == 24){
                hora = 0;
            }
        }
    }
    if(estado_alarma == 1){
        if(min == selec_alarma_min){
            if(hora == selec_alarma_ho){
                sprintf(cadena, "Alarma %02d:%02d", hora, min);
                escribir(cadena, linea+4);
            }
        }
    }
    sprintf(cadena,"%02d:%02d:%02d",hora,min,seg);
    escribir(cadena,linea+2);
}

void cambio (uint8_t posicion){
    TA1CTL |= 0x0200;
    if (posicion == 0){
        hora += 1;
        hora %= 24;
    }
    else if (posicion == 1){
        min += 1;
        min %= 60;
    }
    else{
        seg += 1;
        seg %= 60;
    }
    TA1CTL |= 0x0210;


}
void mostrar_modo(uint8_t modo,uint8_t linea){

    if (modo == 0){
        sprintf(cadena," Modo:LED");    // Guardamos en cadena la siguiente frase: estado "valor del estado"
        escribir(cadena,linea+1);
    }else{
        sprintf(cadena," Modo:Alarma");    // Guardamos en cadena la siguiente frase: estado "valor del estado"
        escribir(cadena,linea+1);
    }

}

void seleccio_alarma(uint8_t posicio_alarma){

    if (posicio_alarma == 0){
        alarma_min += 1;
        alarma_min %= 60;
    }
    else if (posicio_alarma == 1){
        alarma_ho += 1;
        alarma_ho %= 24;
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
                if(modo_robot == 1){
                    modo_robot = ~ modo_robot;
                }
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

void TA0_0_IRQHandler(void){
    TA0CCTL0 &= ~CCIE;
    count_ms += 1; //Aumentamos el contador para saber cu�nto tiempo ha transcurrido en funci�n del tiempo que tarde en llamarse dicha interrupci�n.
    TA0CCTL0 &= ~CCIFG;
    TA0CCTL0 |= CCIE;
}

void TA0_1_IRQHandler(void){
    TA1CCTL0 &=~CCIE;
    tiempo += 1;
    if (tiempo > 1000){ // Establecemos que, hasta que no hayan pasado 1000 unidades de tiempo, no se ponga en marcha el cambio en el reloj. 
        tiempo = 0;
        reloj();

    }

    TA1CCTL0 &= ~CCIFG;
    TA1CCTL0 |= CCIE;
}

void mov_right(uint32_t temps)
{
    for(P7OUT = 1; P7OUT < 128; P7OUT <<= 1){
        delay_t(temps*250);
    }
    delay_t(temps*250);
    P7OUT = 0;


}

void mov_left(uint32_t temps)
{
    int i;
    for(P7OUT = 128; P7OUT > 0;  P7OUT >>= 1){

        delay_t(temps*250);
    }
    delay_t(temps*250);
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
    init_botons();         //Configuramos botones y leds
    init_interrupciones();  //Configurar y activar las interrupciones de los botones
    init_LCD(); // Inicializamos la pantalla
    config_P7_LEDS(); //Inicialitzem els LEDs que es troben al port 7

    halLcdPrintLine(saludo,linea, INVERT_TEXT); //escribimos saludo en la primera linea
    linea++;                    //Aumentamos el valor de linea y con ello pasamos a la linea siguiente

    //Bucle principal (infinito):
    do
    {
        mostrar_modo(modo_robot,linea);
    if (estado_anterior != estado)          // Dependiendo del valor del estado se encender� un LED u otro.
    {
        sprintf(cadena,"Estado %02d", estado);  // Guardamos en cadena la siguiente frase: Estado "valor del estado",
                                                //con formato decimal, 2 cifras, rellenando con 0 a la izquierda.
        escribir(cadena,linea); // Escribimos la cadena al LCD
        estado_anterior = estado; // Actualizamos el valor de estado_anterior, para que no est� siempre escribiendo.

        sprintf(cadena,"%f",(freq/24000)+retras_sumat);
        escribir(cadena,linea+5);

        sprintf(cadena, "Hora: %02d:%02d", hora, min);
        escribir(cadena, linea+2);

        sprintf(cadena,"Alarma %02d:%02d",selec_alarma_ho,selec_alarma_min);
        escribir(cadena,linea+3);

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
            if(modo_robot == 0){
                ApagarRGB_Launchpad();
                P2OUT |= 0x40; //RED
                P2OUT |= 0x10; //GREEN
                P5OUT |= 0x40;  //BLUE
            }else{
                pos_alarma += 1;
                pos_alarma %= 2;
            }
            break;

        case S2:
            direccio_LED = STOP;

            //APAGUEM ELS 3 LEDS RGB
            if(modo_robot == 0){
                ApagarRGB_Launchpad();
                P2OUT &= ~0x40; // APAGUEM LED RED
                P2OUT &= ~0x10; // APAGUEM LED GREEN
                P5OUT &= ~0x40; //APAGUEM LED BLUE
            }else{
                seleccio_alarma(pos_alarma);
            }
            break;

        case Jstick_Center:

            if(modo_robot == 0){
                ApagarRGB_Launchpad();
                //INVERTIM ESTAT: ENCES -> APAGAT O APAGAT -> ENCES
                //INVERTIM LED RED I GREEN
                P2OUT ^= 0x40;
                P2OUT ^= 0x10;
                //INVERTIM LED BLUE
                P5OUT ^= 0x40;
            }else{

                selec_alarma_min = min;
                selec_alarma_ho = hora;
                estado_alarma = 1;
                sprintf(cadena,"Alarma %02d:%02d",selec_alarma_ho,selec_alarma_min);
                escribir(cadena,linea+3);
            }
            break;

        case Jstick_Left:

            if(modo_robot == 0){
                ApagarRGB_Launchpad();
                direccio_LED = MOV_LEFT;

                //ENCENEM ELS 3 LEDS RGB
                P2OUT |= 0x40; //RED
                P2OUT |= 0x10; //GREEN
                P5OUT |= 0x40;  //BLUE
            }else{
                 pos_actual -= 1;
                 pos_actual %= 3;
            }
            break;

        case Jstick_Right:
            if(modo_robot == 0){
                ApagarRGB_Launchpad();
                direccio_LED = MOV_RIGHT;

                //APAGUEM LED BLUE I ENCENEM RED I GREEN
                P2OUT |= 0x40; //ENCENEM RED
                P2OUT |= 0x10; //ENCENEM GREEN
                P5OUT &= ~0x40;  //APAGUEM BLUE
            }else{
                pos_actual += 1;
                pos_actual %= 3;
            }
            break;

        case Jstick_Up:
            //ENCENEM BLUE I APAGUEM RED I GREEN
            if(modo_robot == 0){

                ApagarRGB_Launchpad();
                P2OUT |= 0x40; //ENCENEM RED
                P2OUT &= ~0x10; //APAGUEM GREEN
                P5OUT |= 0x40;  //ENCENEM BLUE

                if(velocitat < 1000){
                    velocitat = velocitat +100; //Augmentem la velocitat dels LEDs
                }

                if(retraso < 10000){ // En este caso augmentaremos la variable retraso, eso s� fijando un maximo.
                      retraso += 250;
                      retras_sumat += 0.25;
                }

            }else{
                cambio(pos_actual);
            }
            break;

        case Jstick_Down:
            //ENCENEM BLUE I GREEN I APAGUEM RED
            if(modo_robot == 0){
                velocitat = velocitat - 100;//Disminuim la velocitat dels LEDs
                ApagarRGB_Launchpad();
                if(velocitat < 0){ //No volem que la velocitat sigui negativa
                    velocitat = 0;
                }

                if(retraso > 0){ // En este caso augmentaremos la variable retraso, eso s� fijando un maximo.
                      retraso -= 250;
                      retras_sumat -= 0.25;
                }


                P2OUT &= ~0x40; //APAGUEM RED
                P2OUT |= 0x10; //ENCENEM GREEN
                P5OUT |= 0x40;  //ENCENEM BLUE
            }

            modo_robot = ~ modo_robot; //cambiamos estado del robot

            break;

        default:
            break;
       }

       if(direccio_LED != STOP && velocitat != 0){

           if(direccio_LED == MOV_RIGHT){
               mov_right(retraso / velocitat);


           }
           else if(direccio_LED == MOV_LEFT){
               mov_left(retraso / velocitat);
           }
       }

        
        borrar(linea);
    }

     //P2OUT ^= 0x40;     // Conmutamos el estado del LED R (bit 6)
     //delay_t(retraso);  // periodo del parpadeo
     //P2OUT ^= 0x10;     // Conmutamos el estado del LED G (bit 4)
     //delay_t(retraso);  // periodo del parpadeo
     //P5OUT ^= 0x40;     // Conmutamos el estado del LED B (bit 6)
     //delay_t(retraso);  // periodo del parpadeo

    }while(1); //Condicion para que el bucle sea infinito
}
