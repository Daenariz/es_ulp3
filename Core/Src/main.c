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
//#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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

#define NUM_NEIGHBOURS 2
#define STORAGE_SIZE 6
#define RGB_DATA_LEN (LED_COUNT * 24)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim2_ch3_up;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t L7_SDU[L7_SDU_size];
// Speicher
uint8_t state;
uint8_t errorId;
uint8_t partnerId;
uint8_t packageId;
uint8_t storage[6] = {0};
uint8_t tempStorage[6] = {0};
uint8_t ApNr;
uint8_t neighbourIDs[] = { 38 };

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
// Speicher
//uint32_t L7_SDU
/*uint8_t state;
uint8_t storage;
uint32_t pkgID;
uint32_t partnerID;
*/
// Kontrollfluesse
bool spaceAvailable = true;
bool pkgAvailable = false;
bool partnerIDValid = false;

// Aktionentyp + Aktionsvariable
typedef enum { Z_processing, Z_awaiting, Z_received, Z_sent, Z_failure, Z_idle, } zustand_t;
zustand_t zustand;

// Aktionentyp + Aktionsvariable
typedef enum { A_send, A_status, A_store, A_message, A_updateStorage} aktion_t;
aktion_t aktion;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void L1_receive(uint8_t L1_PDU[L1_PDU_size]);
void L2_receive(uint8_t L2_PDU[L2_PDU_size]);
void L3_receive(uint8_t L3_PDU[L3_PDU_size]);
void L7_receive(uint8_t L7_PDU[L7_PDU_size]);
void L7_send(uint8_t Id, uint8_t L7_SDU[L7_SDU_size]);
void L3_send(uint8_t L3_SDU[L3_SDU_size]);
void L2_send(uint8_t L2_SDU[L2_SDU_size]);
void L1_send(uint8_t L1_SDU[L1_SDU_size]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t rxBuffer[L1_PDU_size];
uint32_t myAddress = 37;
uint32_t cnt = 0;
uint32_t lastTime = 0;
const uint32_t debounceDelay = 150;
bool uartDataReceived = false;
bool uartDataSendable = true;
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
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, rxBuffer, sizeof(rxBuffer));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  uint8_t L1_PDU[L1_PDU_size];
	  if (uartDataReceived) {
		for(int i = 0; i < L1_PDU_size;i++ ){
			L1_PDU[i] = rxBuffer[i];
		}
		uartDataReceived = false;
		L1_receive(L1_PDU);
	  }
	  std();
	  //pat();
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
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
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
}
// STD
void std(void){
	switch(zustand){
	case Z_processing:
		if (receivedSDU){
			checkFailure();
			//updateStorage();   /// ?
			receivedSDU = false;
		}
		else if(poll || (!receivedSDU)){
			poll = false;
		}
		if (create && (!failure)){ // no poll (transient state), because processing + packageId should only be reported in modes passOn and deliver
			deployPackage();
			zustand = Z_awaiting;
			poll = false;
		}
		else if (await && (!failure)){ // no poll (transient state), because processing + packageId should only be reported in modes passOn and deliver
			deployPackage();
			zustand = Z_awaiting;
			poll = false;
		}
		else if (deliver && (!failure)){ /// ?
			handleSend();
			updateStorage();
			zustand = Z_sent;
			poll = false;
		}
		else if (passOn && pushedButton && (!failure)){
			handleSend();
			updateStorage();
			zustand = Z_sent;
			poll = false;
		}
		else if (failure){
			zustand = Z_failure;
			poll = false;
		}
		break;

	case Z_awaiting:
//		if (poll && await && pushedButton){
//			//aktion = A_store;
//			zustand = Z_received;
//		}
		//else
			if (poll && finishedStore && pushedButton && await){
			zustand = Z_received;
			poll = false; //*
		}
		else if (poll && finishedStore && create){ //else if (poll && finishedStore && create && count	){
			//updateStorage();
			deployPackage();
			zustand = Z_received;
			poll = false; //*
		}
		break;

	case Z_received:
		if (poll){
			updateStorage();
			resetData();
			zustand = Z_processing;
			poll = false;
		}
		break;

	case Z_sent:
		if (poll){
			resetData();
			zustand = Z_processing;
			poll = false;
		}
		break;

	case Z_failure:
		if(poll){
			resetData();
			zustand = Z_processing;
			poll = false;
		}
		break;

	}
}

//* Kontrollflüsse werden im Interrupt gesetzt und müssen deshalb hier zurückgesetzt werden

