/**
  ******************************************************************************
  * @file    wm8731.c
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    24-June-2015
  * @brief   This file provides the WM8994 Audio Codec driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "wm8731.h"
#include "musicboard.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup wm8731
  * @brief     This file provides a set of functions needed to drive the
  *            WM8994 audio codec.
  * @{
  */

/** @defgroup WM8994_Private_Types
  * @{
  */

/**
  * @}
  */

/** @defgroup WM8994_Private_Defines
  * @{
  */
/* Uncomment this line to enable verifying data sent to codec after each write
   operation (for debug purpose) */
#if !defined (VERIFY_WRITTENDATA)
/* #define VERIFY_WRITTENDATA */
#endif /* VERIFY_WRITTENDATA */
/**
  * @}
  */

/** @defgroup WM8994_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup WM8994_Private_Variables
  * @{
  */

/* Audio codec driver structure initialization */
AUDIO_DrvTypeDef wm8731_drv =
{
  wm8731_Init,
  wm8731_DeInit,
  wm8731_ReadID,

  wm8731_Play,
  wm8731_Pause,
  wm8731_Resume,
  wm8731_Stop,

  wm8731_SetFrequency,
  wm8731_SetVolume,
  wm8731_SetMute,
  wm8731_SetOutputMode,

  wm8731_Reset
};

static uint32_t outputEnabled = 0;
static uint32_t inputEnabled = 0;
/**
  * @}
  */

/** @defgroup WM8994_Function_Prototypes
  * @{
  */
static uint8_t CODEC_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value);
/**
  * @}
  */

/** @defgroup WM8994_Private_Functions
  * @{
  */

static void wm8731_SetVolumedB(uint16_t DeviceAddr, int voldB) {
  // -73 <= voldB <= 6
  const unsigned volume = 121 + voldB;
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x02, 0x100 | (volume & 0x7f)); // left line out volume
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x03, 0x100 | (volume & 0x7f)); // right line out volume
}

/**
  * @brief Initializes the audio codec and the control interface.
  * @param DeviceAddr: Device address on communication Bus.
  * @param OutputInputDevice: can be OUTPUT_DEVICE_SPEAKER, OUTPUT_DEVICE_HEADPHONE,
  *  OUTPUT_DEVICE_BOTH, OUTPUT_DEVICE_AUTO, INPUT_DEVICE_DIGITAL_MICROPHONE_1,
  *  INPUT_DEVICE_DIGITAL_MICROPHONE_2, INPUT_DEVICE_INPUT_LINE_1 or INPUT_DEVICE_INPUT_LINE_2.
  * @param Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param AudioFreq: Audio Frequency
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_Init(uint16_t DeviceAddr, uint16_t OutputInputDevice, uint8_t Volume, uint32_t AudioFreq)
{
  uint32_t counter = 0;

  /* Initialize the Control interface of the Audio Codec */
  AUDIO_IO_Init();

  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x0f, 0b000000000); // Reset!

  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x00, 0x100 | (0x17 & 0x1f)); // Left line in, unmute
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x01, 0x100 | (0x17 & 0x1f)); // right line in, unmute

  wm8731_SetVolumedB(AUDIO_I2C_ADDRESS, -10);

  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x04, 0b000010010); // Analog path - select DAC, no bypass
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x05, 0b000000000); // Digital path - disable soft mute
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x06, 0b000000000); // Power down control - enable everything
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x07, 0b000000010); // Interface format - 16-bit I2S
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x09, 0b000000001); // Active control - engage!

  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x00, 0x100 | (0x17 & 0x1f)); // Left line in, unmute
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x01, 0x100 | (0x17 & 0x1f)); // right line in, unmute

  wm8731_SetVolumedB(AUDIO_I2C_ADDRESS, -10);

  /* Return communication control value */
  return counter;
}

/**
  * @brief  Deinitializes the audio codec.
  * @param  None
  * @retval  None
  */
void wm8731_DeInit(void)
{
  /* Deinitialize Audio Codec interface */
  AUDIO_IO_DeInit();
}

/**
  * @brief  Get the WM8994 ID.
  * @param DeviceAddr: Device address on communication Bus.
  * @retval The WM8994 ID
  */
uint32_t wm8731_ReadID(uint16_t DeviceAddr)
{
  /* Initialize the Control interface of the Audio Codec */
  AUDIO_IO_Init();

  return WM8731_ID;
}

