/*
 * car_components.h
 * 
 * This file contains declarations of all of the objects used to drive the car.
 *
 *  Created on: Feb 5, 2019
 *      Author: Jack
 */

#ifndef CAR_COMPONENTS_H_
#define CAR_COMPONENTS_H_

#include <car_drivers/Camera.h>
#include <car_drivers/Motor.h>
#include <car_drivers/Servo.h>
#include <car_drivers/PortExpander.h>
#include <car_drivers/UserInterface.h>
#include "MotorDrive.h"

extern Motor motor_l;
extern Motor motor_r;

extern MotorDrive drive;

extern Servo servo;

extern Camera camera;

extern PortExpander expander;
extern User_Interface interface;

#endif /* CAR_COMPONENTS_H_ */
