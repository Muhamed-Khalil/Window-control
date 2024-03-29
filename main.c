#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "tm4c123gh6pm.h"
#include "DIO.h"
#include "types.h"
#include "queue.h"
#include "semphr.h"

//define motor's direction
#define Motor_up 						GPIO_PORTA_DATA_R |= (1<<5)  
#define Motor_down					GPIO_PORTA_DATA_R |= (1<<6)

// To calculate pressed time on the button
TickType_t time_start;
TickType_t time_stop;

// Create Semaphores 
xSemaphoreHandle driver_open;
xSemaphoreHandle driver_close;
xSemaphoreHandle passenger_open;
xSemaphoreHandle passenger_close;
xSemaphoreHandle Key;

// Function to initialize Ports of Tiva c
void GPIO_Ports(void);

// Function to check if button pressed
void Buttons(void *pvParameters);

//control driver's direction dependant on the button pressed
void driver_open_fun(void *pvParameters);
void driver_close_fun(void *pvParameters);

void passenger_open_fun(void *pvParameters);
void passenger_close_fun(void *pvParameters);

void Automatic_mode_open(void);
void Automatic_mode_close(void);

int main(void)
{
	GPIO_Ports();		//initalize ports
	driver_open = xSemaphoreCreateBinary();
	driver_close = xSemaphoreCreateBinary();
	passenger_open = xSemaphoreCreateBinary();
	passenger_close = xSemaphoreCreateBinary();
	
	Key = xSemaphoreCreateMutex(); //create mutex to be used in functions
	
	// Create Tasks
	xTaskCreate(Buttons,"Check Buttons Pressed",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(driver_open_fun,"Window is opening from driver panel",configMINIMAL_STACK_SIZE,NULL,2,NULL);
	xTaskCreate(driver_close_fun,"Window is closing from driver panel",configMINIMAL_STACK_SIZE,NULL,2,NULL);
	xTaskCreate(passenger_open_fun,"Window is opening from passenger panel",configMINIMAL_STACK_SIZE,NULL,2,NULL);
	xTaskCreate(passenger_close_fun,"Window is closing from passenger panel",configMINIMAL_STACK_SIZE,NULL,2,NULL);
	
	// Start the scheduler
	vTaskStartScheduler();
	while(1){} // It should not get here
}


void GPIO_Ports(void){
	SYSCTL_RCGCGPIO_R|= 0x20; 								// enable clock for PORTF
  while((SYSCTL_PRGPIO_R & 0x20)!= 0x20); 	//actual clk value check 
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY; 				// entering port password
  GPIO_PORTF_CR_R = 0x1F; 									// commit both buttons (0,4) and 3 LED
  GPIO_PORTF_DIR_R = 0x0E;									// make 3 LED output pins 
  GPIO_PORTF_PUR_R = 0x11; 									// make buttons pull up
  GPIO_PORTF_DEN_R = 0x1F;									// enable both buttons (0,4) and 3 LED
	GPIO_PORTF_DATA_R = 0x00;      

	
	SYSCTL_RCGCGPIO_R|= 0x01; 								// enable clock for PORTF
  while((SYSCTL_PRGPIO_R & 0x01)!= 0x01); 	//actual clk value check 
  GPIO_PORTA_LOCK_R = GPIO_LOCK_KEY; 				// entering port password
  GPIO_PORTA_CR_R = 0x7C; 									// commit A2 A3 A4 A5 A6
  GPIO_PORTA_DIR_R = 0x64;									// make A2 output pin
  GPIO_PORTA_PUR_R = 0x18; 									// make buttons pull up
  GPIO_PORTA_DEN_R = 0x7C;									// enable A2 A3 A4 A5 A6
	GPIO_PORTA_DATA_R = 0x00;
	
	
SYSCTL_RCGCGPIO_R|= 0x02; 								// enable clock for PORTF
  while((SYSCTL_PRGPIO_R & 0x02)!= 0x02); 	//actual clk value check 
  GPIO_PORTB_LOCK_R = GPIO_LOCK_KEY; 				// entering port password
  GPIO_PORTB_CR_R = 0xEF; 									// commit B4 B6 B7
  GPIO_PORTB_DIR_R = 0x00;									// make  output pin
  GPIO_PORTB_PUR_R = 0xEF; 									// make buttons pull up
  GPIO_PORTB_DEN_R = 0xEF;									// enable A2 A3 A4
	GPIO_PORTB_DATA_R = 0x00;
	
}

void Buttons(void *pvParameters){
	while(1){
		//check if a button pressed 
		if((GPIOA->DATA & 0x08)== 0) xSemaphoreGive(driver_open);
		if((GPIOA->DATA & 0x10)== 0) xSemaphoreGive(driver_close);
		if((GPIOB->DATA & 0x40)== 0) xSemaphoreGive(passenger_open);
		if((GPIOB->DATA & 0x80)== 0) xSemaphoreGive(passenger_close);
	}
}

void driver_open_fun(void *pvParameters){
	while(1){
		xSemaphoreTake(driver_open, portMAX_DELAY);
		xSemaphoreTake(Key,portMAX_DELAY);
		if(!((GPIOB->DATA & 0x01)== 0)) { 						// Is limit switch not reached yet
			time_start = xTaskGetTickCount();						// Get the current tick count of the FreeRTOS task scheduler
			while(((GPIOA->DATA & 0x08)== 0) && !((GPIOB->DATA & 0x01)== 0)){ //check if buttons is pressed and limit switch not reached yet
				
				Motor_up; 
				GPIO_PORTF_DATA_R = 0x08; // Green led to indicate everything is working fine
			
			}
			time_stop = xTaskGetTickCount();					// Get the current tick count of the FreeRTOS task scheduler at the end of an operation
			if((time_stop - time_start) < 0x00000100) Automatic_mode_open(); // check if the time of press is short or long
			GPIO_PORTF_DATA_R = 0x00;
			GPIO_PORTA_DATA_R = 0x00;
			GPIO_PORTB_DATA_R = 0x00;
		}
		xSemaphoreGive(Key);
	}
}
void driver_close_fun(void *pvParameters){
	while(1){
		xSemaphoreTake(driver_close, portMAX_DELAY);
		xSemaphoreTake(Key,portMAX_DELAY);
		if(!((GPIOB->DATA & 0x02)== 0)) { 						// Is limit switch not reached yet
			time_start = xTaskGetTickCount();					 // Get the current tick count of the FreeRTOS task scheduler
			while(((GPIOA->DATA & 0x10)== 0) && !((GPIOB->DATA & 0x02)== 0)){//check if buttons is pressed and limit switch not reached yet
				Motor_down;
				GPIO_PORTF_DATA_R = 0x02;	// Red led to indicate everything is working fine
			
			}
			time_stop = xTaskGetTickCount();		// Get the current tick count of the FreeRTOS task scheduler at the end of an operation
			if((time_stop - time_start) < 0x00000100) Automatic_mode_close();// check if the time of press is short or long
			GPIO_PORTF_DATA_R = 0x00;
			GPIO_PORTA_DATA_R = 0x00;
			GPIO_PORTB_DATA_R = 0x00;
		}
		xSemaphoreGive(Key);
	}
}


void passenger_open_fun(void *pvParameters){
	while(1){
		xSemaphoreTake(passenger_open, portMAX_DELAY);
		xSemaphoreTake(Key,portMAX_DELAY);
		if(!((GPIOB->DATA & 0x01)== 0)) { 						// Is limit switch not reached yet
			if(!((GPIOB->DATA & 0x20)== 0)) { 					// Check if the switch is on or off
				time_start = xTaskGetTickCount();				  // Get the current tick count of the FreeRTOS task scheduler
				while(((GPIOB->DATA & 0x40)== 0) && !((GPIOB->DATA & 0x01)== 0) && !((GPIOB->DATA & 0x20)== 0) ){//check if buttons is pressed and limit switch not reached yet and lock switch is off
						Motor_up;
					GPIO_PORTF_DATA_R = 0x08;		// Green led to indicate everything is working fine
				}
				
				
				time_stop = xTaskGetTickCount();		// Get the current tick count of the FreeRTOS task scheduler at the end of an operation	
				if((time_stop - time_start) < 0x00000100) Automatic_mode_open(); // check if the time of press is short or long
				GPIO_PORTF_DATA_R = 0x00;
				GPIO_PORTA_DATA_R = 0x00;
				GPIO_PORTB_DATA_R = 0x00;
			}
		}
	xSemaphoreGive(Key);
	}
}

void passenger_close_fun(void *pvParameters){
	while(1){
		xSemaphoreTake(passenger_close, portMAX_DELAY);
		xSemaphoreTake(Key,portMAX_DELAY);
		if(!((GPIOB->DATA & 0x02)== 0)) { 						// Is limit switch not reached yet
			if(!((GPIOB->DATA & 0x20)== 0)) {           // Check if the switch is on or off
				time_start = xTaskGetTickCount();				  // Get the current tick count of the FreeRTOS task scheduler
				while(((GPIOB->DATA & 0x80)== 0) && !((GPIOB->DATA & 0x02)== 0) && !((GPIOB->DATA & 0x20)== 0)){//check if buttons is pressed and limit switch not reached yet and lock switch is off
					Motor_down;
					GPIO_PORTF_DATA_R = 0x02;		// Red led to indicate everything is working fine
				
				}
				time_stop = xTaskGetTickCount();		// Get the current tick count of the FreeRTOS task scheduler at the end of an operation	
				if((time_stop - time_start) < 0x00000100) Automatic_mode_close();		// check if the time of press is short or long
				GPIO_PORTF_DATA_R = 0x00;
				GPIO_PORTA_DATA_R = 0x00;
				GPIO_PORTB_DATA_R = 0x00;
			}
		}
		xSemaphoreGive(Key);
	}
}
void Automatic_mode_open(void){
	while(!((GPIOB->DATA & 0x01)== 0)&& !((GPIOB->DATA & 0x04)== 0)){//check limit switch not reached yet and jamming button is not pressed
		Motor_up; 
		GPIO_PORTF_DATA_R = 0x08;			// Green led to indicate everything is working fine
		
	}
	if (((GPIOB->DATA & 0x04)== 0)){// check if jamming button is pressed
				GPIO_PORTF_DATA_R = 0x00;
				GPIO_PORTA_DATA_R = 0x00;	//stop the motor
				Motor_down; 
					GPIO_PORTF_DATA_R = 0x04;
				for(int i=0;i<500000;i++);// delay of 0.5 seconds
					GPIO_PORTB_DATA_R = 0x00;
			GPIO_PORTF_DATA_R = 0x00;
				GPIO_PORTA_DATA_R = 0x00;//stop motor
				}
}

void Automatic_mode_close(void){
	while(!((GPIOB->DATA & 0x02)== 0)){//check limit switch not reached yet
		Motor_down; 
		GPIO_PORTF_DATA_R = 0x02;
	}
	
}


