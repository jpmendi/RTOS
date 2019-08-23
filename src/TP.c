/*=============================================================================
 * Author: Juan Pablo Menditto <jpmenditto@gmail.com>
 * Date: 2019/07/20
 * Version: 1.0
 *===========================================================================*/

/*==================[inlcusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"   //Motor del OS
#include "FreeRTOSConfig.h"
#include "task.h"		//Api de control de tareas y temporización
#include "semphr.h"		//Api de sincronización (sem y mutex)

// sAPI header
#include "sapi.h"
#include <string.h>


/*==================[definiciones y macros]==================================*/

#define UART_PC        UART_USB
#define UART_BLUETOOTH UART_232


/*==================[definiciones de datos internos]=========================*/
SemaphoreHandle_t Evento_pulsado, Mutex_t_pulsado, Mutex_UART; //Para el uso de semaforos
portTickType TiempoPulsado;  //Para la periodicidad
/*==================[definiciones de datos externos]=========================*/

uint8_t control = 0; //variable que indica la instruccion a seguir en el switch
DEBUG_PRINT_ENABLE;  //Para configurar los mensajes por monitor


/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

void ReceiveBT ( void* taskParmPtr ); //definicion de tarea que Enlaza y recibe las señales de bluetooth

void pwmMotor( void* taskParmPtr );  //Tarea que establece los valores de PWM y activación / desactivacion

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main(void)
{
   uint8_t Error_state = 0;  //variable local para registrar errores

   // ---------- CONFIGURACIONES ------------------------------
   // Inicializar y configurar la plataforma
   boardConfig();

   // Inicializar UART_USB para conectar a la PC
   uartConfig( UART_PC, 9600 );
   uartWriteString( UART_PC, "UART_PC configurada.\r\n" );

   // Inicializar UART_232 para conectar al modulo bluetooth
   uartConfig( UART_BLUETOOTH, 9600 );
   uartWriteString( UART_PC, "UART_BLUETOOTH para modulo Bluetooth configurada.\r\n" );


   /* Attempt to create a semaphore. */
   if (NULL == (Evento_pulsado = xSemaphoreCreateBinary()))   // la crea y comprueba al mismo tiempo
   {
	   Error_state =1;
   }

   if (NULL == (Mutex_t_pulsado = xSemaphoreCreateMutex()))
   {
   	   Error_state =1;
   }

   if (NULL == (Mutex_UART = xSemaphoreCreateMutex()))
   {
	   Error_state =1;
   }

   // Crear tarea ReceiveBT en freeRTOS

   xTaskCreate(
     ReceiveBT,                     // Funcion de la tarea a ejecutar
	 (const char *)"ReceiveBT",     // Nombre de la tarea como String amigable para el usuario
	 configMINIMAL_STACK_SIZE*4, // Cantidad de stack de la tarea
	 0,                          // Parametros de tarea
	 tskIDLE_PRIORITY+1,         // Prioridad de la tarea
	 0                           // Puntero a la tarea creada en el sistema
     );

  // Crear tarea pwmMotor en freeRTOS

  xTaskCreate(
     pwmMotor,                     // Funcion de la tarea a ejecutar
	 (const char *)"pwmMotor",     // Nombre de la tarea como String amigable para el usuario
	 configMINIMAL_STACK_SIZE*2, // Cantidad de stack de la tarea
	 0,                          // Parametros de tarea
	 tskIDLE_PRIORITY+1,         // Prioridad de la tarea
	 0                           // Puntero a la tarea creada en el sistema
     );

   // Iniciar scheduler
   if (0 == Error_state)
      {
  	   vTaskStartScheduler();
      }
   else
      {
	  printf("Error al iniciar el sistema !!!!!!!!!!!!!!");
      }


   // ---------- REPETIR POR SIEMPRE --------------------------
   while( TRUE )
   {
      // Si cae en este while 1 significa que no pudo iniciar el scheduler
   }

   // NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
   // directamenteno sobre un microcontroladore y no es llamado por ningun
   // Sistema Operativo, como en el caso de un programa para PC.
   return 0;
}

/*==================[definiciones de funciones internas]=====================*/

/*==================[definiciones de funciones externas]=====================*/


// Implementacion de funcion de la tarea ReceiveBT


