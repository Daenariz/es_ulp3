/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef uint8_t crc;
typedef enum {FALSE, TRUE} BOOL;
enum
{
  C_WHITE, C_RED, C_GREEN, C_BLUE,
  C_CYAN, C_MAGENTA, C_YELLOW, C_BROWN,
  C_LIME, C_OLIVE, C_ORANGE, C_PINK,
  C_PURPLE, C_TEAL, C_VIOLET, C_MAUVE,
};

typedef union
{
  struct
  {
    uint8_t b; // blue
    uint8_t r; // red
    uint8_t g; // green
  } color; // color
  uint32_t data;
} PixelRGB_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define debounceDelay 150

//Elevator stuff
#define MOTORGPIO GPIOC
#define stay 0
#define up 1
#define down 2

#define MMCP_MASTER_ADDRESS 0
#define MMCP_VERSION 6
#define L7_PDU_size 9
#define L7_SDU_size 8
#define L7_PCI_size 1
#define L3_PDU_size 13
#define L3_SDU_size 9
#define L3_PCI_size 4
#define L2_PDU_size 14
#define L2_SDU_size 13
#define L2_PCI_size 1
#define L1_PDU_size 16
#define L1_SDU_size 14
#define L1_PCI_size 2

#define NUM_NEIGHBOURS 4
#define STORAGE_SIZE 6
#define RGB_DATA_LEN (LED_COUNT * 24)
#define LAGER_SIZE 6

// CRC defines
#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL 0x9b

/* LED stuff */
#define NEOPIXEL_ZERO 34
#define NEOPIXEL_ONE 67

/*Neighbours*/
#define NEIGHBOUR_ID_SOUTH 0
#define NEIGHBOUR_ID_NORTH 0
#define NEIGHBOUR_ID_EAST 0
#define NEIGHBOUR_ID_WEST 55

#define otherElevatorAddress 55
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim2_ch3_up;
DMA_HandleTypeDef hdma_tim2_ch2_ch4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
const uint16_t neighbourSendPins[NUM_NEIGHBOURS] = {to_S_SV4_Pin, to_N_SV8_Pin, to_O_SV3_Pin, to_W_SV5_Pin};
const uint8_t neighbourIDs[NUM_NEIGHBOURS] = {
	NEIGHBOUR_ID_SOUTH,
	NEIGHBOUR_ID_NORTH,
    NEIGHBOUR_ID_EAST,
    NEIGHBOUR_ID_WEST
};
uint8_t L7_SDU[L7_SDU_size];
// Speicher
uint8_t state;
uint8_t errorId;
uint8_t partnerId;
uint8_t packageId;
uint8_t fromId;
uint8_t toId;
uint8_t storage[6] = {0};
uint8_t tempStorage[6] = {0};
uint8_t ApNr;
uint8_t direction;
uint32_t myAddress = 63;
uint8_t NUM_PIXELS = 8;
uint8_t DMA_BUFF_SIZE = (8 * 24) + 1;

// Kontrollfluesse
bool receive = false;
bool passOn = false;
bool create = false;
bool deliver = false;
bool poll = false;
bool await = false;
bool finishedSend = false;
bool finishedStore = false;
bool failure = false;
bool receivedSDU = false;
bool GPIO_neighbour_in = false;
bool pushedButton = false;
bool positionReached = false;

//LED Sachen
uint32_t dmaBuffer[193] = {0};
uint32_t *pBuff;
uint8_t LEDColors[17][3] = { {0, 0, 0}, {255, 255, 255},  {255, 0, 0},
		 {0, 255, 0},  {0, 0, 255},  {0, 255, 255},
		 {255, 0, 255},  {255, 255, 0},  {191, 128, 64},
		 {191, 255, 0},  {128, 128, 0},  {255, 128, 0},
		 {255, 191, 191},  {191, 0, 64},  {0, 128, 128},
		 {128, 0, 128},  {224, 176, 255} };
PixelRGB_t pixels[8] = {0};
BOOL timer_irq = FALSE; // gets set HIGH every 100ms
PixelRGB_t leds[4] = {0};

// Zustandstyp + Zustandsvariable
typedef enum {Z_idle, Z_processing, Z_failure, Z_deliver, Z_passOn, Z_sent, Z_awaiting, Z_received, Z_elevate} zustand_t;
zustand_t zustand;

// Aktionentyp + Aktionsvariable
typedef enum {A_idle, A_setup,A_updateLager, A_await, A_create, A_deliver, A_passOn, A_forwardAwait, A_failure, A_pulse, A_handleStore, A_storeAwait, A_storeCreate, A_checkFailure, A_forwardAgain, A_elevate} aktion_t;
aktion_t aktion;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_GPIO_EXTI_Callback ( uint16_t GPIO_Pin );