// PAT
//void pat(void){
//	switch(aktion){
//	case A_send:
//		resetData();
//		stateProcessing();
//		break;
//	case A_status:
//		stateProcessing();
//		//handleSend();
//		break;
//	case A_store:
//		stateProcessing();
//		//handleSend();
//		break;
//	case A_message:
//		stateFailure();
//		break;
//	case A_updateStorage:
//		//updatestorage();
//		stateSent();
//		break;
//	}
//}

// DFD processes //

void sendPulse(void) {
	uint8_t partnerNumber = 0;
//		// find out number of partner
//		for(int i = 0; i < NUM_NEIGHBOURS; i++){
//			if(neighbourIDs[i] == partnerId){
//				partnerNumber = i;
//				break;
//			}
//		}
		// toggle corresponding pin for 1ms
		HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		HAL_Delay(1000);
		HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		HAL_Delay(1000);
}

void parseRxBuff(void) {

}

void parseSDU(void) {

}

void status(void) {

}

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

void handleSend(void) {
		// copy storage to tempLager
		for(int i = 0; i < STORAGE_SIZE; i++){
			tempStorage[i] = storage[i];
		}
		// delete package out of tempLager
		for(int i = 0; i < STORAGE_SIZE; i++){
			if(tempStorage[i] == packageId){
				tempStorage[i] = 0;
				break;
			}
		}
		finishedSend = true;
}

void checkFailure(void){
	bool storageFull = false;
	bool PackageInStorage = false;
	bool PackageNoExist = false;
	bool unknownNeighbour = true;
	bool unknownPackage = false;

	int i;

	// check if neighbour is known
	if(partnerId == 0){ //TODO: fixed with idle in processing, but that should not be there
		unknownNeighbour = false; // neighbourId is valid
	}
	for(i = 0; i < NUM_NEIGHBOURS; i++){
		if(partnerId == neighbourIDs[i]){
			unknownNeighbour = false; // neighbourId is valid
			break;
		}
	}
	if((create || await) == true){ // create or await is set
		// check if storage is already completely filled
		for(i = 0; i < STORAGE_SIZE; i++){
			if(storage[i] == 0){
				storageFull = false; // storage is not completely filled
			}
			else {
				storageFull = true;
			}
		}
		PackageNoExist = false;
		// check if packageId is already stored in storage
		for(i = 0; i < STORAGE_SIZE; i++){
			if(storage[i] == packageId){
				PackageInStorage = true; // packageId already exists in storage
				break;
			}
		}
	}

	if((passOn || deliver) == true){ // deliver or passOn is set
		storageFull = false;

		// Check if packageId exists in storage
		for(i = 0; i < STORAGE_SIZE; i++){
			if(storage[i] == packageId){
				PackageNoExist = false; // packageId does exist in storage
			}
		}
	}

	// check if package has a valid number
	if((packageId < 0) || (packageId > 16)){
		unknownPackage = true;
	}

	// set errorId according to failure
	if(PackageInStorage){
		failure = true;
		errorId = 1;
	}
	else if(storageFull){
		failure = true;
		errorId = 2;
	}
	else if(PackageNoExist){
		failure = true;
		errorId = 3;
	}
	else if(unknownNeighbour){
		failure = true;
		errorId = 4;
	}
	else if(unknownPackage){
		failure = true;
		errorId = 5;
	}
}


void msg(void) {

}

void validatePartner() {

}

void updateStorage(void) {
		// copy tempStorage to storage
		for(int i = 0; i < STORAGE_SIZE; i++){
			storage[i] = tempStorage[i];
		}
}

void L1_receive(uint8_t L1_PDU[L1_PDU_size]) {
	uint8_t L1_SDU[L1_SDU_size];
    memcpy(L1_SDU, &L1_PDU[1], L1_SDU_size);
    HAL_UART_Receive_IT(&huart2, rxBuffer, sizeof(rxBuffer));
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
    } else {}
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
    }else if (to != myAddress && to != 0 && from == 0) {
	     hops++;
	     L3_PDU[3] = hops;
	     L2_send(L3_PDU);
	}
}

