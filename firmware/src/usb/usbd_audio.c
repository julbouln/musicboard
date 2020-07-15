/**
  ******************************************************************************
  * @file    usbd_audio.c
  * @author  MCD Application Team
  * @brief   This file provides the Audio core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                AUDIO Class  Description
  *          ===================================================================
  *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
  *           Audio Devices V1.0 Mar 18, 98".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Standard AC Interface Descriptor management
  *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
  *             - 1 Audio Streaming Endpoint
  *             - 1 Audio Terminal Input (1 channel)
  *             - Audio Class-Specific AC Interfaces
  *             - Audio Class-Specific AS Interfaces
  *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
  *             - Audio Feature Unit (limited to Mute control)
  *             - Audio Synchronization type: Asynchronous
  *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
  *          The current audio class version supports the following audio features:
  *             - Pulse Coded Modulation (PCM) format
  *             - sampling rate: 48KHz.
  *             - Bit resolution: 16
  *             - Number of channels: 2
  *             - No volume control
  *             - Mute/Unmute capability
  *             - Asynchronous Endpoints
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
- "stm32xxxxx_{eval}{discovery}_audio.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usbd_ctlreq.h"

#include "hal_usb_ex.h"
#include "audio_buffer.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_AUDIO
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_AUDIO_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Macros
  * @{
  */
#define AUDIO_SAMPLE_FREQ(frq)         (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

/*#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2U * 2U)/1000U) & 0xFFU), \
                                       (uint8_t)((((frq * 2U * 2U)/1000U) >> 8) & 0xFFU)
*/
#define AUDIO_PACKET_SZE(val)          (uint8_t)((val) & 0xFF), \
                                       (uint8_t)(((val) >> 8) & 0xFF)
  
/**
  * @}
  */

// Async ISOC
static volatile  uint16_t SOF_num=0;

volatile uint32_t tx_flag = 1;
volatile uint32_t fnsof = 0;

/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
  * @{
  */
uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);

uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length);
uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);

uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
void AUDIO_REQ_GetMin(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
void AUDIO_REQ_GetMax(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
void AUDIO_REQ_GetRes(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

uint32_t USBD_AUDIO_GetFeedback(USBD_HandleTypeDef *pdev);
/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Variables
  * @{
  */

USBD_ClassTypeDef USBD_AUDIO =
{
  USBD_AUDIO_Init,
  USBD_AUDIO_DeInit,
  USBD_AUDIO_Setup,
  USBD_AUDIO_EP0_TxReady,
  USBD_AUDIO_EP0_RxReady,
  USBD_AUDIO_DataIn,
  USBD_AUDIO_DataOut,
  USBD_AUDIO_SOF,
  USBD_AUDIO_IsoINIncomplete,
  USBD_AUDIO_IsoOutIncomplete,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetDeviceQualifierDesc,
};

/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_AUDIO_CfgDesc[USB_AUDIO_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /* Configuration 1 */
  0x09,                                 /* bLength */
  USB_DESC_TYPE_CONFIGURATION,          /* bDescriptorType */
  LOBYTE(USB_AUDIO_CONFIG_DESC_SIZ),    /* wTotalLength  109 bytes*/
  HIBYTE(USB_AUDIO_CONFIG_DESC_SIZ),
  0x02,                                 /* bNumInterfaces */
  0x01,                                 /* bConfigurationValue */
  0x00,                                 /* iConfiguration */
  0xC0,                                 /* bmAttributes  BUS Powred*/
  0x32,                                 /* bMaxPower = 100 mA*/
  /* 09 byte*/

  /* USB Speaker Standard interface descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  0x00,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Speaker Class-specific AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */
  0x00,          /* 1.00 */             /* bcdADC */
  0x01,
  0x27,                                 /* wTotalLength = 39*/
  0x00,
  0x01,                                 /* bInCollection */
  0x01,                                 /* baInterfaceNr */
  /* 09 byte*/

  /* USB Speaker Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  0x01,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
  0x01,
  0x00,                                 /* bAssocTerminal */
  0x01,                                 /* bNrChannels */
  0x00,                                 /* wChannelConfig 0x0000  Mono */
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* iTerminal */
  /* 12 byte*/

  /* USB Speaker Audio Feature Unit Descriptor */
  0x09,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  AUDIO_OUT_FEATURE_ID,                 /* bUnitID */
  0x01,                                 /* bSourceID */
  0x01,                                 /* bControlSize */
  AUDIO_CONTROL_MUTE | AUDIO_CONTROL_VOLUME, /* bmaControls(0) */
  0,                                    /* bmaControls(1) */
  0x00,                                 /* iTerminal */
  /* 09 byte*/

  /*USB Speaker Output Terminal Descriptor */
  0x09,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  0x03,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType  0x0301*/
  0x03,
  0x00,                                 /* bAssocTerminal */
  0x02,                                 /* bSourceID */
  0x00,                                 /* iTerminal */
  /* 09 byte*/

  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwith */
  /* Interface 1, Alternate Setting 0                                             */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Interface 1, Alternate Setting 1                                           */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x01,                                 /* bAlternateSetting */
#ifdef USB_AUDIO_ASYNC_ENABLE
  0x02,                                 /* bNumEndpoints */
#else
  0x01,
#endif
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Speaker Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  0x01,                                 /* bTerminalLink */
  0x01,                                 /* bDelay */
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001 */
  0x00,
  /* 07 byte*/

  /* USB Speaker Audio Type III Format Interface Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */
  0x02,                                 /* bNrChannels */
  0x02,                                 /* bSubFrameSize :  2 Bytes per frame (16bits) */
  16,                                   /* bBitResolution (16-bits per sample) */
  0x01,                                 /* bSamFreqType only one frequency supported */
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),   /* Audio sampling frequency coded on 3 bytes */
  /* 11 byte*/

  /* Endpoint 1 - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_OUT_EP,                         /* bEndpointAddress 1 out endpoint */
  USBD_EP_TYPE_ISOC|USBD_EP_ATTR_ISOC_ASYNC,                    /* bmAttributes */
  AUDIO_PACKET_SZE(AUDIO_OUT_PACKET),    /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  AUDIO_FS_BINTERVAL,                   /* bInterval */
  0x00,                                 /* bRefresh */
#ifdef USB_AUDIO_ASYNC_ENABLE
  AUDIO_FEEDBACK_EP,                                 /* bSynchAddress */
#else
  0x00,
#endif
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00,
  /* 07 byte*/

#ifdef USB_AUDIO_ASYNC_ENABLE
    /* Endpoint 2 - Standard Descriptor - See UAC Spec 1.0 p.63 4.6.2.1 Standard AS Isochronous Synch Endpoint Descriptor */
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE, /* bLength */
    USB_DESC_TYPE_ENDPOINT,            /* bDescriptorType */
    AUDIO_FEEDBACK_EP,                       /* bEndpointAddress */
    0x11,                              /* bmAttributes */
    0x03, 0x00,                        /* wMaxPacketSize in Bytes */
    0x01,                              /* bInterval 1ms */
    SOF_FEEDBACK_RATE,                 /* bRefresh = 2^SOF_FEEDBACK_RATE ms */
    0x00,                              /* bSynchAddress */
    /* 09 byte*/
#endif
} ;

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Functions
  * @{
  */

/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);
  USBD_AUDIO_HandleTypeDef *haudio;

    /* Allocate Audio structure */
  haudio = USBD_malloc(sizeof(USBD_AUDIO_HandleTypeDef));

  if (haudio == NULL)
  {
    pdev->pClassData[0] = NULL;
    return (uint8_t)USBD_EMEM;
  }

  pdev->pClassData[0] = (void *)haudio;

  if (pdev->dev_speed == USBD_SPEED_HIGH)
  {
    pdev->ep_out[AUDIO_OUT_EP & 0xFU].bInterval = AUDIO_HS_BINTERVAL;
  }
  else   /* LOW and FULL-speed endpoints */
  {
    pdev->ep_out[AUDIO_OUT_EP & 0xFU].bInterval = AUDIO_FS_BINTERVAL;
  }

  /* Open EP OUT */
  (void)USBD_LL_OpenEP(pdev, AUDIO_OUT_EP, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET);

#ifdef USB_AUDIO_ASYNC_ENABLE
  /* Open EP for Sync */
  USBD_LL_OpenEP(pdev, AUDIO_FEEDBACK_EP, USBD_EP_TYPE_ISOC, 3);
  /* Flush EP for Sync */
  USBD_LL_FlushEP(pdev, AUDIO_FEEDBACK_EP);
#endif

  pdev->ep_out[AUDIO_OUT_EP & 0xFU].is_used = 1U;

  haudio->alt_setting = 0U;
  haudio->offset = AUDIO_OFFSET_UNKNOWN;
  haudio->wr_ptr = 0U;
  haudio->rd_ptr = 0U;
  haudio->rd_enable = 0U;
  haudio->freq = USBD_AUDIO_FREQ;
  haudio->vol = USBD_AUDIO_VOL_DEFAULT;
  haudio->mute = 0;
#ifdef USB_AUDIO_ASYNC_ENABLE
  haudio->feedback_val = (USBD_AUDIO_FREQ << 14) / 1000;
#endif

  /* Initialize the Audio output Hardware layer */
  if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[0])->Init(USBD_AUDIO_FREQ,
                                                       VOL_PERCENT(haudio->vol),
                                                       0U) != 0U)
  {
    return (uint8_t)USBD_FAIL;
  }

#ifdef USB_AUDIO_ASYNC_ENABLE
  USBD_LL_Transmit(pdev, AUDIO_FEEDBACK_EP, (uint8_t*)(&haudio->feedback_val), 3);
#endif

  /* Prepare Out endpoint to receive 1st packet */
  (void)USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, haudio->buffer, AUDIO_OUT_PACKET);

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Init
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  /* Open EP OUT */
  (void)USBD_LL_CloseEP(pdev, AUDIO_OUT_EP);

