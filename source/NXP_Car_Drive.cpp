/*
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    NXP_Car_Drive.cpp
 * @brief   Application entry point.
 */
#include <car_drivers/PortExpander.h>
#include <car_drivers/UserInterface.h>
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "car_components.h"
#include "interrupt_handlers.h"

#define DIAG_TASK_PRIO (configMAX_PRIORITIES - 2)
#define CAM_TASK_PRIO (configMAX_PRIORITIES - 1)

static void motor_test_task(void *);
static void servo_test_task(void *pvParameters);
static void print_diagnostic_task(void *pvParameters);
static void user_interface_task(void *pvParameters);

static void camera_task(void *pvParameters);
static void motor_task(void *pvParameters);
static void servo_task(void *pvParameters);

//test drive
TaskHandle_t circle_handle;

/*
 * @brief   Application entry point.
 */
int main(void)
{

	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();

	//extra initializations that need to happen after peripherals are setup
	motor_l.init();
	motor_r.init();
	camera.init();
	camera.adc_base->SC1[0] |= ADC_SC1_AIEN_MASK;

	//change interrupt priorities to work with freeRTOS
	__NVIC_SetPriority(FTM1_IRQn,
					   ((configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)
						<< __NVIC_PRIO_BITS) -
						   1UL);
	__NVIC_SetPriority(ADC0_IRQn,
					   ((configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)
						<< __NVIC_PRIO_BITS) -
						   1UL);
	__NVIC_SetPriority(PIT0_IRQn,
					   ((configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)
						<< __NVIC_PRIO_BITS) -
						   1UL);

	//start the I2C port expander
	expander.begin();

	//this task prints diagnostic information to the console via UART periodically
	xTaskCreate(print_diagnostic_task, "Diagnostic task", configMINIMAL_STACK_SIZE + 100, NULL, DIAG_TASK_PRIO, NULL);

	//these tasks drive the car
	xTaskCreate(camera_task, "Camera", NUM_PIXELS * sizeof(uint16_t) * 2, NULL, CAM_TASK_PRIO, &camera.task_handle);
	xTaskCreate(servo_task, "Servo", configMINIMAL_STACK_SIZE + 100, NULL, CAM_TASK_PRIO, &servo.task_handle);
	xTaskCreate(motor_task, "Motor", configMINIMAL_STACK_SIZE, NULL, CAM_TASK_PRIO, &drive.task_handle);

	vTaskStartScheduler();
	for (;;)
		;
}

// process the camera's raw input to direct the response of the motors and servo
static void camera_task(void *pvParameters)
{
	camera.calibrate();
	for (;;)
	{
		camera.process();
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

// adjust motor output speed and direction while driving
static void motor_task(void *pvParameters)
{
	drive.set_motors(50);
	for (;;)
	{
		drive.update_from_camera(camera.camera_data.center);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

// steer the car while driving
static void servo_task(void *pvParameters)
{
	servo.set_position(servo.center_pulse_width);
	for (;;)
	{
		servo.position_from_camera(camera.camera_data.center);
		//this is the maximum update frequency for the servo because any faster is shorter than the control signal period
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

// spin the drive motors up and down in both directions
static void motor_test_task(void *pvParameters)
{
	Motor *motor_p = (Motor *)pvParameters;
	for (;;)
	{
		motor_p->motor_test();
	}
}

// steer the servo left and right to verify endpoints
static void servo_test_task(void *pvParameters)
{
	Servo *servo_p = (Servo *)pvParameters;
	for (;;)
	{
		servo_p->servo_test();
	}
}

// output debugging information via UART
static void print_diagnostic_task(void *pvParameters)
{
	for (;;)
	{
		PRINTF("motor_l: %d   motor_r: %d  servo: %d\r\n",
			   motor_l.getRotationSpeed(), motor_r.getRotationSpeed(), servo.current_pulse_width);
		PRINTF("outer_l: %d  inner_l: %d  center: %d inner_r: %d  outer_r: %d\r\n\r\n",
			   camera.camera_data.left_edge_outer,
			   camera.camera_data.left_edge_inner,
			   camera.camera_data.center,
			   camera.camera_data.right_edge_inner,
			   camera.camera_data.right_edge_outer);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/*
 * outputs the character sequence to display a simple user interface
 * on the LCD/button module
*/
void print_interface()
{
	interface.clear();
	interface.setCursor(0, 0);
	interface.print("TEST  CALIB  RUN");
	interface.setCursor(1, 1);
	interface.write(0x7F);
	interface.setCursor(8, 1);
	interface.write(0x5E);
	interface.setCursor(14, 1);
	interface.write(0x7E);
}

/*
 * handles user inputs to perform various tasks such as testing the actuators
 * and calibrating the camera thresholds for the current lighting conditions
 */
static void user_interface_task(void *pvParameters)
{
	interface.begin(16, 2, 0);
	print_interface();
	for (;;)
	{
		uint8_t button = interface.readButtons();
		if (button && button != 255)
		{
			interface.clear();
			interface.setCursor(0, 0);
			//start test
			if (button == BUTTON_LEFT)
			{
				interface.print("Running Test...");
				//start test task
				if ((eTaskGetState(motor_l.test_task_handle) == eSuspended) && (eTaskGetState(motor_r.test_task_handle) == eSuspended))
				{
					vTaskResume(motor_l.test_task_handle);
					vTaskResume(motor_r.test_task_handle);
				}
				if (eTaskGetState(servo.test_task_handle) == eSuspended)
				{
					vTaskResume(servo.test_task_handle);
				}
			}
			//start calibration
			else if (button == BUTTON_UP)
			{
				interface.print("Running Calibration");
				//start calibration task
			}
			//start running
			else if (button == BUTTON_RIGHT)
			{
				interface.print("Starting Car!");
				//run main driving task
				vTaskResume(&circle_handle);
			}
			//stop running
			else
			{
				interface.print("***ABORT***");
				motor_l.set_speed(0);
				motor_r.set_speed(0);
			}
			vTaskDelay(pdMS_TO_TICKS(1000));
			print_interface();
		}
		//lost i2c connection
		if (button == 255)
		{
			motor_l.set_speed(0);
			motor_r.set_speed(0);
		}
	}
}
