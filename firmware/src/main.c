#include "main.h"

USBD_HandleTypeDef USBD_Device;
TIM_HandleTypeDef    TimHandle;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);

#ifdef USE_FREERTOS

osThreadId_t led_handle, synth_handle, midi_handle;
osSemaphoreId_t synth_sem;
osMutexId_t synth_mutex;

osMessageQueueId_t midi_queue;

uint8_t synthesized = 0;
#else
uint8_t synthesized = 1;
#endif

uint8_t global_buf[AUDIO_BUF_SIZE];

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
  if(synthesized) 
  {
    synth_update(global_buf, 0, AUDIO_BUF_SIZE / 2);
    audio_update(global_buf, 0, AUDIO_BUF_SIZE / 2);
  } else {
    // skip half/frame
    memset(&global_buf[0], 0, AUDIO_BUF_SIZE / 2);
  }
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  if(synthesized) {
    synth_update(global_buf, AUDIO_BUF_SIZE / 2, AUDIO_BUF_SIZE / 2);
    audio_update(global_buf, AUDIO_BUF_SIZE / 2, AUDIO_BUF_SIZE / 2);
  } else {
    // skip half/frame
    memset(&global_buf[0] + AUDIO_BUF_SIZE / 2, 0, AUDIO_BUF_SIZE / 2);
  }

#ifdef USE_FREERTOS
  osSemaphoreRelease(synth_sem);
#endif
}

#ifdef USE_FREERTOS
void vApplicationIdleHook( void )
{
  while (1)
  {
    __WFI();
  }
}

void led_task(void *argument)
{
  while (1) {
    if (synth_available()) {
      BSP_LED_Toggle(LED1);
      osDelay(1000);
    } else {
      if(QSPI_is_writing()) {
        BSP_LED_Toggle(LED1);
        osDelay(100);        
      }
    }
    osThreadYield();
  }
}

void midi_task(void *argument)
{
  while (1) {
    if (synth_available()) {
      struct midi_message midi_msg;

      osStatus_t status = osMessageQueueGet(midi_queue, &midi_msg, NULL, osWaitForever);
      if (status == osOK) {
#ifdef LED2_PIN
        BSP_LED_On(LED2);
#endif
        synth_midi_process(midi_msg.data, midi_msg.len);
#ifdef LED2_PIN
        BSP_LED_Off(LED2);
#endif
      }
    }
    osThreadYield();
  }
}

void synth_task(void *argument)
{
  while (1) {
    osSemaphoreAcquire(synth_sem, osWaitForever);
    synthesized = 0;
    synth_render(0, AUDIO_BUF_SIZE/2);
    synthesized = 1;
    synth_render(AUDIO_BUF_SIZE/2, AUDIO_BUF_SIZE/2);
    synthesized = 2;
    osThreadYield();
  }
}
#endif

void led_init() {
  /* Configure LED1 */
  BSP_LED_Init(LED1);
#ifdef LED2_PIN
  BSP_LED_Init(LED2);
#endif

  BSP_LED_Off(LED1);
#ifdef LED2_PIN
  BSP_LED_Off(LED2);
#endif
}

void usb_init() {
#ifdef MUCISBOARD_USB_COMPOSITE
  /* Make the connection and initialize to USB_OTG/usbdc_core */
  USBD_Init(&USBD_Device, &MUSICBOARD_Desc, 0);
  USBD_RegisterClass(&USBD_Device, &USBD_Composite_ClassDriver);
  USBD_Composite_RegisterInterface(&USBD_Device);
#else
#ifdef MUCISBOARD_USB_MIDI_ONLY
  USBD_Init(&USBD_Device, &MUSICBOARD_Desc, 0);
  USBD_RegisterClass(&USBD_Device, &USBD_Midi_ClassDriver);
  USBD_Composite_RegisterInterface(&USBD_Device);
#endif

#ifdef MUCISBOARD_USB_AUDIO_ONLY
  USBD_Init(&USBD_Device, &MUSICBOARD_Desc, 0);
  USBD_RegisterClass(&USBD_Device, &USBD_AUDIO);
  USBD_Composite_RegisterInterface(&USBD_Device);
#endif

#ifdef MUSICBOARD_USB_MSC_ONLY
  USBD_Init(&USBD_Device, &MUSICBOARD_Desc, 0);
  USBD_RegisterClass(&USBD_Device, &USBD_MSC);
  USBD_MSC_RegisterStorage(&USBD_Device, &USBD_DISK_fops);
#endif
#endif

  USBD_Start(&USBD_Device);
}

void app_main(void *argument) {

}

int main(void)
{
  /* Configure the MPU attributes as Write Through */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32F7xx HAL library initialization */
  HAL_Init();

  /* Configure the system clock to 200 MHz */
  SystemClock_Config();

  led_init();

  setbuf(stdout, NULL);
  HAL_Delay(100);

  QSPI_init();

  HAL_Delay(100);

  audio_init();
  audio_buffer_init();

  BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_AUTO, MASTER_VOLUME, SAMPLE_RATE);
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);  // PCM 2-channel

  memset(global_buf, 0, AUDIO_BUF_SIZE);
  BSP_AUDIO_OUT_Play((uint16_t *)&global_buf[0], AUDIO_BUF_SIZE);

  usb_init();

  synth_init();
  synth_buffer_init();

#ifdef USE_FREERTOS
  osKernelInitialize();

  synth_sem = osSemaphoreNew (1, 1, NULL);
  synth_mutex = osMutexNew (NULL);

#ifdef QUEUED_MIDI_MESSAGES
  midi_queue = osMessageQueueNew(512, sizeof(struct midi_message), NULL);
#endif
  osThreadAttr_t led_thr_attr = {
    .priority = osPriorityLow
  };