#ifdef USB_AUDIO_ASYNC_ENABLE
  /* Close EP IN */
  USBD_LL_CloseEP(pdev, AUDIO_FEEDBACK_EP);
#endif

  pdev->ep_out[AUDIO_OUT_EP & 0xFU].is_used = 0U;
  pdev->ep_out[AUDIO_OUT_EP & 0xFU].bInterval = 0U;

  /* DeInit  physical Interface components */
  if (pdev->pClassData[0] != NULL)
  {
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[0])->DeInit(0U);
    (void)USBD_free(pdev->pClassData[0]);
    pdev->pClassData[0] = NULL;
  }

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  USBD_StatusTypeDef ret = USBD_OK;

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData[0];

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS:
    switch (req->bRequest)
    {
    case AUDIO_REQ_GET_CUR:
      AUDIO_REQ_GetCurrent(pdev, req);
      break;
    case AUDIO_REQ_GET_MIN:
      AUDIO_REQ_GetMin(pdev, req);
      break;
    case AUDIO_REQ_GET_MAX:
      AUDIO_REQ_GetMax(pdev, req);
      break;
    case AUDIO_REQ_GET_RES:
      AUDIO_REQ_GetRes(pdev, req);
      break;
    case AUDIO_REQ_SET_CUR:
      AUDIO_REQ_SetCurrent(pdev, req);
      break;
    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_DESCRIPTOR:
      if ((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_AUDIO_CfgDesc + 18;
        len = MIN(USB_AUDIO_DESC_SIZ, req->wLength);

        (void)USBD_CtlSendData(pdev, pbuf, len);
      }
      break;

    case USB_REQ_GET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        (void)USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
      }
      else
      {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES)
        {
          haudio->alt_setting = (uint8_t)(req->wValue);
          USBD_LL_FlushEP(pdev, AUDIO_FEEDBACK_EP);

          if (haudio->alt_setting == 1) {
            SOF_num=0;
          }

        }
        else
        {
          /* Call the error management function (command will be nacked */
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
        }
      }
      else
      {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_CLEAR_FEATURE:
      break;

    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;
  default:
    USBD_CtlError(pdev, req);
    ret = USBD_FAIL;
    break;
  }

  return (uint8_t)ret;
}


/**
  * @brief  USBD_AUDIO_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_AUDIO_CfgDesc);

  return USBD_AUDIO_CfgDesc;
}

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData[0];

  if (epnum == (AUDIO_FEEDBACK_EP&0x7f))
  {  

    tx_flag = 0;
#ifdef USB_AUDIO_ASYNC_ENABLE
//      haudio->feedback_val = audio_buffer_getfeedback();
//      USBD_LL_Transmit(pdev, AUDIO_FEEDBACK_EP, (uint8_t*)(&haudio->feedback_val), 3);
#endif
  }

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData[0];

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR) {
    switch (haudio->control.dest ) {
    /* In this driver, to simplify code, only SET_CUR request is managed */

    case AUDIO_REQ_CONTROL:
      //if (haudio->control.unit == AUDIO_OUT_FEATURE_ID)
    {
      switch (haudio->control.cs) {
      case AUDIO_CONTROL_MUTE:
        haudio->mute = haudio->control.data[0];
        ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[0])->MuteCtl(haudio->mute);
        break;
      case AUDIO_CONTROL_VOLUME:
      {
        int16_t vol = *(int16_t*)&haudio->control.data[0];
        haudio->vol = vol;
        ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[0])->VolumeCtl(VOL_PERCENT(haudio->vol));
      }
      break;
      }
      haudio->control.cmd = 0;
      haudio->control.len = 0;
    }
    break;
    case AUDIO_REQ_STREAMING:
      break;

    }
  }

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  /* Only OUT control data are processed */
  return (uint8_t)USBD_OK;
}
/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */

