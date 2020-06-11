
#ifndef __USBD_COMPOSITE_H
#define __USBD_COMPOSITE_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"
#include "usbd_audio.h"
#include "usbd_midi.h"
#include "usbd_msc.h"

#include "usbd_storage.h"
#include "usbd_desc.h"

#ifndef MUCISBOARD_USB_MSC_DISABLE
#define USB_COMPOSITE_CONFIG_DESC_SIZ       USB_AUDIO_CONFIG_DESC_SIZ + USB_MIDI_CONFIG_DESC_SIZ - 9 + USB_MSC_CONFIG_DESC_SIZ - 9
#else
#define USB_COMPOSITE_CONFIG_DESC_SIZ       USB_AUDIO_CONFIG_DESC_SIZ + USB_MIDI_CONFIG_DESC_SIZ - 9
#endif

#define AUDIO_INTERFACE0         0x00
#define AUDIO_INTERFACE1         0x01
#define MIDI_INTERFACE0          0x02
#define MIDI_INTERFACE1          0x03
#define MSC_INTERFACE			 0x04

extern USBD_ClassTypeDef  USBD_Composite_ClassDriver;

typedef struct _USBD_Composite_Itf
{
  USBD_AUDIO_ItfTypeDef *audioItf;
  USBD_Midi_ItfTypeDef *midiItf;
} USBD_Composite_ItfTypeDef;


uint8_t  USBD_Composite_RegisterInterface  (USBD_HandleTypeDef   *pdev);

#ifdef __cplusplus
}
#endif

#endif  /* __USB_COMPOSITE_H */