/**
  * @brief Start the audio Codec play feature.
  * @note For this codec no Play options are required.
  * @param DeviceAddr: Device address on communication Bus.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_Play(uint16_t DeviceAddr, uint16_t* pBuffer, uint16_t Size)
{
  uint32_t counter = 0;

  /* Resumes the audio file playing */
  /* Unmute the output first */
  counter += wm8731_SetMute(DeviceAddr, AUDIO_MUTE_OFF);

  return counter;
}

/**
  * @brief Pauses playing on the audio codec.
  * @param DeviceAddr: Device address on communication Bus.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_Pause(uint16_t DeviceAddr)
{
  uint32_t counter = 0;

  /* Pause the audio file playing */
  /* Mute the output first */
  counter += wm8731_SetMute(DeviceAddr, AUDIO_MUTE_ON);

  /* Put the Codec in Power save mode */

  return counter;
}

/**
  * @brief Resumes playing on the audio codec.
  * @param DeviceAddr: Device address on communication Bus.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_Resume(uint16_t DeviceAddr)
{
  uint32_t counter = 0;

  /* Resumes the audio file playing */
  /* Unmute the output first */
  counter += wm8731_SetMute(DeviceAddr, AUDIO_MUTE_OFF);

  return counter;
}

/**
  * @brief Stops audio Codec playing. It powers down the codec.
  * @param DeviceAddr: Device address on communication Bus.
  * @param CodecPdwnMode: selects the  power down mode.
  *          - CODEC_PDWN_SW: only mutes the audio codec. When resuming from this
  *                           mode the codec keeps the previous initialization
  *                           (no need to re-Initialize the codec registers).
  *          - CODEC_PDWN_HW: Physically power down the codec. When resuming from this
  *                           mode, the codec is set to default configuration
  *                           (user should re-Initialize the codec in order to
  *                            play again the audio stream).
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_Stop(uint16_t DeviceAddr, uint32_t CodecPdwnMode)
{
  uint32_t counter = 0;
  return counter;
}

uint32_t wm8731_SetVolume(uint16_t DeviceAddr, uint8_t Volume)
{
  uint32_t counter = 0;

  int voldB = (int)Volume/2 - 73;

  wm8731_SetVolumedB(DeviceAddr, voldB);
  return counter;
}

/**
  * @brief Enables or disables the mute feature on the audio codec.
  * @param DeviceAddr: Device address on communication Bus.
  * @param Cmd: AUDIO_MUTE_ON to enable the mute or AUDIO_MUTE_OFF to disable the
  *             mute mode.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_SetMute(uint16_t DeviceAddr, uint32_t Cmd)
{
  uint32_t counter = 0;
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x02, 0x100); // left line out volume
  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x03, 0x100); // right line out volume
  return counter;
}

/**
  * @brief Switch dynamically (while audio file is played) the output target
  *         (speaker or headphone).
  * @param DeviceAddr: Device address on communication Bus.
  * @param Output: specifies the audio output target: OUTPUT_DEVICE_SPEAKER,
  *         OUTPUT_DEVICE_HEADPHONE, OUTPUT_DEVICE_BOTH or OUTPUT_DEVICE_AUTO
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_SetOutputMode(uint16_t DeviceAddr, uint8_t Output)
{
  uint32_t counter = 0;
  return counter;
}

/**
  * @brief Sets new frequency.
  * @param DeviceAddr: Device address on communication Bus.
  * @param AudioFreq: Audio frequency used to play the audio stream.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_SetFrequency(uint16_t DeviceAddr, uint32_t AudioFreq)
{
  uint32_t counter = 0;

  return counter;
}

/**
  * @brief Resets wm8731 registers.
  * @param DeviceAddr: Device address on communication Bus.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8731_Reset(uint16_t DeviceAddr)
{
  uint32_t counter = 0;

  outputEnabled = 0;
  inputEnabled = 0;

  CODEC_IO_Write(AUDIO_I2C_ADDRESS, 0x0f, 0b00000000); // Reset!

  return counter;
}

/**
  * @brief  Writes/Read a single data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  Value: Data to be written
  * @retval None
  */
static uint8_t CODEC_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value)
{
  uint32_t result = 0;

  AUDIO_IO_Write(Addr, Reg, Value);

#ifdef VERIFY_WRITTENDATA
  /* Verify that the data has been correctly written */
  result = (AUDIO_IO_Read(Addr, Reg) == Value) ? 0 : 1;
#endif /* VERIFY_WRITTENDATA */

  return result;
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

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
