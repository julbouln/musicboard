
#ifndef __USBD_MIDI_H
#define __USBD_MIDI_H

#ifdef __cplusplus
 extern "C" {
#endif

#include  "usbd_ioreq.h"

#define USB_MIDI_CONFIG_DESC_SIZ       0x65

#define   MIDI_INTERFACE_DESC_SIZE 0x09

#define MIDI_INTERFACE_DESCRIPTOR_TYPE 0x24

#define MIDI_STANDARD_ENDPOINT_DESC_SIZE 0x09

#define   MIDI_OUT_EP         0x02
#define   MIDI_IN_EP          0x82
#define   MIDI_DATA_IN_PACKET_SIZE  0x40
#define   MIDI_DATA_OUT_PACKET_SIZE 0x40

#define MIDI_BUF_SIZE 64


extern USBD_ClassTypeDef  USBD_Midi_ClassDriver;

typedef struct _USBD_Midi_Itf
{
  int8_t (* Receive)       (uint8_t *, uint32_t);  

}USBD_Midi_ItfTypeDef;


typedef struct
{  
  uint8_t rxBuffer[MIDI_BUF_SIZE];
  uint32_t rxLen;
}
USBD_Midi_HandleTypeDef; 


uint8_t  USBD_Midi_RegisterInterface  (USBD_HandleTypeDef   *pdev, 
                                      USBD_Midi_ItfTypeDef *fops);

#ifdef __cplusplus
}
#endif

#endif  /* __USBD_MIDI_H */