void L7_receive(uint8_t L7_PDU[L7_PDU_size]){
	 uint8_t apNr = L7_PDU[0];

	 memcpy(L7_SDU, &L7_PDU[1], L7_SDU_size);
	 switch (apNr) {
	    case 42: // Await/ Create Package
		    packageId = L7_SDU[0];
		 	partnerId = L7_SDU[1];

		 	receivedSDU = true; // relevant?
		 	//ApNr = 42;

		 	if(partnerId != 0){ // partnerId is not 0 -> await new package
		 		await = true;
		 	} else { // partnerId is 0 -> create new package
		 		create = true;
		 	}
		 	L7_send(apNr, L7_SDU);
		 	//for(int i = 0; i < L7_SDU_size; i++){ // copy L7_SDU to L7_SDU_send
		 	//	L7_SDU_send[i] = L7_SDU[i];
		 	//}
	    case 43: // Forward Package
	    	packageId = L7_SDU[0];
	    	receivedSDU = true;
	    	uint8_t fromId = L7_SDU[1];
	    	uint8_t toId = L7_SDU[2];
//	    	            if (toId == 0 || fromId == 0) { // Invalid neighbor IDs
//	    	            	failure = true;
//	    	                //zustand = Z_failure;
//	    	                //errorId = 4; // Ungültige Nachbar-ID
//	    	                break;
//	    	            }

	    	            await = true;
	    	            passOn = true;// Warten auf Signal
	    	            // Weiterleiten des Pakets (ohne Speichern)
	    	            L7_send(apNr, L7_SDU);
	    	            break;

	     case 44: // Pass on/ Deliver Package
	    	 packageId = L7_SDU[0];
	    	 partnerId = L7_SDU[1];

	    	 receivedSDU = true;
	    	 //ApNr = 44;

	    	 if(partnerId != 0){ // partnerId is not 0 -> passOn package
	    	 	    passOn = true;
	    	 	} else { // partnerId is 0 -> deliver package
	    	 		deliver = true;
	    	 	}
	    	    L7_send(apNr, L7_SDU);

	    	 	//for(int i = 0; i < L7_SDU_size; i++){ // copy L7_SDU to L7_SDU_send
	    	 	//	L7_SDU_send[i] = L7_SDU[i];
	    	 	//}
	     case 50:
	    	 poll = true;

	    	 	L7_SDU[0] = zustand;
	    	 	if(zustand == Z_failure){ // state is failure, send errorId instead of packageId
	    	 		L7_SDU[1] = errorId;
	    	 	} else {
	    	 		L7_SDU[1] = packageId;
	    	 	}
	    	 	for(int i = 2; i < L7_SDU_size; i++){ // copy storage to L7_SDU_send, index 2 to 7 //TODO: adapt to storage size constant
	    	 		L7_SDU[i] = storage[i-2];
	    	 	}
	    	 	L7_send(apNr, L7_SDU);
	 	 case 100:
	 		 if(L7_SDU[7]!=0){
	 			 HAL_GPIO_WritePin ( GPIOA , GPIO_PIN_5 , GPIO_PIN_SET );
	 		 }else{
	 			HAL_GPIO_WritePin ( GPIOA , GPIO_PIN_5 , GPIO_PIN_RESET );
	 	 	 }
	         L7_send(apNr, L7_SDU);
	         break;
	     case 101:
	         L7_SDU[7] = cnt;
	         cnt = 0;
	         L7_send(apNr, L7_SDU);
	         break;
	     case 102:
	         uint32_t uidw0 = HAL_GetUIDw0();
	         uint32_t uidw1 = HAL_GetUIDw1();
	         memcpy(&L7_SDU[0], &uidw0, 4); // 0-31
	         memcpy(&L7_SDU[4], &uidw1, 4); // 32-63
	         L7_send(apNr, L7_SDU);
	         break;
	     case 103:
	         uint32_t uidw2 = HAL_GetUIDw2();
	         memcpy(&L7_SDU[0], &uidw2, 4); // 64-95
	         memset(&L7_SDU[4], 0, 4);
	         L7_send(apNr, L7_SDU);
	         break;
	     default:
	         memset(L7_SDU, 0, L7_SDU_size);
	         L7_send(apNr, L7_SDU);
	         break;
	 }
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
	 HAL_UART_Transmit_IT(&huart2, L1_PDU, L1_PDU_size);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	 if (huart->Instance == USART2) {
	        uartDataReceived = true;
	    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
    	uartDataSendable = true;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == B1_Pin) {
	        uint32_t currentTime = HAL_GetTick();
	        if ((currentTime - lastTime) > debounceDelay) {
	            cnt++;
	        	pushedButton = true;
	            lastTime = currentTime;
	        }
		// Reset der L7_SDU
		/*for (uint8_t i = 0; i < L7_SDU_size; i++) {
		                L7_SDU[i] = 0;
		            }*/
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
