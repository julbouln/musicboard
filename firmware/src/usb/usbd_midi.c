/* simple USB Midi IN device driver */
/* inspired from mbed implementation */
/* TODO: sysex support */

#include "usbd_midi.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"

uint8_t  USBD_Midi_Init (USBD_HandleTypeDef *pdev,
                         uint8_t cfgidx);
uint8_t  USBD_Midi_DeInit (USBD_HandleTypeDef *pdev,
                           uint8_t cfgidx);
uint8_t  USBD_Midi_Setup (USBD_HandleTypeDef *pdev,
                          USBD_SetupReqTypedef *req);
uint8_t  *USBD_Midi_GetCfgDesc (uint16_t *length);
uint8_t  *USBD_Midi_GetDeviceQualifierDesc (uint16_t *length);
uint8_t  USBD_Midi_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t  USBD_Midi_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t  USBD_Midi_EP0_RxReady (USBD_HandleTypeDef *pdev);
uint8_t  USBD_Midi_EP0_TxReady (USBD_HandleTypeDef *pdev);
uint8_t  USBD_Midi_SOF (USBD_HandleTypeDef *pdev);
uint8_t  USBD_Midi_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t  USBD_Midi_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);


USBD_ClassTypeDef  USBD_Midi_ClassDriver =
{
  USBD_Midi_Init,
  USBD_Midi_DeInit,
  USBD_Midi_Setup,
  USBD_Midi_EP0_TxReady,
  USBD_Midi_EP0_RxReady,
  USBD_Midi_DataIn,
  USBD_Midi_DataOut,
  USBD_Midi_SOF,
  USBD_Midi_IsoINIncomplete,
  USBD_Midi_IsoOutIncomplete,
  USBD_Midi_GetCfgDesc,
  USBD_Midi_GetCfgDesc,
  USBD_Midi_GetCfgDesc,
  USBD_Midi_GetDeviceQualifierDesc,
};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment=4
#endif
uint8_t USBD_Midi_CfgDesc[USB_MIDI_CONFIG_DESC_SIZ] =
{
  // configuration descriptor
  0x09,
  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,
  LOBYTE(USB_MIDI_CONFIG_DESC_SIZ),
  HIBYTE(USB_MIDI_CONFIG_DESC_SIZ),
  0x02, /* bNumInterfaces */
  0x01, /* bConfigurationValue */
  0x00, /* iConfiguration */
  0xc0, /* bmAttributes  BUS Powred*/
  0x50, /* bMaxPower = 100 mA*/

  // The Audio Interface Collection
  // Standard AC Interface Descriptor
  MIDI_INTERFACE_DESC_SIZE,
  USB_DESC_TYPE_INTERFACE,
  0x00,
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
  0x04,
  0x01,
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
};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment=4
#endif
/* USB Standard Device Descriptor */
uint8_t USBD_Midi_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] =
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

/** @defgroup USBD_Midi_Private_Functions
  * @{
  */

/**
  * @brief  USBD_Midi_Init
  *         Initialize the Midi interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_Midi_Init (USBD_HandleTypeDef *pdev,
                         uint8_t cfgidx)
{

  pdev->pClassData[1] = USBD_malloc(sizeof (USBD_Midi_HandleTypeDef));

  if (pdev->pClassData[1] == NULL)
  {
    return USBD_FAIL;
  }
  else
  {
    USBD_Midi_HandleTypeDef *hmidi = (USBD_Midi_HandleTypeDef*) pdev->pClassData[1];

    /* Open the in EP */
    USBD_LL_OpenEP(pdev,
                   MIDI_IN_EP,
                   USBD_EP_TYPE_BULK,
                   MIDI_DATA_IN_PACKET_SIZE
                  );

    /* Open the out EP */
    USBD_LL_OpenEP(pdev,
                   MIDI_OUT_EP,
                   USBD_EP_TYPE_BULK,
                   MIDI_DATA_OUT_PACKET_SIZE
                  );

    /* Prepare Out endpoint to receive next packet */
    USBD_LL_PrepareReceive(pdev,
                           MIDI_OUT_EP,
                           hmidi->rxBuffer,
                           MIDI_DATA_OUT_PACKET_SIZE);

    return USBD_OK;
  }
}