void ReceiveBT ( void* taskParmPtr )
{
	   uint8_t data = 0; //variable para registrar la entrada desde el celular por BT
	   // ---------- REPETIR POR SIEMPRE --------------------------
 while( TRUE )
	  {
     // Si leo un dato de una UART lo envio a al otra (bridge)
     if( uartReadByte( UART_PC, &data ) )   //para ver lo que envia el BT
       {
	    uartWriteByte( UART_BLUETOOTH, data );
       }

   if( uartReadByte( UART_BLUETOOTH, &data ) ) //Si se Reciben de datos por BT en cada caso utilizo el semaforo
	    //para modificar la variable y luego lo libero.
     {
      if( data == 'C' )
      {
         xSemaphoreTake(Mutex_UART,portMAX_DELAY);
         control = 1;
         xSemaphoreGive(Mutex_UART);
      }

      if( data == 'A' )
      {
          xSemaphoreTake(Mutex_UART,portMAX_DELAY);
          control = 2;
          xSemaphoreGive(Mutex_UART);
      }

      if( data == 'B' )
      {
      	  xSemaphoreTake(Mutex_UART,portMAX_DELAY);
          control = 3;
          xSemaphoreGive(Mutex_UART);
      }

      if( data == 'D' )
      {
          xSemaphoreTake(Mutex_UART,portMAX_DELAY);
          control = 4;
          xSemaphoreGive(Mutex_UART);
      }
      uartWriteByte( UART_PC, data );

	  //  vTaskDelayUntil( &xLastWakeTime, xPeriodicity ); Lo iba autilizar a aca pero no es necesario,
      //Son instantaneas cada recepcion de BT
      }
	}
}

