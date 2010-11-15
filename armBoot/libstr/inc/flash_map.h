/******************** (C) COPYRIGHT 2006 STMicroelectronics ********************
* File Name          : flash_map.h
* Author             : MCD Application Team
* Date First Issued  : 10/01/2006 : V1.0
* Description        : Flash registers definition and memory mapping.
********************************************************************************
* History:
* 10/01/2006 : V1.0
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __FLASH_MAP_H
#define __FLASH_MAP_H

/* Includes ------------------------------------------------------------------*/
#define STR75x

#ifdef STR71x 
  #include "71x_type.h"
#endif

#ifdef STR73x 
  #include "73x_type.h"
#endif

#ifdef STR75x 
  #include "75x_type.h"
#endif

/* Exported types ------------------------------------------------------------*/
/******************************************************************************/
/*                         FLASH registers structure                          */
/******************************************************************************/
typedef struct
{
  vu32 CR0;
  vu32 CR1;
  vu32 DR0;
  vu32 DR1;
  vu32 AR;
  vu32 ER;
} FLASH_TypeDef;

/* Exported constants --------------------------------------------------------*/
/******************************************************************************/
/*                               Flash addresses                              */
/******************************************************************************/

#ifdef STR71x
  #define FLASH_BASE_ADDRESS      0x40100000
#endif

#ifdef STR73x
  #define FLASH_BASE_ADDRESS      0x80100000
#endif

#ifdef STR75x
  #define FLASH_BASE_ADDRESS      0x20100000
#endif

/* Flash protection registers addresses */
#define FLASH_NVWPAR_ADDRESS   (FLASH_BASE_ADDRESS + 0xDFB0)
#define FLASH_NVAPR0_ADDRESS   (FLASH_BASE_ADDRESS + 0xDFB8)
#define FLASH_NVAPR1_ADDRESS   (FLASH_BASE_ADDRESS + 0xDFBC)

#define FLASH           ((FLASH_TypeDef *)FLASH_BASE_ADDRESS)

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
  
#endif /* __FLASH_MAP_H */

/******************* (C) COPYRIGHT 2006 STMicroelectronics *****END OF FILE****/