#define AUDIO_FB_DEFAULT (48 << 14)
#define AUDIO_FB_DELTA (uint32_t)(1 << 14)

uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData[0];

#ifdef USB_AUDIO_ASYNC_ENABLE

  if (haudio->alt_setting == 1)
  {

    SOF_num++;
//    if (SOF_num == (1 << SOF_FEEDBACK_RATE))
    if (SOF_num == 1)
    {
      SOF_num = 0;

      haudio->feedback_val = audio_buffer_getfeedback();

      // min/max
      if (haudio->feedback_val > AUDIO_FB_DEFAULT + AUDIO_FB_DELTA) {
        haudio->feedback_val = AUDIO_FB_DEFAULT + AUDIO_FB_DELTA;
      } else if (haudio->feedback_val < AUDIO_FB_DEFAULT - AUDIO_FB_DELTA) {
        haudio->feedback_val = AUDIO_FB_DEFAULT - AUDIO_FB_DELTA;
      }
    }

    if (tx_flag == 0) {
      USB_OTG_GlobalTypeDef* USBx = USB_OTG_FS;
      uint32_t USBx_BASE = (uint32_t)USBx;
      uint32_t volatile fnsof_new = (USBx_DEVICE->DSTS & USB_OTG_DSTS_FNSOF) >> 8;

      if ((fnsof & 0x1) == (fnsof_new & 0x1)) {
        USBD_LL_Transmit(pdev, AUDIO_FEEDBACK_EP, (uint8_t*)(&haudio->feedback_val), 3);
        tx_flag = 1U;
      }
    }
  }

#endif

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */


// called 100x seconds
void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset)
{
}

/**
  * @brief  USBD_AUDIO_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */

uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData[0];



#ifdef USB_AUDIO_ASYNC_ENABLE
  USB_OTG_GlobalTypeDef* USBx = USB_OTG_FS;
  uint32_t USBx_BASE = (uint32_t)USBx;
  fnsof = (USBx_DEVICE->DSTS & USB_OTG_DSTS_FNSOF) >> 8;

  if (tx_flag == 1U) {
    tx_flag = 0U;
    USBD_LL_FlushEP(pdev, AUDIO_FEEDBACK_EP);
  }

#if 0
  USB_DIEPCTL(AUDIO_FEEDBACK_EP) |= (USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK);
  while (USB_DIEPCTL(AUDIO_FEEDBACK_EP) & USB_OTG_DIEPCTL_EPENA);

  USBD_LL_FlushEP(pdev, AUDIO_FEEDBACK_EP);

  haudio->feedback_val = audio_buffer_getfeedback();
  USBD_LL_Transmit(pdev, AUDIO_FEEDBACK_EP, (uint8_t*)&haudio->feedback_val, 3);

  SOF_num = 0;
#endif
#endif
  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData[0];


  USB_DOEPCTL(AUDIO_OUT_EP) |= USB_OTG_DOEPCTL_EPDIS;
  //  USB_DOEPCTL(AUDIO_OUT_EP) |= (USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK);
  //  while (USB_DOEPCTL(AUDIO_OUT_EP) & USB_OTG_DOEPCTL_EPENA);

  USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, &haudio->buffer[0], AUDIO_OUT_PACKET);  

#ifdef USB_AUDIO_ASYNC_ENABLE
  audio_buffer_fill(0,(haudio->feedback_val >> 14) * 8);
#endif

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  uint16_t PacketSize;
  USBD_AUDIO_HandleTypeDef *haudio;

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData[0];

  if (epnum == AUDIO_OUT_EP)
  {
    /* Get received data packet length */
    PacketSize = (uint16_t)USBD_LL_GetRxDataSize(pdev, epnum);

    audio_buffer_feed(&haudio->buffer[0], PacketSize);

    (void)USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP,
                                 &haudio->buffer[0],
                                 AUDIO_OUT_PACKET);
  }

  return (uint8_t)USBD_OK;
}

