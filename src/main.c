/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "main.h"

#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "slew.h"
#include "ADC_example.h"
#include "I2C_controller.h"
#include "Si7006.h"
//#include "FSM.h"
#include "status_leds.h"
#include "INA226.h"
#include "PWM_out.h"
#include "stm32f0xx_tim.h"
#include "Buffer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/*
 * I2C2_semaphore_control is used when a task want to take control of the I2C2 bus
 * This does not necessarily mean that the task is using the I2C2 bus it just
 * means that it has a use for it and needs to block it from other tasks
 */
static SemaphoreHandle_t I2C2_semaphore_control;


void vApplicationTickHook( void )
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */
}
/*-----------------------------------------------------------*/

void CreateSemaphores(void){
	//The I2C2 Semaphore to safeguard tasks from overlapping usage of the bus
	I2C2_semaphore_control = xSemaphoreCreateBinary();
	//Just double check that initialized successfully
	configASSERT(I2C2_semaphore_control);
	//For some reason the semaphore start empty so we must free it
	xSemaphoreGive(I2C2_semaphore_control);
}

int main(void)
{

	blink_led_C8_C9_init();
	timer16_it_config_48MHz_to_1Hz();


	DAC_init();

	/*
	SetClockForADC();
	SetClockForADC();
	CalibrateADC();
	EnableADC();
	ConfigureADC();
	ConfigureTIM15();
	ADC1->CR |= ADC_CR_ADSTART;
	*/

	timer1_A7_A8_config();

	Configure_GPIO_I2C2();
	Configure_I2C2_Master();

	init_Si700X();

	init_INA226();


	//init_FSM();


	//Configure_GPIO_USART1();
	//Configure_USART1();

	//safety checks

	vBootTaskInit();
	CreateSemaphores();

	//System boot

	//reconfigure

	//System go

	/* Start the kernel.  From here on, only tasks and interrupts will run. */
	vTaskStartScheduler();

	//This should never happen
	while(1);
}


void bootUpSeq(void *dummy){
	//inital boot delay
	vTaskDelay(200/portTICK_PERIOD_MS);

	//Start the slew start
	static uint16_t dac_mv = 0;
	while(1){
		while(dac_mv < 3300){
			dac_mv += 100;
			DAC_change_milli_volt(dac_mv);
			vTaskDelay(10/portTICK_PERIOD_MS);
		}
		vGeneralTaskInit();
		vTaskDelete(NULL);
	}
}

void updateSI700X(void *dummy){
	enum {
		Humidity,
		Temperature,
		update_item_size
	};

	static uint8_t next_update = Humidity;

	while(1){
		// Wait until we are able to take control of the I2C2 bus
		while(xSemaphoreTake(I2C2_semaphore_control, (TickType_t) 1000) == pdFALSE);
		//Delay to ensure that the I2C2 bus will not cause error
		vTaskDelay(TEMPERATURE_DELAY_TIME_MS/10);
		//Check to make sure we are configured to read the temperature

		if(next_update == Humidity){
			//Set the temperature read
			//INA226_set_fet_volt_ptr();
		}else{

		}

		//Si700X_set_temp_read_over_I2C();
		/* Block to wait for prvTask2() to notify this task. */
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		vTaskDelay(TEMPERATURE_DELAY_TIME_MS/10);

		//Now that we know the system is configured correctly execute temperature read
		//INA226_exec_volt_read();
		//wait until this blocked tasks is released
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		//We are now done with the I2C2 bus
		//Release the I2C2 bus before sleeping
		xSemaphoreGive(I2C2_semaphore_control);
		//Delay the task until it's time to read again
		vTaskDelay(TEMPERATURE_DELAY_TIME_MS*5);
	}
}

