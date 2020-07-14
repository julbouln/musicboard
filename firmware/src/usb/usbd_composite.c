#include "usbd_composite.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"

static uint8_t  USBD_Composite_Init (USBD_HandleTypeDef *pdev, 
                               uint8_t cfgidx);
static uint8_t  USBD_Composite_DeInit (USBD_HandleTypeDef *pdev, 
                                 uint8_t cfgidx);
static uint8_t  USBD_Composite_Setup (USBD_HandleTypeDef *pdev, 
                                USBD_SetupReqTypedef *req);
static uint8_t  *USBD_Composite_GetCfgDesc (uint16_t *length);
static uint8_t  *USBD_Composite_GetDeviceQualifierDesc (uint16_t *length);
static uint8_t  USBD_Composite_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_Composite_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_Composite_EP0_RxReady (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_Composite_EP0_TxReady (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_Composite_SOF (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_Composite_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_Composite_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);


/* USB audio */
extern uint8_t  USBD_AUDIO_Init (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);
extern uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev,
                                   uint8_t cfgidx);
extern uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                  USBD_SetupReqTypedef *req);
extern uint8_t  *USBD_AUDIO_GetCfgDesc (uint16_t *length);
extern uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length);
extern uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t  USBD_AUDIO_EP0_RxReady (USBD_HandleTypeDef *pdev);
extern uint8_t  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev);
extern uint8_t  USBD_AUDIO_SOF (USBD_HandleTypeDef *pdev);
extern uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

#define AUDIO_SAMPLE_FREQ(frq)      (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(val)          (uint8_t)((val) & 0xFF), \
                                       (uint8_t)(((val) >> 8) & 0xFF)

/* USB midi */
extern uint8_t  USBD_Midi_Init (USBD_HandleTypeDef *pdev, 
                               uint8_t cfgidx);
extern uint8_t  USBD_Midi_DeInit (USBD_HandleTypeDef *pdev, 
                                 uint8_t cfgidx);
extern uint8_t  USBD_Midi_Setup (USBD_HandleTypeDef *pdev, 
                                USBD_SetupReqTypedef *req);
extern uint8_t  *USBD_Midi_GetCfgDesc (uint16_t *length);
extern uint8_t  *USBD_Midi_GetDeviceQualifierDesc (uint16_t *length);
extern uint8_t  USBD_Midi_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t  USBD_Midi_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t  USBD_Midi_EP0_RxReady (USBD_HandleTypeDef *pdev);
extern uint8_t  USBD_Midi_EP0_TxReady (USBD_HandleTypeDef *pdev);
extern uint8_t  USBD_Midi_SOF (USBD_HandleTypeDef *pdev);
extern uint8_t  USBD_Midi_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t  USBD_Midi_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

/* USB MSC */
extern uint8_t USBD_MSC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
extern uint8_t USBD_MSC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
extern uint8_t USBD_MSC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
extern uint8_t USBD_MSC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t USBD_MSC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);

extern uint8_t *USBD_MSC_GetHSCfgDesc(uint16_t *length);
extern uint8_t *USBD_MSC_GetFSCfgDesc(uint16_t *length);
extern uint8_t *USBD_MSC_GetOtherSpeedCfgDesc(uint16_t *length);
extern uint8_t *USBD_MSC_GetDeviceQualifierDescriptor(uint16_t *length);


extern USBD_AUDIO_ItfTypeDef  USBD_AUDIO_fops;
extern USBD_Midi_ItfTypeDef  USBD_Midi_fops;

USBD_Composite_ItfTypeDef Composite = 
{
  &USBD_AUDIO_fops,
  &USBD_Midi_fops,
};

USBD_ClassTypeDef  USBD_Composite_ClassDriver = 
{
  USBD_Composite_Init,
  USBD_Composite_DeInit,
  USBD_Composite_Setup,
  USBD_Composite_EP0_TxReady,  
  USBD_Composite_EP0_RxReady,
  USBD_Composite_DataIn,
  USBD_Composite_DataOut,
  USBD_Composite_SOF,
  USBD_Composite_IsoINIncomplete,
  USBD_Composite_IsoOutIncomplete,      
  USBD_Composite_GetCfgDesc,
  USBD_Composite_GetCfgDesc, 
  USBD_Composite_GetCfgDesc,
  USBD_Composite_GetDeviceQualifierDesc,
};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif

static uint8_t USBD_Composite_CfgDesc[USB_COMPOSITE_CONFIG_DESC_SIZ] =
{
// configuration descriptor
  0x09,
  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,
  LOBYTE(USB_COMPOSITE_CONFIG_DESC_SIZ),
  HIBYTE(USB_COMPOSITE_CONFIG_DESC_SIZ),
#ifndef MUCISBOARD_USB_MSC_DISABLE
  0x05,   /* bNumInterfaces : 2 audio + 2 midi + 1 msc*/
#else
  0x04,   /* bNumInterfaces : 2 audio + 2 midi */
#endif
  0x01,   /* bConfigurationValue */
  0x00,  /* iConfiguration */
  0xc0,  /* bmAttributes  BUS Powred*/
  0x50,  /* bMaxPower = 100 mA*/

  /* USB AUDIO */

  /* USB Speaker Standard interface descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  AUDIO_INTERFACE0,                     /* bInterfaceNumber */
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
  AUDIO_OUT_FEATURE_ID,             /* bUnitID */
  0x01,                                 /* bSourceID */
  0x01,                                 /* bControlSize */
  AUDIO_CONTROL_MUTE | AUDIO_CONTROL_VOLUME, /* bmaControls(0) */
  0,                                    /* bmaControls(1) */
  0x00,                                 /* iTerminal */
  /* 09 byte*/

  /*USB Speaker Output Terminal Descriptor */
  0x09,      /* bLength */
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
  AUDIO_INTERFACE1,                     /* bInterfaceNumber */
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
  AUDIO_INTERFACE1,                     /* bInterfaceNumber */
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
    AUDIO_FEEDBACK_EP,                 /* bEndpointAddress */
    0x11,                              /* bmAttributes */
    0x03, 0x00,                        /* wMaxPacketSize in Bytes */
    0x01,                              /* bInterval 1ms */
    SOF_RATE,                          /* bRefresh 4ms = 2^2 */
    0x00,                              /* bSynchAddress */
    /* 09 byte*/
#endif
  /* MIDI */

  // The Audio Interface Collection
  // Standard AC Interface Descriptor
  MIDI_INTERFACE_DESC_SIZE,
  USB_DESC_TYPE_INTERFACE,
  MIDI_INTERFACE0,
  0x00,
  0x00,
  0x01,
  0x01,
  0x00,
  0x00,

  // Class-specific AC Interface Descriptor
  MIDI_INTERFACE_DESC_SIZE,
  MIDI_INTERFACE_DESCRIPTOR_TYPE,
  0x01,
  0x00,
  0x01,
  0x09,
  0x00,
  0x01,
  0x01,

  // MIDIStreaming Interface Descriptors
  MIDI_INTERFACE_DESC_SIZE,
  USB_DESC_TYPE_INTERFACE,
  MIDI_INTERFACE1,
  0x00,
  0x02,
  0x01,
  0x03,
  0x00,
  0x00,

  // Class-Specific MS Interface Header Descriptor
  0x07,
  0x24,
  0x01,
  0x00,
  0x01,
  0x41,
  0x00,

  // MIDI IN JACKS
  0x06,
  0x24,
  0x02,
  0x01,
  0x01,
  0x00,
  0x06,
  0x24,
  0x02,
  0x02,
  0x02,
  0x00,

  // MIDI OUT JACKS
  0x09,
  0x24,
  0x03,
  0x01,
  0x03,
  0x01,
  0x02,
  0x01,
  0x00,

  0x09,
  0x24,
  0x03,
  0x02,
  0x06,
  0x01,
  0x01,
  0x01,
  0x00,

  // OUT endpoint descriptor
  MIDI_STANDARD_ENDPOINT_DESC_SIZE,
  USB_DESC_TYPE_ENDPOINT,
  MIDI_OUT_EP,
  0x02,
  0x40,
  0x00,
  0x00,
  0x00,
  0x00,
  0x05,
  0x25,
  0x01,
  0x01,
  0x01,

  // IN endpoint descriptor
  MIDI_STANDARD_ENDPOINT_DESC_SIZE,
  USB_DESC_TYPE_ENDPOINT,
  MIDI_IN_EP,
  0x02,
  0x40,
  0x00,
  0x00,
  0x00,
  0x00,
  0x05,
  0x25,
  0x01,
  0x01,
  0x03,

#ifndef MUCISBOARD_USB_MSC_DISABLE
  // MASS STORAGE
  /********************  Mass Storage interface ********************/
  0x09,                                            /* bLength: Interface Descriptor size */
  0x04,                                            /* bDescriptorType: */
  MSC_INTERFACE,                                            /* bInterfaceNumber: Number of Interface */
  0x00,                                            /* bAlternateSetting: Alternate setting */
  0x02,                                            /* bNumEndpoints*/
  0x08,                                            /* bInterfaceClass: MSC Class */
  0x06,                                            /* bInterfaceSubClass : SCSI transparent*/
  0x50,                                            /* nInterfaceProtocol */
  0x05,                                            /* iInterface: */
  /********************  Mass Storage Endpoints ********************/
  0x07,                                            /* Endpoint descriptor length = 7 */
  0x05,                                            /* Endpoint descriptor type */
  MSC_IN_EP,                                   /* Endpoint address (IN, address 1) */
  0x02,                                            /* Bulk endpoint type */
  LOBYTE(MSC_MAX_FS_PACKET),
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00,                                            /* Polling interval in milliseconds */

  0x07,                                            /* Endpoint descriptor length = 7 */
  0x05,                                            /* Endpoint descriptor type */
  MSC_OUT_EP,                                  /* Endpoint address (OUT, address 1) */
  0x02,                                            /* Bulk endpoint type */
  LOBYTE(MSC_MAX_FS_PACKET),
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00                                             /* Polling interval in milliseconds */
#endif
};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
/* USB Standard Device Descriptor */
static uint8_t USBD_Composite_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] =
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