void L1_receive(uint8_t L1_PDU[L1_PDU_size]);
void L2_receive(uint8_t L2_PDU[L2_PDU_size]);
void L3_receive(uint8_t L3_PDU[L3_PDU_size]);
void L7_receive(uint8_t L7_PDU[L7_PDU_size]);

void L7_send(uint8_t Id, uint8_t L7_SDU[L7_SDU_size]);
void L3_send(uint8_t L3_SDU[L3_SDU_size]);
void L2_send(uint8_t L2_SDU[L2_SDU_size]);
void L1_send(uint8_t L1_SDU[L1_SDU_size]);

void ApNr_42(uint8_t L7_SDU[], uint8_t L7_SDU_send[]);
void ApNr_43(uint8_t L7_SDU[], uint8_t L7_SDU_send[]);
void ApNr_44(uint8_t L7_SDU[], uint8_t L7_SDU_send[]);
void ApNr_50(uint8_t L7_SDU[], uint8_t L7_SDU_send[]);

void HAL_Delay(uint32_t Delay);
void stateProcessing(void);
void stateAwait(void);
void stateReceived(void);
void stateSent(void);
void stateFailure(void);
void handleStore(void);
void handleSend(void);
void updateLager(void);
void animateSentStorage(void);
void animateReceiveStorage(void);
void animateCreate(void);
void animateDeliver(void);
void animateSentElevator(void);
void animateReceiveElevator(void);
void pulse(void);
void handleForward(void);
void resetData(void);
void forwardReset(void);
void resetAufzug(void);
void Aufzugfahr(void);
void resetPulse(void);
void resetLEDs(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_3);
  HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_2);
}

void stateProcessing(void) {
	state = 0;
}

void stateAwait(void) {
	state = 1;
}

void stateReceived(void) {
	state = 2;
}

void stateSent(void) {
	state = 3;
}

void stateFailure(void) {
    state = 4;
}