#ifdef QUEUED_MIDI_MESSAGES
  osThreadAttr_t midi_thr_attr = {
    .priority = osPriorityHigh,
    .stack_size = 4096
  };
#endif
  osThreadAttr_t synth_thr_attr = {
    .priority = osPriorityRealtime,
    .stack_size = 4096
  };

  led_handle = osThreadNew(led_task, NULL, &led_thr_attr);
#ifdef QUEUED_MIDI_MESSAGES
  midi_handle = osThreadNew(midi_task, NULL, &midi_thr_attr);
#endif
  synth_handle = osThreadNew(synth_task, NULL, &synth_thr_attr);

  osKernelStart();
#else
  timer_init();
#endif

  while (1)
  {
    __WFI();
  }
}


void timer_init() {
  /* Prescaler declaration */
  uint32_t uwPrescalerValue = 0;
  uwPrescalerValue = (uint32_t)((SystemCoreClock / 2) / 10000) - 1;

  /* Set TIMx instance */
  TimHandle.Instance = TIM3;

  /* Initialize TIMx peripheral as follows:
       + Period = 10000 - 1
       + Prescaler = ((SystemCoreClock / 2)/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  TimHandle.Init.Period            = 10000 - 1;
  TimHandle.Init.Prescaler         = uwPrescalerValue;
  TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  TimHandle.Init.RepetitionCounter = 0;
  TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if (HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3) {
    if (synth_available()) {
      BSP_LED_Toggle(LED1);
    }
  }
}

/**
  * @brief  Clock Config.
  * @param  hsai: might be required to set audio peripheral predivider if any.
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @note   This API is called by BSP_AUDIO_OUT_Init() and BSP_AUDIO_OUT_SetFrequency()
  *         Being __weak it can be overwritten by the application
  * @retval None
  */
void BSP_AUDIO_OUT_ClockConfig(SAI_HandleTypeDef *hsai, uint32_t AudioFreq, void *Params)
{
  RCC_PeriphCLKInitTypeDef RCC_ExCLKInitStruct;

  HAL_RCCEx_GetPeriphCLKConfig(&RCC_ExCLKInitStruct);

  /* Set the PLL configuration according to the audio frequency */
  if ((AudioFreq == AUDIO_FREQUENCY_11K) || (AudioFreq == AUDIO_FREQUENCY_22K) || (AudioFreq == AUDIO_FREQUENCY_44K))
  {
    /* Configure PLLSAI prescalers */
    /* PLLI2S_VCO: VCO_429M
    SAI_CLK(first level) = PLLI2S_VCO/PLLSAIQ = 429/2 = 214.5 Mhz
    SAI_CLK_x = SAI_CLK(first level)/PLLI2SDivQ = 214.5/19 = 11.289 Mhz */
    RCC_ExCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI2;
    RCC_ExCLKInitStruct.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLI2S;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SP = 8;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SN = 429;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SQ = 2;
    RCC_ExCLKInitStruct.PLLI2SDivQ = 19;
    HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct);
  }
  else /* AUDIO_FREQUENCY_8K, AUDIO_FREQUENCY_16K, AUDIO_FREQUENCY_48K), AUDIO_FREQUENCY_96K */
  {
    /* SAI clock config
    PLLI2S_VCO: VCO_344M
    SAI_CLK(first level) = PLLI2S_VCO/PLLSAIQ = 344/7 = 49.142 Mhz
    SAI_CLK_x = SAI_CLK(first level)/PLLI2SDivQ = 49.142/1 = 49.142 Mhz */
    RCC_ExCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI2;
    RCC_ExCLKInitStruct.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLI2S;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SP = 8;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SN = 344;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SQ = 7;
    RCC_ExCLKInitStruct.PLLI2SDivQ = 1;
    HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct);
  }
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 200000000
  *            HCLK(Hz)                       = 200000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLL_Q                          = 8
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 6
  * @param  None
  * @retval None
  */


/**
  * @brief TIM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  * @param htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  /*##-1- Enable peripheral clock #################################*/
  /* TIMx Peripheral clock enable */
  __HAL_RCC_TIM3_CLK_ENABLE();

  /*##-2- Configure the NVIC for TIMx ########################################*/
  /* Set the TIMx priority */
  HAL_NVIC_SetPriority(TIM3_IRQn, 3, 0);

  /* Enable the TIMx global Interrupt */
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}


// 200 mhz - don't know why but 216 mhz hang
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
#ifdef FREQ_216
  RCC_OscInitStruct.PLL.PLLN = 432;
#else
  RCC_OscInitStruct.PLL.PLLN = 400;
#endif
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
#ifdef FREQ_216
  RCC_OscInitStruct.PLL.PLLQ = 9;
#else
  RCC_OscInitStruct.PLL.PLLQ = 8;
#endif

  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if (ret != HAL_OK)
  {
    while (1) { ; }
  }

  /* Activate the OverDrive to reach the 200 MHz Frequency */
  ret = HAL_PWREx_EnableOverDrive();
  if (ret != HAL_OK)
  {
    while (1) { ; }
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

#ifdef FREQ_216
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
#else
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6);
#endif

  if (ret != HAL_OK)
  {
    while (1) { ; }
  }
}

/**
  * @brief  Configure the MPU attributes as Write Through for SDRAM.
  * @note   The Base Address for SDRAM is 0xC0000000.
  *         The Region Size is 8MB, it is related to SDRAM memory size.
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WT for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x20010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;

  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable branch prediction */
  SCB->CCR |= (1 << 18);
  __DSB();

  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  while (1)
  {
    /* LED1 blinks */
    BSP_LED_Toggle(LED1);
    HAL_Delay(100);
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