/** @defgroup USBD_Composite_Private_Functions
  * @{
  */ 

/**
  * @brief  USBD_Composite_Init
  *         Initialize the Midi interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_Composite_Init (USBD_HandleTypeDef *pdev, 
                               uint8_t cfgidx)
{

  USBD_AUDIO_Init(pdev, cfgidx);
  USBD_Midi_Init(pdev, cfgidx);
#ifndef MUCISBOARD_USB_MSC_DISABLE
  USBD_MSC_Init(pdev, cfgidx);
#endif

  return USBD_OK;
}

/**
  * @brief  USBD_Composite_Init
  *         DeInitialize the Midi layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_Composite_DeInit (USBD_HandleTypeDef *pdev, 
                                 uint8_t cfgidx)
{

  USBD_AUDIO_DeInit(pdev, cfgidx);
  USBD_Midi_DeInit(pdev, cfgidx);
#ifndef MUCISBOARD_USB_MSC_DISABLE
  USBD_MSC_DeInit(pdev, cfgidx);
#endif

  return USBD_OK;
}

/**
  * @brief  USBD_Composite_Setup
  *         Handle the Midi specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_Composite_Setup (USBD_HandleTypeDef *pdev, 
                                USBD_SetupReqTypedef *req)
{

  switch(req->wIndex & 0xff) {
    case AUDIO_INTERFACE0:
    case AUDIO_INTERFACE1:
      return (USBD_AUDIO_Setup (pdev, req));
      break;
    case MIDI_INTERFACE0:
    case MIDI_INTERFACE1:
      return (USBD_Midi_Setup(pdev, req));
      break;
#ifndef MUCISBOARD_USB_MSC_DISABLE
    case MSC_INTERFACE:
      return (USBD_MSC_Setup(pdev, req));
      break;
#endif
  }

  return USBD_FAIL;

}


/**
  * @brief  USBD_Composite_GetCfgDesc 
  *         return configuration descriptor
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_Composite_GetCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_Composite_CfgDesc);
  return USBD_Composite_CfgDesc;
}

/**
* @brief  DeviceQualifierDescriptor 
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_Composite_DeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (USBD_Composite_DeviceQualifierDesc);
  return USBD_Composite_DeviceQualifierDesc;
}


/**
  * @brief  USBD_Composite_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_Composite_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
//      return (USBD_AUDIO_DataIn(pdev, epnum));
#if 1
  switch(epnum|0x80) {
    case AUDIO_FEEDBACK_EP:
      return (USBD_AUDIO_DataIn(pdev, epnum));
      break;
    case MIDI_IN_EP:
      return (USBD_Midi_DataIn(pdev, epnum));
      break;
#ifndef MUCISBOARD_USB_MSC_DISABLE
    case MSC_IN_EP:
      return (USBD_MSC_DataIn(pdev, epnum));
      break;
#endif
  }
#endif
  return USBD_FAIL;
}

/**
  * @brief  USBD_Composite_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_Composite_EP0_RxReady (USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_EP0_RxReady(pdev);
  USBD_Midi_EP0_RxReady(pdev);
  return USBD_OK;
}
/**
  * @brief  USBD_Composite_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_Composite_EP0_TxReady (USBD_HandleTypeDef *pdev)
{
  return USBD_OK;
}
/**
  * @brief  USBD_Composite_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_Composite_SOF (USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_SOF(pdev);
  USBD_Midi_SOF(pdev);
  return USBD_OK;
}
/**
  * @brief  USBD_Composite_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_Composite_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_AUDIO_IsoINIncomplete(pdev, epnum);
#if 0
  switch(epnum|0x80) {
    case AUDIO_FEEDBACK_EP:
      return (USBD_AUDIO_IsoINIncomplete(pdev, epnum));
      break;
    break;
  }
#endif
  return USBD_OK;
}
/**
  * @brief  USBD_Composite_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_Composite_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  return USBD_OK;
}
/**
  * @brief  USBD_Composite_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_Composite_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  switch(epnum) {
    case AUDIO_OUT_EP:
      return (USBD_AUDIO_DataOut(pdev, epnum));
      break;
    case MIDI_OUT_EP:
      return (USBD_Midi_DataOut(pdev, epnum));
      break;
#ifndef MUCISBOARD_USB_MSC_DISABLE
    case MSC_OUT_EP:
      return (USBD_MSC_DataOut(pdev, epnum));
      break;
#endif
  }

  return USBD_FAIL;
}

/**
* @brief  DeviceQualifierDescriptor 
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_Composite_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_Composite_DeviceQualifierDesc);
  return USBD_Composite_DeviceQualifierDesc;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_Composite_RegisterInterface(USBD_HandleTypeDef *pdev)
{

  USBD_AUDIO_RegisterInterface(pdev, &USBD_AUDIO_fops);
  USBD_Midi_RegisterInterface(pdev, &USBD_Midi_fops);
#ifndef MUCISBOARD_USB_MSC_DISABLE
  USBD_MSC_RegisterStorage(pdev, &USBD_DISK_fops);
#endif  
  return USBD_OK;
}