/**
  * @brief  USBD_Midi_Init
  *         DeInitialize the Midi layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_Midi_DeInit (USBD_HandleTypeDef *pdev,
                           uint8_t cfgidx)
{
  USBD_LL_CloseEP(pdev,
                  MIDI_IN_EP);
  USBD_LL_CloseEP(pdev,
                  MIDI_OUT_EP);
  return USBD_OK;
}

/**
  * @brief  USBD_Midi_Setup
  *         Handle the Midi specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
uint8_t  USBD_Midi_Setup (USBD_HandleTypeDef *pdev,
                          USBD_SetupReqTypedef *req)
{

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {

    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL;
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {

    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL;
    }
  }
  return USBD_OK;
}


/**
  * @brief  USBD_Midi_GetCfgDesc
  *         return configuration descriptor
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t  *USBD_Midi_GetCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_Midi_CfgDesc);
  return USBD_Midi_CfgDesc;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_Midi_DeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (USBD_Midi_DeviceQualifierDesc);
  return USBD_Midi_DeviceQualifierDesc;
}


/**
  * @brief  USBD_Midi_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_Midi_DataIn (USBD_HandleTypeDef *pdev,
                           uint8_t epnum)
{
  /* TODO */
  return USBD_OK;
}

/**
  * @brief  USBD_Midi_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_Midi_EP0_RxReady (USBD_HandleTypeDef *pdev)
{

  return USBD_OK;
}
/**
  * @brief  USBD_Midi_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_Midi_EP0_TxReady (USBD_HandleTypeDef *pdev)
{

  return USBD_OK;
}
/**
  * @brief  USBD_Midi_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_Midi_SOF (USBD_HandleTypeDef *pdev)
{

  return USBD_OK;
}
/**
  * @brief  USBD_Midi_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_Midi_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  return USBD_OK;
}
/**
  * @brief  USBD_Midi_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_Midi_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  return USBD_OK;
}
/**
  * @brief  USBD_Midi_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */

uint32_t data_pos = 0;
uint8_t data[1024];

uint8_t  USBD_Midi_DataOut (USBD_HandleTypeDef *pdev,
                            uint8_t epnum)
{
  USBD_Midi_HandleTypeDef *hmidi = (USBD_Midi_HandleTypeDef*) pdev->pClassData[1];

  hmidi->rxLen = USBD_LL_GetRxDataSize (pdev, epnum);

  uint8_t *buf = hmidi->rxBuffer;

  uint32_t raw_pos = 0;

  while (raw_pos < hmidi->rxLen) {
    uint8_t data_read;
    uint8_t data_end = 1;
    switch (buf[raw_pos]) {
    case 0x2:
      // Two-bytes System Common Message - undefined in USBMidi 1.0
      data_read = 2;
      break;
    case 0x4:
      // SysEx start or continue
      data_end = 0;
      data_read = 3;
      break;
    case 0x5:
      // Single-byte System Common Message or SysEx end with one byte
      data_read = 1;
      break;
    case 0x6:
      // SysEx end with two bytes
      data_read = 2;
      break;
    case 0xC:
      // Program change
      data_read = 2;
      break;
    case 0xD:
      // Channel pressure
      data_read = 2;
      break;
    case 0xF:
      // Single byte
      data_read = 1;
      break;
    default:
      // Others three-bytes messages
      data_read = 3;
      break;
    }

    for (uint8_t j = 1; j < data_read + 1; j++) {
      data[data_pos] = buf[raw_pos + j];
      data_pos++;
    }
    raw_pos += 4;

    if (data_end) {
      ((USBD_Midi_ItfTypeDef *)pdev->pUserData[1])->Receive(data, data_pos);
      data_pos = 0;
    }
  }

  USBD_LL_PrepareReceive(pdev,
                         MIDI_OUT_EP,
                         hmidi->rxBuffer,
                         MIDI_DATA_OUT_PACKET_SIZE);

  return USBD_OK;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_Midi_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_Midi_DeviceQualifierDesc);
  return USBD_Midi_DeviceQualifierDesc;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_Midi_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                       USBD_Midi_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if (fops != NULL)
  {
    pdev->pUserData[1] = fops;
    ret = USBD_OK;
  }

  return ret;
}
