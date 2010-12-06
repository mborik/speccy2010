/******************** (C) COPYRIGHT 2006 STMicroelectronics ********************
* File Name          : 7xx_flash.h
* Author             : MCD Application Team
* Date First Issued  : 10/01/2006 : V1.0
* Description        : This file contains all the functions prototypes for the
*                      Flash software library.
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
#ifndef __7XX_FLASH_H
#define __7XX_FLASH_H

/* Includes ------------------------------------------------------------------*/
#include "flash_map.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Flash bank 0 sectors */
#define FLASH_BANK0_SECTOR0    0x00000001  /* Bank 0 sector 0 */
#define FLASH_BANK0_SECTOR1    0x00000002  /* Bank 0 sector 1 */
#define FLASH_BANK0_SECTOR2    0x00000004  /* Bank 0 sector 2 */
#define FLASH_BANK0_SECTOR3    0x00000008  /* Bank 0 sector 3 */
#define FLASH_BANK0_SECTOR4    0x00000010  /* Bank 0 sector 4 */
#define FLASH_BANK0_SECTOR5    0x00000020  /* Bank 0 sector 5 */
#define FLASH_BANK0_SECTOR6    0x00000040  /* Bank 0 sector 6 */
#define FLASH_BANK0_SECTOR7    0x00000080  /* Bank 0 sector 7 */
/* Flash bank 0 module */
#define FLASH_BANK0_MODULE     0x000000FF  /* Bank 0 module   */

#ifndef STR73x
  /* Flash bank 1 sectors */
  #define FLASH_BANK1_SECTOR0    0x00010000  /* Bank 1 sector 0 */
  #define FLASH_BANK1_SECTOR1    0x00020000  /* Bank 1 sector 1 */
  /* Flash bank 1 module */
  #define FLASH_BANK1_MODULE     0x00030000  /* Bank 1 module   */
#endif

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void FLASH_DeInit(void);
void FLASH_WriteWord(u32 FLASH_Address, u32 FLASH_Data);
void FLASH_WriteDoubleWord(u32 FLASH_Address, u32 FLASH_Data_1, u32 FLASH_Data_2);
void FLASH_EraseSector(u32 FLASH_Sectors);
u32 FLASH_SuspendOperation(void);
void FLASH_ResumeOperation(u32 FLASH_LastOperation);
void FLASH_ITConfig(FunctionalState FLASH_NewState);
FlagStatus FLASH_GetITStatus(void);
void FLASH_ClearITPendingBit(void);
u16 FLASH_GetFlagStatus(void);
void FLASH_ClearFlag(void);
void FLASH_WaitForLastOperation(void);
void FLASH_WriteProtectionCmd(u32 FLASH_Sectors, FunctionalState FLASH_NewState);
FlagStatus FLASH_GetWriteProtectionStatus(u32 FLASH_Sectors);
u16 FLASH_GetPDSProtectionLevel(void);
u16 FLASH_GetPENProtectionLevel(void);
void FLASH_PermanentProtectionCmd(FunctionalState FLASH_NewState);
void FLASH_TemporaryProtectionDisable(void);

#endif  /* __7XX_FLASH_H */

/*******************(C) COPYRIGHT 2006 STMicroelectronics *****END OF FILE****/
