/*
 * lib_motor.c
 *
 *  Created on: 7 de maig 2019
 *      Author: mat.aules
 */


//#include "lib_UART.h"
#include "lib_motor.h"




uint16_t velocidad = 250;
char cadena[16];//Una linea entera con 15 caracteres visibles + uno oculto de terminacion de cadena (codigo ASCII 0)



/**
 * Funcio que mou el robot cap a endavant
 */


void move_forward(void)
{
    byte parametres[3] = {0x20, velocidad, (velocidad >> 8)|CW};
    TxPacket(MOTOR_2, 0x03, WRITE, parametres);
    RxPacket();
    byte parametres2[3] = {0x20, velocidad, (velocidad >> 8)|CCW};
    TxPacket(MOTOR_3, 0x03, WRITE, parametres2);
    RxPacket();

}
/**
 * Funcio que mou el robot cap a enrere
 */
void move_backward(void)
{
    byte parametres[3] = {0x20, velocidad, (velocidad >>8)|CCW};
    TxPacket(MOTOR_2, 0x03, WRITE, parametres);
    RxPacket();
    byte parametres2[3] = {0x20, velocidad, (velocidad >> 8)|CW};
    TxPacket(MOTOR_3, 0x03, WRITE, parametres2);
    RxPacket();
}
/**
 * Funcio que mou el robot cap a la esquerra
 */
void move_left(void)
{
    byte parametres[3] = {0x20, velocidad, (velocidad >> 8)|CW};
    TxPacket(MOTOR_2, 0x03, WRITE, parametres);
    RxPacket();
    TxPacket(MOTOR_3, 0x03, WRITE, parametres);
    RxPacket();
}
/**
 * Funcio que mou el robot cap a la dreta.
 */
void move_right(void)
{
    byte parametres[3] = {0x20, velocidad, (velocidad >> 8)|CCW};
    TxPacket(MOTOR_2, 0x03, WRITE, parametres);
    RxPacket();
    TxPacket(MOTOR_3, 0x03, WRITE, parametres);
    RxPacket();
}

/**
 * Funcio que incrementa en 100 la velocitat
 */
void increase_speed(void)
{
    if(velocidad < MAX_SPEED){
        velocidad += 100;
    }

}

/**
 * Funcio per decrementar en 100 la velocitat
 */
void decrease_speed(void)
{
    if(velocidad > 0){
        velocidad -= 100;
    }
}

/**
 * Funcio que para els motors
 */
void stop(void)
{
    byte parametres[3] = {0x20, 0, 0};
    TxPacket(MOTOR_2, 0x03, WRITE, parametres);
    RxPacket();
    TxPacket(MOTOR_3, 0x03, WRITE, parametres);
    RxPacket();
}

/**
 * Funcio per imprimir per la pantalla LCD les dades dels sensor
 *
 * Dades d'entrada: distancia, ID del sensor (1 esquerra, 2 central, 3 dreta)
 *
 * Dades sortida: cap
 */
void print_distance(uint8_t distance, uint8_t sensor)
{

    switch(sensor){

    case 1:
        sprintf(cadena,"Left %03d", distance);
        escribir(cadena,3); // Escribimos la cadena al LCD
        break;
    case 2:
        sprintf(cadena,"Center %03d", distance);
        escribir(cadena,4); // Escribimos la cadena al LCD
        break;
    case 3:
        sprintf(cadena,"Right %03d", distance);
        escribir(cadena,5); // Escribimos la cadena al LCD
        break;

    }


}

void change_detection_distance(void)
{

    byte parametres[2] = {0x34,0x32};
    TxPacket(SENSOR, 0x02, WRITE, parametres);
}


/**
 * Funció per demanar informació als sensors.
 */
void read_sensors(void)
{
    struct RxReturn resposta;
    byte parametres_left[2] = {SENSOR_LEFT, 1};
    TxPacket(SENSOR, 0x02, READ, parametres_left);
    resposta = RxPacket();
    print_distance(resposta.StatusPacket[5], 1);
    byte parametres_center[2] = {SENSOR_CENTER, 1};
    TxPacket(SENSOR, 0x02, READ, parametres_center);
    resposta = RxPacket();
    print_distance(resposta.StatusPacket[5], 2);
    byte parametres_right[2] = {SENSOR_RIGHT, 1};
    TxPacket(SENSOR, 0x02, READ, parametres_right);
    resposta = RxPacket();
    print_distance(resposta.StatusPacket[5], 3);
}

struct RxReturn obstacle_detected(void)
{
    struct RxReturn resposta;
    byte parametres[2] = {0x20,1};
    TxPacket(SENSOR, 0x02, READ, parametres);
    resposta = RxPacket();
    return resposta;
}


void print_obstacle(struct RxReturn resposta)
{

    sprintf(cadena,"%03d", resposta.StatusPacket[5]);
    escribir(cadena,7); // Escribimos la cadena al LCD

}










