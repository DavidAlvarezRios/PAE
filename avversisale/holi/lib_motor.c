/*
 * lib_motor.c
 *
 *  Created on: 7 de maig 2019
 *      Author: mat.aules
 */

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

void increase_speed(void)
{
    if(velocidad < MAX_SPEED){
        velocidad += 100;
    }

}

void decrease_speed(void)
{
    if(velocidad > 0){
        velocidad -= 100;
    }
}

void stop(void)
{
    byte parametres[3] = {0x20, 0, 0};
    TxPacket(MOTOR_2, 0x03, WRITE, parametres);
    RxPacket();
    TxPacket(MOTOR_3, 0x03, WRITE, parametres);
    RxPacket();
}

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
