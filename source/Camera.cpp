/*
 * Camera.cpp
 *
 *  Created on: Feb 21, 2019
 *      Author: Jack
 */

#include <Camera.h>

void Camera::init() {
	//set gpio pins as output
	gpio_pin_config_t gpioconfig;
	gpioconfig.pinDirection = kGPIO_DigitalOutput;
	gpioconfig.outputLogic = 0;
	GPIO_PinInit(gpio_base, si_pin, &gpioconfig);
	GPIO_PinInit(gpio_base, clk_pin, &gpioconfig);

	//enable the ADC
	adc16_channel_config_t adcconfig;
	adcconfig.channelNumber = 0;
	adcconfig.enableInterruptOnConversionCompleted = true;
	adcconfig.enableDifferentialConversion = false;
	ADC16_SetChannelConfig(adc_base, 0, &adcconfig);
}