/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData[0];

  haudio->control.cmd = AUDIO_REQ_GET_CUR;     /* Set the request value */
  haudio->control.len = req->wLength;     /* Set the request data length */
  haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
  haudio->control.cs = HIBYTE(req->wValue);
  haudio->control.dest = req->bmRequest & 0x1f;

  int len = 0;
  uint8_t data[8];
  int v;
  switch (haudio->control.dest ) {
  case AUDIO_REQ_CONTROL:
//    if (haudio->control.unit == AUDIO_OUT_FEATURE_ID) 
  {
      switch (haudio->control.cs) {
      case AUDIO_CONTROL_MUTE:
        USBD_CtlSendData(pdev, (uint8_t *)&haudio->mute, 1);
        break;
      case AUDIO_CONTROL_VOLUME:
        USBD_CtlSendData(pdev, (uint8_t *)&haudio->vol, 2);
        break;
      }
      haudio->control.cmd = 0;
      haudio->control.len = 0;
    }
    break;
  case AUDIO_REQ_STREAMING:
    v = haudio->freq;
    len = 3;
    data[0] = v & 0xff;
    data[1] = v >> 8;
    data[2] = v >> 16;
    USBD_CtlSendData(pdev, data, len);

    haudio->control.cmd = 0;
    haudio->control.len = 0;

    break;
  }

}


void AUDIO_REQ_GetMin(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData[0];

  haudio->control.cmd = AUDIO_REQ_GET_MIN;     /* Set the request value */
  haudio->control.len = req->wLength;          /* Set the request data length */
  haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
  haudio->control.cs = HIBYTE(req->wValue);
  haudio->control.dest = req->bmRequest & 0x1f;

  switch (haudio->control.dest ) {
  case AUDIO_REQ_CONTROL:
    // if (haudio->control.unit == AUDIO_OUT_FEATURE_ID) 
    {
      switch (haudio->control.cs) {
        case AUDIO_CONTROL_VOLUME:
        {
          int16_t vol_min = USBD_AUDIO_VOL_MIN;
          USBD_CtlSendData(pdev, (uint8_t*)&vol_min, 2);
        }
          break;
      }
      haudio->control.cmd = 0;
      haudio->control.len = 0;
    }
    break;
  }
}

void AUDIO_REQ_GetMax(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData[0];

  haudio->control.cmd = AUDIO_REQ_GET_MAX;     /* Set the request value */
  haudio->control.len = req->wLength;          /* Set the request data length */
  haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
  haudio->control.cs = HIBYTE(req->wValue);
  haudio->control.dest = req->bmRequest & 0x1f;

  switch (haudio->control.dest ) {
  case AUDIO_REQ_CONTROL:
    // if (haudio->control.unit == AUDIO_OUT_FEATURE_ID) 
    {
      switch (haudio->control.cs) {
      case AUDIO_CONTROL_VOLUME:
        {
          int16_t vol_max = USBD_AUDIO_VOL_MAX;
          USBD_CtlSendData(pdev, (uint8_t*)&vol_max, 2);
        }
        break;
      }
      haudio->control.cmd = 0;
      haudio->control.len = 0;
    }
    break;
  }
}


/**
 * @brief  AUDIO_Req_GetRes
 *         Handles the GET_RES Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
void AUDIO_REQ_GetRes(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req)
{
  if ((req->bmRequest & 0x1f) == AUDIO_REQ_CONTROL) {
    switch (HIBYTE(req->wValue)) {
      case AUDIO_CONTROL_VOLUME: {
        int16_t vol_res = USBD_AUDIO_VOL_STEP;
        USBD_CtlSendData(pdev, (uint8_t*)&vol_res, 2);
      };
      break;
    }
  }
}

/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData[0];

  if (req->wLength != 0U)
  {
    /* Prepare the reception of the buffer over EP0 */
    (void)USBD_CtlPrepareRx(pdev, haudio->control.data, req->wLength);

    haudio->control.cmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
    haudio->control.len = req->wLength;          /* Set the request data length */
    haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
    haudio->control.cs = HIBYTE(req->wValue);
    haudio->control.dest = req->bmRequest & 0x1f;
  }
}


/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_AUDIO_DeviceQualifierDesc);

  return USBD_AUDIO_DeviceQualifierDesc;
}

/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops)
{
  if (fops == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  pdev->pUserData[0] = fops;

  return (uint8_t)USBD_OK;
}
/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