void secondTierUpdates(void *dummy){
	//the last value must be the size!!!!!
	enum{
		humidity = 0,
		temperature,
		FET_volt,
		Bat_volt,
		Curr1,
		Curr2,
		update_item_size
	};
	static uint8_t updating_item = FET_volt;
	while(1){
		// Wait until we are able to take control of the I2C2 bus
		while(xSemaphoreTake(I2C2_semaphore_control, (TickType_t) 1000) == pdFALSE);
		//Delay to ensure that the I2C2 bus will not cause error
		vTaskDelay(1/portTICK_PERIOD_MS);

		if(updating_item == humidity){
			//Set the temperature read
			//Si700X_set_humidity_read_over_I2C();
		}else if(updating_item == temperature){
			//Set the temperature read
			//Si700X_set_temp_read_over_I2C();
		}else if(updating_item == FET_volt){
			INA226_set_volt_ptr(FET_Voltage_sensor);
		}else if(updating_item == Bat_volt){
			INA226_set_volt_ptr(Bat_Voltage_sensor);
		}else if(updating_item == Curr1){
			INA226_set_curr_ptr(FET_Voltage_sensor);
		}else if(updating_item == Curr2){
			INA226_set_curr_ptr(Bat_Voltage_sensor);
		}else{
			while(1);
		}

		/* Block to wait for prvTask2() to notify this task. */
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		vTaskDelay(1/portTICK_PERIOD_MS);

		if(updating_item == humidity){
			//Set the temperature read
			//Si700X_exec_humidty_read_over_I2C();
		}else if(updating_item == temperature){
			//Set the temperature read
			//Si700X_exec_temp_read_over_I2C();
		}else if(updating_item == FET_volt){
			INA226_exec_volt_read(FET_Voltage_sensor);
		}else if(updating_item == Bat_volt){
			INA226_exec_volt_read(Bat_Voltage_sensor);
		}else if(updating_item == Curr1){
			INA226_exec_curr_read(FET_Voltage_sensor);
		}else if(updating_item == Curr2){
			INA226_exec_curr_read(Bat_Voltage_sensor);
		}else{
			while(1);
		}

		//wait until this blocked tasks is released
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		updating_item = ((updating_item + 1) % update_item_size);
		//We are now done with the I2C2 bus
		//Release the I2C2 bus before sleeping
		xSemaphoreGive(I2C2_semaphore_control);
		//Delay the task until it's time to read again
		vTaskDelay(TEMPERATURE_DELAY_TIME_MS/portTICK_PERIOD_MS);
	}
}

void blinkyTask(void *dummy){
	while(1){
		GPIOA->ODR ^= GPIO_ODR_3;
		/* maintain LED3 status for 200ms */
		vTaskDelay(500);
	}
}

void vBootTaskInit(void){
	xTaskCreate(bootUpSeq,
		(const signed char *)"boot",
		configMINIMAL_STACK_SIZE,
		NULL,                 // pvParameters
		tskIDLE_PRIORITY + 1, // uxPriority
		NULL               );///* pvCreatedTask */
}

void vGeneralTaskInit(void){
    xTaskCreate(blinkyTask,
		(const signed char *)"blinkyTask",
		configMINIMAL_STACK_SIZE,
		NULL,                 // pvParameters
		tskIDLE_PRIORITY + 1, // uxPriority
		NULL              ); // pvCreatedTask */
    /*xTaskCreate(updateVoltages,
			(const signed char *)"FetVoltage",
			configMINIMAL_STACK_SIZE * 2,
			NULL,                 // pvParameters
			tskIDLE_PRIORITY + 1, // uxPriority
			NULL              );  // pvCreatedTask*/
    xTaskCreate(secondTierUpdates,
		(const signed char *)"tier_two",
		configMINIMAL_STACK_SIZE,
		NULL,                 // pvParameters
		tskIDLE_PRIORITY + 1, // uxPriority
		NULL              );  // pvCreatedTask*/
}

void TIM16_IRQHandler(void)//Once per second
{
	if (TIM_GetITStatus(TIM16, TIM_IT_Update) != RESET){
		GPIOC->ODR ^= GPIO_ODR_9;
		TIM_ClearITPendingBit(TIM16, TIM_IT_Update);
	}
}