// Implementacion de funcion de la tarea pwmMotor
void pwmMotor(void* taskParmPtr)
	{
	bool_t initTim = 0; //var para chequear que se inicializo ok el timer para pwm
	bool_t initPin = 0; // var para chequear que se inicializo ok el pin
	bool_t valor0 = 0;  //aux para testeo
	bool_t valor1 = 0;  //aux para testeo
	bool_t test = 0;  //aux para testeo
	bool_t test1 = 0;  //aux para testeo
	bool_t test2 = 0;  //aux para testeo
	bool_t fT1 = 1,fT2 = 0,fT3 = 0,fT4 = 0;

	int16_t  dutyCycleInit = 0; // ciclo de trabajo inicial
	int16_t  dutyCycle0 = 0; // ciclo transitorio
	int16_t  dutyCycle1 = 128;
	int16_t  dutyCycleTest = 0;
	int16_t pwmVal = 0; // otra variale por si quiero leer el valor del pin
	int16_t control2 = 0; //variable que indica la instruccion en el switch

	   portTickType xPeriodicity =  400 / portTICK_RATE_MS;
	   portTickType xLastWakeTime = xTaskGetTickCount();

	/* Configurar PWM */

	initTim = pwmConfig( 0, PWM_ENABLE ); //inicializa el timer para los pulsos con Frecuencia predet 1Khz
	while(1) //Aca configuro para que se puedan utilizar las teclas como alternativa al BT
	{
		valor0 = !gpioRead( TEC1 ); //valor = true (!false), si la tecla esta a GD es decir presionada la (si la tecla esta en true=1, esta levantada)
		if (valor0)
		   {
			control = 1;
	       }

		valor1 = !gpioRead( TEC2 ); //valor = true (!false), si la tecla esta a GD es decir presionada la (si la tecla esta en true=1, esta levantada)
		if (valor1)
		   {
			control = 2;
		   }

		valor0 = !gpioRead( TEC3 ); //valor = true (!false), si la tecla esta a GD es decir presionada la (si la tecla esta en true=1, esta levantada)
		if (valor0)
		   {
			control = 3;
	       }

		valor1 = !gpioRead( TEC4 ); //valor = true (!false), si la tecla esta a GD es decir presionada la (si la tecla esta en true=1, esta levantada)
		if (valor1)
		   {
			control = 4;
		   }


        xSemaphoreTake(Mutex_UART,portMAX_DELAY);
		int8_t control2 = control;
		xSemaphoreGive(Mutex_UART);

		//Segun lo ingresado es la accion que ejecuta el PWM
		//USO DE LEDS TESTIGOS PARA EVALUAR PROGRAMA
		// LED R ------------------> SE INICIA PROGRAMA
		// LED 1 (ROJO)------------> EL PWM ESTA DESHABILITADO (SALVO QUE VERDE ESTE ENCENDIDO)
		// LED 2 (AMARILLO)--------> SE LLEGO AL MAXIMO PWM (250)
		// LED 3 (VERDE------------> EL PWM ESTA HABILITADO
		// LED 1 Y 3 AL MISMO TIEMPO -----> SE LLEGO AL MINIMO DE PWM (0)

		switch (control2)
		  {

		    case 1: //Se presionó la tecla deshabilitar por mas que se presionen
		    	    //las otras no deberia funcionar el PWM
		   	  gpioWrite( LED2, FALSE); //apago led de maximo pwm
		   	  pwmWrite( PWM6, dutyCycleInit ); //primero lo pongo en cero por si venia con algun valor
		      dutyCycle0 = 0;
		      test1 = pwmConfig( PWM6, PWM_DISABLE_OUTPUT );// deshabilito PWM
		      if (test1) //verifica que se deshabilito y prende un led y apaga el de habilitado
		          {
		    	  gpioWrite( LED1, TRUE); //pwm deshabilitado
	              gpioWrite( LED3, FALSE);
		          }
			  fT1 = TRUE;  //es un flag de estado
			  control = 0;  //vuelvo a cero para reiniciar
			break;

		    case 2:       //esta funcion habilita el PWM y lo pone en cero
		    	          //activa un flag que permite luego acelerar desacelerar

		      gpioWrite( LED2, FALSE); //apago led de maximo pwm
			  test2 = pwmConfig( PWM6, PWM_ENABLE_OUTPUT ); // testeo de que se habilitó
		   	  pwmWrite( PWM6, dutyCycleInit ); // pone PWM en cero
		      dutyCycle0 = 0;
		      vTaskDelayUntil( &xLastWakeTime, xPeriodicity ); // espera 400ms para que se establezca el PWM

		      if (test2) //verifica que se habilito y prende un led y apaga el de deshabilitado
		          {
		    	  gpioWrite( LED3, TRUE); //PWM Habilitado
	              gpioWrite( LED1, FALSE);
		          }
			  fT1 = FALSE;  // NO ESTA DESHABILITADO
			  control = 0; //vuelvo a cero para reiniciar

			break;

		    case 3:

		      if(fT1 == FALSE)  //esta habilitado para incrementar,
			    {
				   if(dutyCycle0 == 0) //si el estado anterior era cero establece un PWM inicial
				   {
					  gpioWrite( LED1, FALSE);   //Apaga el de led deshabilitado
					  dutyCycle0 = 10;              //es el valor inicial luego de que pwm sea cero
				      vTaskDelayUntil( &xLastWakeTime, xPeriodicity );
				   }
				   else
					{
					  if(dutyCycle0 < 250) //si tenia un valor mayor a cero y menor a 250(deberia ser siempre menor)
						  {
							gpioWrite( LED1, FALSE);  //Apago el pwm deshabilitado
							gpioWrite( LED2, FALSE); //apago led de maximo pwm
							dutyCycle0 = dutyCycle0 + 10;
							pwmWrite( PWM6, dutyCycle0 );
							control = 0;
						    vTaskDelayUntil( &xLastWakeTime, xPeriodicity );
						   }
	 	               else
	 	            	  gpioWrite( LED2, TRUE);
			            	break;
					}

				gpioWrite( LEDR, dutyCycle0);
				control = 0;
			    }
   			break;


		    case 4:

			  if(fT1 == FALSE)  //idem anterior pero para desacelerar
			  {
				  if(dutyCycle0 <= 10)
				     {
					  pwmWrite( PWM6, dutyCycleInit );
					  gpioWrite( LED1, dutyCycle0);  // Me indica que llegue al PWM minimo
					  vTaskDelayUntil( &xLastWakeTime, xPeriodicity );
					  control = 0;
					  break;            //no se puede desacelerar menos de cero
				     }
				  else
				  {
					    gpioWrite( LED2, FALSE);
					    dutyCycle0 = dutyCycle0 - 10;
						pwmWrite( PWM6, dutyCycle0 );
					    vTaskDelayUntil( &xLastWakeTime, xPeriodicity );
				  }
				gpioWrite( LEDR, dutyCycle0);
				control = 0;
			  }

			break;


	     }
	 }

	}




/*==================[fin del archivo]========================================*/
