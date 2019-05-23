/*
 * lib_motor.h
 *
 *  Created on: 7 de maig 2019
 *      Author: mat.aules
 */

#ifndef LIB_MOTOR_H_
#define LIB_MOTOR_H_
//#pragma once

#include "lib_UART.h"

#define WRITE 0x03
#define READ 0x02
#define MOTOR_2 0x02
#define MOTOR_3 0x03
#define SENSOR 0x64

#define MAX_SPEED 500
#define FORWARD 0x01
#define BACKWARD 0x00
#define LEFT 0x00
#define RIGHT 0x02
#define CENTER 0x01

// ID'S SENSORS
#define SENSOR_LEFT 0x1A
#define SENSOR_CENTER 0x1B
#define SENSOR_RIGHT 0x1C

// DIRECCIONS MOTOR
#define CW 0x04
#define CCW 0x00

// VELOCITATS
#define MOVING_SPEED 0x20



uint8_t s_left;
uint8_t s_center;
uint8_t s_right;

uint8_t s_left_ant;
uint8_t s_center_ant;
uint8_t s_right_ant;


void move_forward(void);
void move_backward(void);
void move_left(void);
void move_right(void);
void increase_speed(void);
void decrease_speed(void);
void stop(void);
void print_distance(uint8_t distance, uint8_t sensor);
void change_detection_distance(void);
void read_sensors(void);
struct RxReturn obstacle_detected(void);
void print_obstacle(struct RxReturn atope);


#endif /* LIB_MOTOR_H_ */