//void simulateButtonPress(void) {
//    HAL_GPIO_EXTI_Callback(B1_Pin);
//}
void resetPulse(){
	GPIO_neighbour_in = false;
}
//HIER DIE LED BEGINN
void writeLEDs(PixelRGB_t* pixel){
	int i,j;

	pBuff = dmaBuffer;
	  for (i = 0; i < NUM_PIXELS; i++)
	  {
		 for (j = 23; j >= 0; j--)
		 {
		   if ((pixel[i].data >> j) & 0x01)
		   {
			 *pBuff = NEOPIXEL_ONE;
		   }
		   else
		   {
			 *pBuff = NEOPIXEL_ZERO;
		   }
		   pBuff++;
	   }
	  }
	  dmaBuffer[DMA_BUFF_SIZE - 1] = 0; // last element must be 0!
	  //decide which channel to use depends which leds should work
	  if(NUM_PIXELS == 8){
		  HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_3, dmaBuffer, DMA_BUFF_SIZE);
	  }
	  if(NUM_PIXELS == 4){
		  HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_2, dmaBuffer, DMA_BUFF_SIZE);
	  }
}
void resetLEDs(void){
	NUM_PIXELS = 4;
	DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
	if(partnerId == NEIGHBOUR_ID_WEST || toId == NEIGHBOUR_ID_WEST){
		leds[0].color.g = 0;
		leds[0].color.r = 0;
		leds[0].color.b = 0;
	}
	if(partnerId == NEIGHBOUR_ID_SOUTH || toId == NEIGHBOUR_ID_SOUTH){
		leds[1].color.g = 0;
		leds[1].color.r = 0;
		leds[1].color.b = 0;
	}
	if(partnerId == NEIGHBOUR_ID_NORTH || toId == NEIGHBOUR_ID_NORTH){
		leds[2].color.g = 0;
		leds[2].color.r = 0;
		leds[2].color.b = 0;
	}
	if(partnerId == NEIGHBOUR_ID_EAST || toId == NEIGHBOUR_ID_EAST){
		leds[3].color.g = 0;
		leds[3].color.r = 0;
		leds[3].color.b = 0;
	}
	writeLEDs(leds);
}
void animateSentStorage(void){
	uint8_t i = 0;
	NUM_PIXELS = 8;
	DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
	// find index of Lager, were packageId was stored
	for(i = 0; i < LAGER_SIZE; i++){
		if(storage[i] == packageId){
			break;
		}
	}
	//STORAGE LEDS
	// turn off corresponding LED
	pixels[i+1].color.g = 0;
	pixels[i+1].color.r = 0;
	pixels[i+1].color.b = 0;
	writeLEDs(pixels);


			pixels[7].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
			pixels[7].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
			pixels[7].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
			writeLEDs(pixels);
			HAL_Delay(200);
			pixels[7].color.g = 0;
			pixels[7].color.r = 0;
			pixels[7].color.b = 0;
			writeLEDs(pixels);
			HAL_Delay(200);
	// display current Lager
	for(i = 0; i < LAGER_SIZE; i++){
		pixels[i+1].color.g = (uint8_t)LEDColors[tempStorage[i]][1]*0.1;
		pixels[i+1].color.r = (uint8_t)LEDColors[tempStorage[i]][0]*0.1;
		pixels[i+1].color.b = (uint8_t)LEDColors[tempStorage[i]][2]*0.1;

	}
	writeLEDs(pixels);
}
void animateReceiveStorage(void){
	NUM_PIXELS = 8;
	DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
			pixels[0].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
			pixels[0].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
			pixels[0].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
			writeLEDs(pixels);
			HAL_Delay(200);
			pixels[0].color.g = 0;
			pixels[0].color.r = 0;
			pixels[0].color.b = 0;
			writeLEDs(pixels);
			HAL_Delay(200);
	// display current Lager
	for(int i = 0; i < LAGER_SIZE; i++){
		pixels[i+1].color.g = (uint8_t)LEDColors[tempStorage[i]][1]*0.1;
		pixels[i+1].color.r = (uint8_t)LEDColors[tempStorage[i]][0]*0.1;
		pixels[i+1].color.b = (uint8_t)LEDColors[tempStorage[i]][2]*0.1;

	}
	writeLEDs(pixels);
}
void animateCreate(void){
	uint8_t i = 0;
	NUM_PIXELS = 8;
	DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
	// find index of tempLager, were packageId is stored
	for(i = 0; i < LAGER_SIZE; i++){
		if(tempStorage[i] == packageId){
			break;
		}
	}

			pixels[i+1].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
			pixels[i+1].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
			pixels[i+1].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
			writeLEDs(pixels);
			HAL_Delay(200);
			pixels[i+1].color.g = 0;
			pixels[i+1].color.r = 0;
			pixels[i+1].color.b = 0;
			writeLEDs(pixels);
			HAL_Delay(200);

	// display current Lager
	for(i = 0; i < LAGER_SIZE; i++){
		pixels[i+1].color.g = (uint8_t)LEDColors[tempStorage[i]][1]*0.1;
		pixels[i+1].color.r = (uint8_t)LEDColors[tempStorage[i]][0]*0.1;
		pixels[i+1].color.b = (uint8_t)LEDColors[tempStorage[i]][2]*0.1;

	}
	writeLEDs(pixels);
}
void animateDeliver(void){
	uint8_t i = 0;
	NUM_PIXELS = 8;
	DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
	// find index of Lager, were packageId was stored
	for(i = 0; i < LAGER_SIZE; i++){
		if(storage[i] == packageId){
			break;
		}
	}

			pixels[i+1].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
			pixels[i+1].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
			pixels[i+1].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
			writeLEDs(pixels);
			//HAL_Delay(10); SorageIntegrity Test hängt sich auf mit der Animation
			pixels[i+1].color.g = 0;
			pixels[i+1].color.r = 0;
			pixels[i+1].color.b = 0;
			writeLEDs(pixels);
			//HAL_Delay(10);

	// display current Lager
	for(int i = 0; i < LAGER_SIZE; i++){
		pixels[i+1].color.g = (uint8_t)LEDColors[tempStorage[i]][1]*0.1;
		pixels[i+1].color.r = (uint8_t)LEDColors[tempStorage[i]][0]*0.1;
		pixels[i+1].color.b = (uint8_t)LEDColors[tempStorage[i]][2]*0.1;

	}
	writeLEDs(pixels);
}
//HIER DIE LED ENDE
//HIER AUFZUG BEGINN
void animateSentElevator(void){
	//Hier Aufzug fahren lassen
	if(partnerId == otherElevatorAddress || toId == otherElevatorAddress || partnerId == myAddress || toId == myAddress){ //hier Id des Nachbarn mit dem anderen Aufzug
		direction = up;
		Aufzugfahr();
	}

	//äußere LEDs steuern
	NUM_PIXELS = 4;
	DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
	if(partnerId == NEIGHBOUR_ID_NORTH || toId == NEIGHBOUR_ID_NORTH){
		leds[0].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[0].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[0].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	if(partnerId == NEIGHBOUR_ID_EAST || toId == NEIGHBOUR_ID_EAST){
		leds[1].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[1].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[1].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	if(partnerId == NEIGHBOUR_ID_SOUTH || toId == NEIGHBOUR_ID_SOUTH){
		leds[2].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[2].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[2].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	if(partnerId == NEIGHBOUR_ID_WEST || toId == NEIGHBOUR_ID_WEST){
		leds[3].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[3].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[3].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	writeLEDs(leds);
}
void animateReceiveElevator(void){
	//Hier Aufzug fahren lassen
	if(partnerId == otherElevatorAddress || toId == otherElevatorAddress || partnerId == myAddress || toId == myAddress){ //hier Id des Nachbarn mit dem anderen Aufzug
		direction = up;
		Aufzugfahr();
	}

	//hier selbstgeklebte LEDs
	NUM_PIXELS = 4;
	DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
	if(partnerId == NEIGHBOUR_ID_NORTH || toId == NEIGHBOUR_ID_NORTH){
		leds[0].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[0].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[0].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	if(partnerId == NEIGHBOUR_ID_EAST || toId == NEIGHBOUR_ID_EAST){
		leds[1].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[1].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[1].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	if(partnerId == NEIGHBOUR_ID_SOUTH || toId == NEIGHBOUR_ID_SOUTH){
		leds[2].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[2].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[2].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	if(partnerId == NEIGHBOUR_ID_WEST || toId == NEIGHBOUR_ID_WEST){
		leds[3].color.g = (uint8_t)LEDColors[packageId][1]*0.1;
		leds[3].color.r = (uint8_t)LEDColors[packageId][0]*0.1;
		leds[3].color.b = (uint8_t)LEDColors[packageId][2]*0.1;
	}
	writeLEDs(leds);
}

void resetAufzug(void){
	if(positionReached){
		direction = down;
		Aufzugfahr();
	}
}
//HIER AUFZUG ENDE

void deployPackage(void) {
		// copy storage to tempStorage
		for(int i = 0; i < STORAGE_SIZE; i++){
			tempStorage[i] = storage[i];
		}
		// put package in first free spot (0) in tempStorage
		for(int i = 0; i < STORAGE_SIZE; i++){
			if(tempStorage[i] == 0){
				tempStorage[i] = packageId;
				break;
			}
		}
		finishedStore = true;
}

void handleSend(void){
	int i;
	// copy Lager to tempLager
	for(i = 0; i < STORAGE_SIZE; i++){
		tempStorage[i] = storage[i];
	}


	// delete package out of tempLager
	for(i = 0; i < STORAGE_SIZE; i++){
		if(tempStorage[i] == packageId){
			tempStorage[i] = 0;
			break;
		}
	}




	finishedSend = TRUE;
}

void updateStorage(void) {
		// copy tempStorage to storage
		for(int i = 0; i < STORAGE_SIZE; i++){
			storage[i] = tempStorage[i];
		}
}

void sendPulse(void) {
	uint8_t partnerNumber = 0;
		for(int i = 0; i < NUM_NEIGHBOURS; i++){
			if(neighbourIDs[i] == partnerId){
				partnerNumber = i;
				break;
			}
		}
		// toggle corresponding pin for 1ms; keine Unterscheidung für Ports A und B
		HAL_GPIO_WritePin (GPIOA, neighbourSendPins[partnerNumber], GPIO_PIN_SET);
		HAL_GPIO_WritePin (GPIOB, neighbourSendPins[partnerNumber], GPIO_PIN_SET);
		HAL_Delay(10);
		HAL_GPIO_WritePin (GPIOA, neighbourSendPins[partnerNumber], GPIO_PIN_RESET);
		HAL_GPIO_WritePin (GPIOB, neighbourSendPins[partnerNumber], GPIO_PIN_RESET);
		HAL_Delay(10);
}

void checkFailure(void){
	BOOL lagerFull = TRUE;
	BOOL packetInLager = FALSE;
	BOOL packetNoExist = TRUE;
	BOOL unknownNeighbour = TRUE;
	BOOL unknownPacket = FALSE;

	int i;

	// check if neighbour is known
	if(partnerId == 0){
		unknownNeighbour = FALSE; // neighbourId is valid
	}
	for(i = 0; i < NUM_NEIGHBOURS; i++){
		if(partnerId == neighbourIDs[i]){
			unknownNeighbour = FALSE; // neighbourId is valid
			break;
		}
	}

	if(ApNr == 42){ // create or await is set
		// check if Lager is already completely filled
		for(i = 0; i < STORAGE_SIZE; i++){
			if(storage[i] == 0){
				lagerFull = FALSE; // Lager is not completely filled
				break;
			}
		}

		packetNoExist = FALSE;
		// check if packageId is already stored in Lager
		for(i = 0; i < STORAGE_SIZE; i++){
			if(storage[i] == packageId){
				packetInLager = TRUE; // packageId already exists in Lager
				break;
			}
		}
	}
///hallo
	if(ApNr == 43){ // deliver or passOn is set
			lagerFull = FALSE;
			packetInLager = FALSE;
			packetNoExist = FALSE;
	}

	if(ApNr == 44){ // deliver or passOn is set
		lagerFull = FALSE;

		// Check if packageId exists in Lager
		for(i = 0; i < STORAGE_SIZE; i++){
			if(storage[i] == packageId){
				packetNoExist = FALSE; // packageId does exist in Lager
			}
		}
	}

	// check if package has a valid number
	if((packageId < 0) || (packageId > 16)){
		unknownPacket = TRUE;
	}

	// set errorId according to failure
	if(packetInLager){
		failure = TRUE;
		errorId = 1;
	}
	else if(lagerFull){
		failure = TRUE;
		errorId = 2;
	}
	else if(packetNoExist){
		failure = TRUE;
		errorId = 3;
	}
	else if(unknownNeighbour){
		failure = TRUE;
		errorId = 4;
	}
	else if(unknownPacket){
		failure = TRUE;
		errorId = 5;
	}
}

uint8_t rxBuffer[L1_PDU_size];
uint8_t L1_PDU[L1_PDU_size] = {0};
uint32_t cnt = 0;
uint32_t lastDebounceTime = 0;
bool uartDataSendable = true;

void std(void){
	switch(zustand){
	case Z_idle:
		if (receivedSDU){
			aktion = A_checkFailure;
			zustand = Z_processing;
			receivedSDU = FALSE;
		}
		else if(poll || (!receivedSDU)){
			aktion = A_idle;
			poll = FALSE;
		}
		break;

	case Z_processing:
		if (create && (!failure)){								//Erzeugen
			aktion = A_create;
			zustand = Z_awaiting;
			poll = FALSE;
		}
		else if (await && !passOn && (!failure)){				//Erwarten
			aktion = A_elevate;
			zustand = Z_elevate;
		}
		else if (deliver && (!failure)){						//Abliefern
			aktion = A_deliver;
			zustand = Z_deliver;
		}
		else if (passOn && !await && (!failure)){				//Versenden bzw. Forwarding nach Empfang
			aktion = A_passOn;
			zustand = Z_passOn;
		}
		else if ((await && passOn) && (!failure)){				//Forwarding vor Empfang
			aktion = A_elevate;
			zustand = Z_elevate;
		}
		else if (failure){
			aktion = A_failure;
			zustand = Z_failure;
			poll = FALSE;
		}
		break;


	case Z_failure:
		if(poll){
			aktion = A_setup;
			zustand = Z_idle;
			poll = FALSE;
		}
		break;

	case Z_deliver:					//abliefern an Master
		if (poll && finishedSend){
			aktion = A_updateLager;
			zustand = Z_sent;
			poll = FALSE;
		}
		break;

	case Z_passOn:					//ausliefern an Nachbarn
		if (poll && finishedSend && positionReached){
			aktion = A_pulse;
			zustand = Z_sent;
			poll = FALSE;
		}
		break;


	case Z_sent:					//Senden abgeschlossen mit Ankunft des , also alles zurücksetzen
		if (poll){
			aktion = A_setup;
			zustand = Z_idle;
			poll = FALSE;
		}
		break;

	case Z_awaiting:
		if(GPIO_neighbour_in && (await && !passOn)){ 	//Erwarten
			aktion = A_storeAwait;
			zustand = Z_received;
			poll = FALSE;
		}
		else if (poll && finishedStore && create){		//Erzeugen
			aktion = A_storeCreate;
			zustand = Z_received;
			poll = FALSE;
		}
		else if ((await && passOn) && GPIO_neighbour_in){ //weiterleiten
			aktion = A_forwardAwait;
			zustand = Z_received;
			poll = FALSE;
		}
		break;

	case Z_received:
		if(poll && (await && passOn)){			//Forwarding
			aktion = A_forwardAgain;
			zustand = Z_processing;
			poll = FALSE;
			}

		else if (poll && (!await || !passOn)){ 	//Erwarten und Erzeugen
			aktion = A_setup;
			zustand = Z_idle;
			poll = FALSE;
		}
		else if(!poll){
			aktion = A_idle;
		}
		break;

	case Z_elevate:
		if(fromId == otherElevatorAddress || partnerId == otherElevatorAddress){
			if(positionReached == true){
				aktion = A_await;
				zustand = Z_awaiting;
				positionReached = false;
				poll = FALSE;
			}
		}else{
			aktion = A_await;
			zustand = Z_awaiting;
			poll = FALSE;
		}
		break;
	}
}

void pat(void){
	switch(aktion){
	case A_setup:
		resetAufzug();
		resetData();
		stateProcessing();
		resetLEDs();
		break;
	case A_deliver:
		stateProcessing();
		handleSend();
		break;
	case A_passOn:
		stateProcessing();
		handleSend();
		animateSentElevator();
		break;
	case A_failure:
		stateFailure();
		break;
	case A_pulse:
		updateStorage();
		stateSent();
		animateSentStorage();
		sendPulse();
		break;
	case A_updateLager:
		animateDeliver();
		updateStorage();
		//stateSent();
		stateProcessing();
		break;
	case A_forwardAgain:
		resetAufzug();
		forwardReset();
		stateProcessing();
		break;
	case A_forwardAwait:
		stateReceived();
		resetPulse();
		break;
	case A_await:
		break;
	case A_create:
		stateAwait();
		deployPackage();
		break;
	case A_handleStore:
		deployPackage();
		break;
	case A_storeAwait:
		deployPackage();
		updateStorage();
		stateReceived();
		resetPulse();
		animateReceiveStorage();
		break;
	case A_storeCreate:
		animateCreate();
		updateStorage();
		stateReceived();
		break;
	case A_checkFailure:
		checkFailure();
		stateProcessing();
		break;
	case A_elevate:
		animateReceiveElevator();
		break;
	case A_idle:
		break;
	}
	aktion = A_idle;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart1, rxBuffer, sizeof(rxBuffer));
	// Zustandsuebergangsdiagramm und Aufzüge reset
  	direction = up;
  	Aufzugfahr();
  	HAL_Delay(500);
  	direction = down;
  	Aufzugfahr();
  	direction = up;

  	//alles in den Startzustand bringen
  	zustand = Z_idle;
    aktion = A_idle;
    pat();
    //LEDs Zurücksetzen
    for(int i = 0; i < NUM_PIXELS; i++){
    		pixels[i].color.g = 0;
    		pixels[i].color.r = 0;
    		pixels[i].color.b = 0;
    }
    NUM_PIXELS = 8;
    DMA_BUFF_SIZE = (NUM_PIXELS * 24) + 1;
    writeLEDs(pixels);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  	  std();
	  	  pat();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|to_S_SV4_Pin|to_O_SV3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, to_N_SV8_Pin|to_W_SV5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, up_Pin|down_Pin|speed_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : button_down_Pin */
  GPIO_InitStruct.Pin = button_down_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(button_down_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin to_S_SV4_Pin to_O_SV3_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|to_S_SV4_Pin|to_O_SV3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : to_N_SV8_Pin to_W_SV5_Pin */
  GPIO_InitStruct.Pin = to_N_SV8_Pin|to_W_SV5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : up_Pin down_Pin speed_Pin */
  GPIO_InitStruct.Pin = up_Pin|down_Pin|speed_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : from_N_LS1_Pin from_O_LS2_Pin from_S_LS3_Pin */
  GPIO_InitStruct.Pin = from_N_LS1_Pin|from_O_LS2_Pin|from_S_LS3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : from_W_LS4_Pin */
  GPIO_InitStruct.Pin = from_W_LS4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(from_W_LS4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : button_up_Pin */
  GPIO_InitStruct.Pin = button_up_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(button_up_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void Aufzugfahr(){
	if(direction == up){// Fahrtrichtung aufwärts
		HAL_GPIO_WritePin(GPIOC, down_Pin, 0);
	    HAL_GPIO_WritePin(GPIOC, up_Pin, 1);
		// Set the speed high
		HAL_GPIO_WritePin(GPIOC, speed_Pin, 1);
	}
	if(direction == down){// Fahrtrichtung abwärts
		HAL_GPIO_WritePin(GPIOC, up_Pin, 0);
		HAL_GPIO_WritePin(GPIOC, down_Pin, 1);
		// Set the speed high
		HAL_GPIO_WritePin(GPIOC, speed_Pin, 1);
	}
}

void resetData(){
	packageId = 0;
	partnerId = 0;
	errorId = 0;
	receive = false;
	passOn = false;
	create = false;
	deliver = false;
	poll = false;
	await = false;
	ApNr = 0;
	failure = false;
	finishedStore = false;
	finishedSend = false;
	receivedSDU = false;
	GPIO_neighbour_in = false;
	pushedButton = false;
	fromId = -1; ///hallo
	toId = -1; ///hallo
	direction = down;	//damit aufzug im nächsten Schritt wieder runter in default fährt
	positionReached = false; //
}

void forwardReset(void) {
	await = FALSE;
	pushedButton = FALSE;
	direction = down;
	positionReached = false;
}

void L1_receive(uint8_t L1_PDU[L1_PDU_size]) {
	uint8_t L1_SDU[L1_SDU_size];
    memcpy(L1_SDU, &L1_PDU[1], L1_SDU_size);
    HAL_UART_Receive_IT(&huart1, rxBuffer, sizeof(rxBuffer));
    L2_receive(L1_SDU);
}

void L2_receive(uint8_t L2_PDU[L2_PDU_size]){
	 uint8_t receivedChecksum = L2_PDU[L2_SDU_size];
	    uint8_t calculatedChecksum = 0;
	    for (uint8_t i = 0; i < L2_SDU_size; i++) {
	        calculatedChecksum += L2_PDU[i];
	    }
	    calculatedChecksum = ~calculatedChecksum;
	    if (calculatedChecksum == receivedChecksum) {
	        uint8_t L2_SDU[L2_SDU_size];
	        for (uint8_t i = 0; i < L2_SDU_size; i++) {
	            L2_SDU[i] = L2_PDU[i];
	        }
	        L3_receive(L2_SDU);
	    } else {
	    	uartDataSendable = 1;
	    }


}

void L3_receive(uint8_t L3_PDU[L3_PDU_size]){
	uint8_t to = L3_PDU[0];
	uint8_t from = L3_PDU[1];
	uint8_t vers = L3_PDU[2];
	uint8_t hops = L3_PDU[3];
	if (vers != 6) {
	    return;
	}
    if (to == 0 && from == 0) {
        return;
    }
    if (to == myAddress && from == 0) {
        uint8_t L3_SDU[L3_SDU_size];
        memcpy(L3_SDU, &L3_PDU[4], L3_SDU_size);
        L7_receive(L3_SDU);
    }else{// if (to != myAddress && to != 0 && from == 0) { //Mit if-Bed. funktioniert Master nicht
	     hops++;
	     L3_PDU[3] = hops;
	     L2_send(L3_PDU);
	}
}

void L7_receive(uint8_t L7_PDU[L7_PDU_size]){
 	uint8_t L7_SDU[L7_SDU_size] = {0};
 	uint8_t L7_SDU_send[L7_SDU_size] = {0}; // information to send back

 	for(int i = 0; i < L7_SDU_size; i++){ // remove first byte (ApNr) to get L7_SDU
 		L7_SDU[i] = L7_PDU[i+1];
 	}

 	// ApNr 42
 	// await / create package
 	// send back received L7_SDU
 	if(L7_PDU[0] == 42){
 		ApNr_42(L7_SDU, L7_SDU_send);
 		L7_send(42, L7_SDU_send);
 	}

 	// ApNr 43
 	// forward package
 	// send back received L7_SDU ///hallo
 	if(L7_PDU[0] == 43){
 		ApNr_43(L7_SDU, L7_SDU_send);
 		L7_send(43, L7_SDU_send);
 	}

 	// ApNr 44
 	// pass on / deliver package
 	// send back received L7_SDU
 	if(L7_PDU[0] == 44){
 	  	ApNr_44(L7_SDU, L7_SDU_send);
 	  	L7_send(44, L7_SDU_send);
 	}

 	// ApNr 50
 	// poll status
 	if(L7_PDU[0] == 50){
 		ApNr_50(L7_SDU, L7_SDU_send);
 		L7_send(50, L7_SDU_send);
 	}

 	uartDataSendable = true;  // ApNr invalid (unknown) -> discard packet
 	HAL_UART_Receive_IT(&huart1, rxBuffer, sizeof(rxBuffer)); // Attach interrupt to receive L1_PDU from USART
}

void L7_send(uint8_t Id, uint8_t L7_SDU[L7_SDU_size]){
	uint8_t L7_PDU[L7_PDU_size];
	L7_PDU[0] = Id;
	memcpy(&L7_PDU[1], L7_SDU, L7_SDU_size);
	L3_send(L7_PDU);
}

void L3_send(uint8_t L3_SDU[L3_SDU_size]){
	uint8_t L3_PDU[L3_PDU_size];
	// Setzen der PCI
	L3_PDU[0] = 0;           // To-Feld: Adresse 0(Master)
	L3_PDU[1] = myAddress;   // From-Feld: Eigene Adresse
	L3_PDU[2] = 6;           // Version: 6
	L3_PDU[3] = 0;           // Hop Counter: 0
	// SDU->PDU
	memcpy(&L3_PDU[4], L3_SDU, L3_SDU_size);
	L2_send(L3_PDU);
}

void L2_send(uint8_t L2_SDU[L2_SDU_size]){
	uint8_t L2_PDU[L2_PDU_size];
		memcpy(L2_PDU, L2_SDU, L2_SDU_size);
		uint8_t checksum = 0;
		for (uint8_t i = 0; i < L2_SDU_size; i++) {
		    checksum += L2_SDU[i];
		}
		checksum = ~checksum;
		L2_PDU[L2_SDU_size] = checksum;
		L1_send(L2_PDU);
}

void L1_send(uint8_t L1_SDU[L1_SDU_size]){
	 uint8_t L1_PDU[L1_PDU_size];
	 L1_PDU[0] = 0;
	 // SDU->PDU
	 memcpy(&L1_PDU[1], L1_SDU, L1_SDU_size);
	 L1_PDU[15] = 0;
	 HAL_UART_Transmit_IT(&huart1, L1_PDU, L1_PDU_size);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	 if (huart->Instance == USART1) {
		 for(int i = 0; i < L1_PDU_size; i++){ // copy received packet from buffer
		 			L1_PDU[i] = rxBuffer[i];
		 		}
	    }
	 HAL_UART_Receive_IT(&huart1, rxBuffer, L1_PDU_size); // Attach interrupt to receive L1_PDU from USART
	 L1_receive(L1_PDU);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
    	uartDataSendable = true;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == B1_Pin) {
	        uint32_t currentTime = HAL_GetTick();

	       if ((currentTime - lastDebounceTime) > debounceDelay) {
	            cnt++;
	        	pushedButton = true;
	            lastDebounceTime = currentTime;
	        }
	}

	//Communication with Neighbours
	if((GPIO_Pin == from_N_LS1_Pin) && (neighbourIDs[1] == partnerId)){
	        GPIO_neighbour_in = TRUE;
	}

	if((GPIO_Pin == from_O_LS2_Pin) && (neighbourIDs[2] == partnerId)){
	        GPIO_neighbour_in = TRUE;
	}

	if((GPIO_Pin == from_S_LS3_Pin) && (neighbourIDs[0] == partnerId)){
			GPIO_neighbour_in = TRUE;
	}

	if((GPIO_Pin == from_W_LS4_Pin) && (neighbourIDs[3] == partnerId)){
	        GPIO_neighbour_in = TRUE;
	}

	//Elevator
	if(GPIO_Pin == button_down_Pin){
		uint32_t currentTime = HAL_GetTick();
			if((currentTime - lastDebounceTime) > debounceDelay){

				HAL_GPIO_TogglePin (GPIOA, GPIO_PIN_5); //toggle LED for control
			    HAL_GPIO_WritePin(GPIOC, up_Pin, 0); //stop engine
			    HAL_GPIO_WritePin(GPIOC, down_Pin, 0);

			    direction = up; //change direction
			    lastDebounceTime = currentTime;
			}
	}

	if(GPIO_Pin == button_up_Pin){
		uint32_t currentTime = HAL_GetTick();
			if((currentTime - lastDebounceTime) > debounceDelay){

				HAL_GPIO_TogglePin (GPIOA, GPIO_PIN_5); //toggle LED for control
				HAL_GPIO_WritePin(GPIOC, up_Pin, 0); //stop motor
				HAL_GPIO_WritePin(GPIOC, down_Pin, 0);

				direction = down; //change direction
				lastDebounceTime = currentTime;
				positionReached = true;
			}
	}

}

// ApNr 42
// await / create package
// send back received L7_SDU
void ApNr_42(uint8_t L7_SDU[], uint8_t L7_SDU_send[]){
	packageId = L7_SDU[0];
	partnerId = L7_SDU[1];

	receivedSDU = TRUE;
	ApNr = 42;

	if(partnerId != 0){ // partnerId is not 0 -> await new package
		await = TRUE;
	} else { // partnerId is 0 -> create new package
		create = TRUE;
	}

	for(int i = 0; i < L7_SDU_size; i++){ // copy L7_SDU to L7_SDU_send
		L7_SDU_send[i] = L7_SDU[i];
	}
}
// ApNr 43
// forward package
// send back received L7_SDU
///hallo
void ApNr_43(uint8_t L7_SDU[], uint8_t L7_SDU_send[]){
	packageId = L7_SDU[0];
	fromId = L7_SDU[1];
	toId = L7_SDU[2];

	receivedSDU = TRUE;
	ApNr = 43;

	if(fromId != 0 && toId != 0){
		await = TRUE;
		passOn = TRUE;
	}

	for(int i = 0; i < L7_SDU_size; i++){ // copy L7_SDU to L7_SDU_send
		L7_SDU_send[i] = L7_SDU[i];
	}
}
// ApNr 44
// pass on / deliver package
// send back received L7_SDU
void ApNr_44(uint8_t L7_SDU[], uint8_t L7_SDU_send[]){
	packageId = L7_SDU[0];
	partnerId = L7_SDU[1];

	receivedSDU = TRUE;
	ApNr = 44;

	if(partnerId != 0){ // partnerId is not 0 -> passOn package
		passOn = TRUE;
	} else { // partnerId is 0 -> deliver package
		deliver = TRUE;
	}

	for(int i = 0; i < L7_SDU_size; i++){ // copy L7_SDU to L7_SDU_send
		L7_SDU_send[i] = L7_SDU[i];
	}
}

// ApNr 50
// poll status
void ApNr_50(uint8_t L7_SDU[], uint8_t L7_SDU_send[]){
	poll = TRUE;

	L7_SDU_send[0] = state;
	if(state == 4){ // state is failure, send errorId instead of packageId
		L7_SDU_send[1] = errorId;
	} else {
		L7_SDU_send[1] = packageId;
	}
	for(int i = 2; i < L7_SDU_size; i++){ // copy Lager to L7_SDU_send, index 2 to 7
		L7_SDU_send[i] = storage[i-2];
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
