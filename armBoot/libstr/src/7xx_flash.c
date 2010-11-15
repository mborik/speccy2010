/******************** (C) COPYRIGHT 2006 STMicroelectronics ********************
* File Name          : 7xx_flash.c
* Author             : MCD Application Team
* Date First Issued  : 10/01/2006 : V1.0
* Description        : This file provides all the Flash software functions.
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

/* Includes ------------------------------------------------------------------*/
#include"7xx_flash.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Flash operation enable bit mask */
#define FLASH_SPR_MASK     0x01000000
#define FLASH_SER_MASK     0x08000000
#define FLASH_DWPG_MASK    0x10000000
#define FLASH_WPG_MASK     0x20000000
#define FLASH_SUSP_MASK    0x40000000
#define FLASH_WMS_MASK     0x80000000

/* Flash interrupt mask */
#define FLASH_INTM_MASK    0x00200000
#define FLASH_INTP_MASK    0x00100000

#ifdef STR73x
  /* STR73x Flash lock and busy bits */
  #define FLASH_FLAG_LOCKBSY     0x12
#else
  /* STR71x/STR75x Flash lock and busy bits */
  #define FLASH_FLAG_LOCKBSY     0x16
#endif

/* Flash protection mask */
#ifdef STR75x
  /* STR75x Readout protection */
  #define FLASH_PROTECTION_MASK  0x00000001
#else /* STR71x & STR73x */
  /* STR71x & STR73x Debug protection */
  #define FLASH_PROTECTION_MASK  0x00000002
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : FLASH_DeInit
* Description    : Deinitializes the Flash module registers to their default 
*                  reset values.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_DeInit(void)
{
  /* Reset the Flash control registers */
  FLASH->CR0 = 0x00000000;
  FLASH->CR1 = 0x00000000;

  /* Reset the Flash data registers */
  FLASH->DR0 = 0xFFFFFFFF;
  FLASH->DR1 = 0xFFFFFFFF;

  /* Reset the Flash address register */
  FLASH->AR = 0x00000000;

  /* Reset the Flash error register */
  FLASH->ER  = 0x00000000;
}

/*******************************************************************************
* Function Name  : FLASH_WriteWord
* Description    : Writes a Word to a Flash address destination.
* Input          : - FLASH_Address: the Address of the destination.
*                  - FLASH_Data: the data to be programmed.
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_WriteWord(u32 FLASH_Address, u32 FLASH_Data)
{
  /* Set the word programming bit */
  FLASH->CR0 |= FLASH_WPG_MASK;
  /* Load the destination address */
  FLASH->AR   = FLASH_Address;
  /* Load DATA to be programmed */
  FLASH->DR0  = FLASH_Data;
  /* Start the operation */
  FLASH->CR0 |= FLASH_WMS_MASK;
}

/*******************************************************************************
* Function Name  : FLASH_WriteDoubleWord
* Description    : Writes Double Word to the Flash.
* Input          : - FLASH_Address: the destination address.
*                  - FLASH_Data_1: the first word to be programmed.
*                  - FLASH_Data_2: the second word to be programmed.
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_WriteDoubleWord(u32 FLASH_Address, u32 FLASH_Data_1, u32 FLASH_Data_2)
{
  /* Set the double word programming bit */
  FLASH->CR0 |= FLASH_DWPG_MASK;
  /* Load the destination address */
  FLASH->AR   = FLASH_Address;
  /* Load FLASH_Data_1 in the DR0 register */
  FLASH->DR0  = FLASH_Data_1;
  /* Load FLASH_Data_2 in the DR1 register */
  FLASH->DR1  = FLASH_Data_2;
  /* Start the operation */
  FLASH->CR0 |= FLASH_WMS_MASK;
}

/*******************************************************************************
* Function Name  : FLASH_EraseSector
* Description    : Erases one or multiple sectors on a flash bank.
* Input          : FLASH_Sectors: the sectors to be erased.
*                  This parameter can be one of the following values:
*                      - FLASH_BANK0_SECTOR0: Bank 0 sector 0.
*                      - FLASH_BANK0_SECTOR1: Bank 0 sector 1.
*                      - FLASH_BANK0_SECTOR2: Bank 0 sector 2.
*                      - FLASH_BANK0_SECTOR3: Bank 0 sector 3.
*                      - FLASH_BANK0_SECTOR4: Bank 0 sector 4.
*                      - FLASH_BANK0_SECTOR5: Bank 0 sector 5.
*                      - FLASH_BANK0_SECTOR6: Bank 0 sector 6.
*                      - FLASH_BANK0_SECTOR7: Bank 0 sector 7.
*                      - FLASH_BANK0_MODULE:  Bank 0 module.
*                      - FLASH_BANK1_SECTOR0: Bank 1 sector 0. (N.A for STR73x)
*                      - FLASH_BANK1_SECTOR1: Bank 1 sector 1. (N.A for STR73x)
*                      - FLASH_BANK1_MODULE:  Bank 1 module.   (N.A for STR73x)
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_EraseSector(u32 FLASH_Sectors)
{
  /* Set the sector erase bit */
  FLASH->CR0 |= FLASH_SER_MASK;
  /* Select the sectors to be erased */
  FLASH->CR1 |= FLASH_Sectors;
  /* Start the operation */
  FLASH->CR0 |= FLASH_WMS_MASK;
}

/*******************************************************************************
* Function Name  : FLASH_SuspendOperation
* Description    : Suspends the current operation.
* Input          : None
* Output         : None
* Return         : Flash Control register content.
*******************************************************************************/
u32 FLASH_SuspendOperation(void)
{
  u32 FLASH_CONTROL_REGISTER = 0x0;
  /* Wait until register's unlock */
  while(FLASH->CR0 & 0x10);
  /* Save the control register content */
  FLASH_CONTROL_REGISTER = FLASH->CR0;
  /* Set the suspend Bit */
  FLASH->CR0 |= FLASH_SUSP_MASK;
  /* Wait until the flash controller acknowledges the suspend of the current
     operation */
  FLASH_WaitForLastOperation();
  /* Return the suspended operation */
  return FLASH_CONTROL_REGISTER;
}
/*******************************************************************************
* Function Name  : FLASH_ResumeOperation
* Description    : Resumes the suspended operation.
* Input          : FLASH_LastOperation: the Flash control register content
*                  returned by the FLASH_SuspendOperation function.
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_ResumeOperation(u32 FLASH_LastOperation)
{
  /* Clear the suspend Bit */
  FLASH->CR0 &= ~FLASH_SUSP_MASK;
  /* Resume the last operation */
  FLASH->CR0  = FLASH_LastOperation & ~FLASH_WMS_MASK;
  /* Start the operation */
  FLASH->CR0 |= FLASH_WMS_MASK;
}

/*******************************************************************************
* Function Name  : FLASH_ITConfig
* Description    : Enables or disables the end of write interrupt.
* Input          : FLASH_NewState: the new state of end of write interrupt.
*                  This parameter can be one of the following values: 
*                      - ENABLE:  Enable the end of write interrupt.
*                      - DISABLE: Disable the end of write interrupt.
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_ITConfig(FunctionalState FLASH_NewState)
{
  if(FLASH_NewState == ENABLE)
  { 
    /* Enable the end of write interrupt */
    FLASH->CR0  |= FLASH_INTM_MASK;
  }
  else
  {
    /* Disable the end of write interrupt */
    FLASH->CR0  &= ~FLASH_INTM_MASK;
  }
}

/*******************************************************************************
* Function Name  : FLASH_GetITStatus
* Description    : Checks whether the specified Flash interrupt pending bit
*                  is set or not
* Input          : None
* Output         : None
* Return         : The new state of the interrupt (SET or RESET)
*******************************************************************************/
FlagStatus FLASH_GetITStatus(void)
{
  if((FLASH->CR0 & FLASH_INTP_MASK)!= RESET)
  {
    return SET;
  }
  else
  {
    return RESET;
  }
}

/*******************************************************************************
* Function Name  : FLASH_ClearITPendingBit
* Description    : Clears the end of write interrupt pending flag.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_ClearITPendingBit(void)
{
  FLASH->CR0 &= ~FLASH_INTP_MASK;
}

/*******************************************************************************
* Function Name  : FLASH_GetFlagStatus
* Description    : Checks the Flash flags.
* Input          : None
* Output         : None
* Return         : The Flash flag errors. 
*                  The description of the Flash flag errors bits :  
*                      - Bit 0: Write error flag.
*                      - Bit 1: Erase error flag.
*                      - Bit 2: Program error flag.
*                      - Bit 3: 1 over 0 error flag.
*                      - Bit 6: Sequence error flag.
*                      - Bit 7: Resume error flag.
*                      - Bit 8: Write protection error flag.
*******************************************************************************/
u16 FLASH_GetFlagStatus(void)
{
  return FLASH->ER;
}

/*******************************************************************************
* Function Name  : FLASH_ClearFlag
* Description    : Clears the Flash errors flags.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_ClearFlag(void)
{
  /* Clear the correspondent flag */
  FLASH->ER = 0x0;
}

/*******************************************************************************
* Function Name  : FLASH_WaitForLastOperation
* Description    : Waits for the end of the last operation
* Input          : None
* Output         : None
* Return         : None
********************************************************************************
* Important Note :  THIS FUNCTION SHOULD BE EXECUTED FROM RAM
*******************************************************************************/
void FLASH_WaitForLastOperation(void)
{
  /* Wait until the Flash controller acknowledges the end of the last
    operation resetting the BSYAs and LOCK bits in the CR0 register */
  while((FLASH->CR0 & FLASH_FLAG_LOCKBSY) != RESET);
}

/*==============================================================================
                           PROTECTION FUNCTIONS
==============================================================================*/


/*******************************************************************************
* Function Name  : FLASH_WriteProtectionCmd
* Description    : Enables/disables the write protection for the needed sectors.
* Input          : - FLASH_Sectors: the sectors to be protected or unprotected.
*                  This parameter can be one of the following values:
*                      - FLASH_BANK0_SECTOR0: Bank 0 sector 0.
*                      - FLASH_BANK0_SECTOR1: Bank 0 sector 1.
*                      - FLASH_BANK0_SECTOR2: Bank 0 sector 2.
*                      - FLASH_BANK0_SECTOR3: Bank 0 sector 3.
*                      - FLASH_BANK0_SECTOR4: Bank 0 sector 4.
*                      - FLASH_BANK0_SECTOR5: Bank 0 sector 5.
*                      - FLASH_BANK0_SECTOR6: Bank 0 sector 6.
*                      - FLASH_BANK0_SECTOR7: Bank 0 sector 7.
*                      - FLASH_BANK0_MODULE:  Bank 0 module.
*                      - FLASH_BANK1_SECTOR0: Bank 1 sector 0. (N.A for STR73x)
*                      - FLASH_BANK1_SECTOR1: Bank 1 sector 1. (N.A for STR73x)
*                      - FLASH_BANK1_MODULE:  Bank 1 module.   (N.A for STR73x)
*                  - FLASH_NewState: Enable or disable the write protection.
*                  This parameter can be one of the following values:
*                      - ENABLE:  Enable the end of write interrupt.
*                      - DISABLE: Disable the end of write interrupt.
* Output         : None
* Return         : None
********************************************************************************
* Important Note : 
*   - The write protection can be disabled only on a temporary way.  
*   - For the STR73x family this function can work only when the STR73x is on 
*     SystemMemory boot mode. 
*******************************************************************************/
void FLASH_WriteProtectionCmd(u32 FLASH_Sectors, FunctionalState FLASH_NewState)
{
  if (FLASH_NewState == ENABLE)
  {
    /* Set the set protection bit */
    FLASH->CR0 |= FLASH_SPR_MASK;
    /* Set the write protection register address */
    FLASH->AR  = FLASH_NVWPAR_ADDRESS;
    /* Data to be programmed to the protection register */
    FLASH->DR0  = (*(u32*)FLASH_NVWPAR_ADDRESS) & (~FLASH_Sectors);
    /* Start the sequence */
    FLASH->CR0 |= FLASH_WMS_MASK;
  }
  else  /* DISABLE */
  {
    /* Set the set protection bit */
    FLASH->CR0 |= FLASH_SPR_MASK;
    /* Set the write protection register address */
    FLASH->AR  = FLASH_NVWPAR_ADDRESS;
    /* Data to be programmed to the protection register */
    FLASH->DR0  = (*(u32*)FLASH_NVWPAR_ADDRESS) | FLASH_Sectors;
    /* Start the sequence */
    FLASH->CR0 |= FLASH_WMS_MASK;
  }
}

/*******************************************************************************
* Function Name  : FLASH_GetWriteProtectionStatus
* Description    : Get the write protection status for the needed sectors. Only
*                  the non volatile part is returned.
* Input          : - FLASH_Sectors: the needed sectors.
*                  This parameter can be one of the following values:
*                      - FLASH_BANK0_SECTOR0: Bank 0 sector 0.
*                      - FLASH_BANK0_SECTOR1: Bank 0 sector 1.
*                      - FLASH_BANK0_SECTOR2: Bank 0 sector 2.
*                      - FLASH_BANK0_SECTOR3: Bank 0 sector 3.
*                      - FLASH_BANK0_SECTOR4: Bank 0 sector 4.
*                      - FLASH_BANK0_SECTOR5: Bank 0 sector 5.
*                      - FLASH_BANK0_SECTOR6: Bank 0 sector 6.
*                      - FLASH_BANK0_SECTOR7: Bank 0 sector 7.
*                      - FLASH_BANK0_MODULE:  Bank 0 module.
*                      - FLASH_BANK1_SECTOR0: Bank 1 sector 0. (N.A for STR73x)
*                      - FLASH_BANK1_SECTOR1: Bank 1 sector 1. (N.A for STR73x)
*                      - FLASH_BANK1_MODULE:  Bank 1 module.   (N.A for STR73x)
*                  - FLASH_NewState: Enable or disable the write protection.
*                  This parameter can be one of the following values:
*                      - ENABLE:  Enable the end of write interrupt.
*                      - DISABLE: Disable the end of write interrupt.
* Output         : None
* Return         : The status of the write protection (SET or RESET)
********************************************************************************
* Important Note : 
*   - For the STR73x family this function can work only when the STR73x is on 
*     SystemMemory boot mode. 
*******************************************************************************/
FlagStatus FLASH_GetWriteProtectionStatus(u32 FLASH_Sectors)
{
  if ((*(u32*)FLASH_NVWPAR_ADDRESS) & FLASH_Sectors)
  {
    return RESET;
  }
  else
  {
    return SET;
  } 
}

/*******************************************************************************
* Function Name  : FLASH_GetPENProtectionLevel
* Description    : Gets the protection enable level.
* Input          : None
* Output         : None
* Return         : The number of time the debug protection was enabled. 
*                  It's a value from 1 to 16.
********************************************************************************
* Important Note : 
*   - For the STR73x family this function can work only when the STR73x is on 
*     SystemMemory boot mode. 
*******************************************************************************/
u16 FLASH_GetPENProtectionLevel(void)
{
  u16 TmpBitIndex = 0;
  u16 ProtectionRegs = 0;

  ProtectionRegs = ~((*(u32 *)FLASH_NVAPR1_ADDRESS)>>16);

  while (((ProtectionRegs) != 0) && (TmpBitIndex < 16))
  {
    ProtectionRegs  = ProtectionRegs >>  1 ;
    TmpBitIndex++;
  }

  /* Return the number of times the FLASH is Debug protected */
  return TmpBitIndex;
}

/*******************************************************************************
* Function Name  : FLASH_GetPDSProtectionLevel
* Description    : Gets the protection disable level.
* Input          : None
* Output         : None
* Return         : The number of time the debug protection was disabled. 
*                  It's a value from 1 to 16.
********************************************************************************
* Important Note : 
*   - For the STR73x family this function can work only when the STR73x is on 
*     SystemMemory boot mode. 
*******************************************************************************/
u16 FLASH_GetPDSProtectionLevel(void)
{
  u16 TmpBitIndex = 0;
  u16 ProtectionRegs = 0;

  ProtectionRegs = ~(*(u32 *)FLASH_NVAPR1_ADDRESS);

  while (((ProtectionRegs) != 0) && (TmpBitIndex < 16))
  {
    ProtectionRegs  = ProtectionRegs >>  1 ;
    TmpBitIndex++;
  }

  /* Return the number of times the FLASH is Debug protected */
  return TmpBitIndex;
}

/*******************************************************************************
* Function Name  : FLASH_PermanentProtectionCmd
* Description    : Enable or disable the Debug/Readout protection in a permanent
*                  way.
* Input          : - FLASH_NewState: Enable or disable the Debug/Readout 
*                  protection. 
*                  This parameter can be one of the following values:
*                      - ENABLE:  Enable the Debug/Readout protection.
*                      - DISABLE: Disable the Debug/Readout protection.
* Output         : None
* Return         : None
********************************************************************************
* Important Note :  
*   - For the STR73x family this function can work only when the STR73x is on 
*     SystemMemory boot mode. 
*******************************************************************************/
void FLASH_PermanentProtectionCmd(FunctionalState FLASH_NewState)
{
  u16 PEN_ProtectionLevel = FLASH_GetPENProtectionLevel();
  u16 PDS_ProtectionLevel = FLASH_GetPDSProtectionLevel();

  if (FLASH_NewState == ENABLE)
  {
    /*  If it is the first protection, reset the DEBUG bit */
    if(!PDS_ProtectionLevel)
    {
      /* Set the Set Protection bit */
      FLASH->CR0 |= FLASH_SPR_MASK;
      /* Set the Debug Protection register address */
      FLASH->AR  = FLASH_NVAPR0_ADDRESS;
      /* Data to be programmed to the protection register */
      FLASH->DR0 = ~FLASH_PROTECTION_MASK;
      /* Start the operation */
      FLASH->CR0 |= FLASH_WMS_MASK;
    }
    else /* The protection now can be enabled using the PEN bits */
    {
      /* Set the Set Protection bit */
      FLASH->CR0 |= FLASH_SPR_MASK;
      /* Set the Debug Protection register address */
      FLASH->AR  = FLASH_NVAPR1_ADDRESS;
      /* Data to be programmed to the protection register */
      FLASH->DR0 =((~(1 << (16 + PEN_ProtectionLevel))) & (*(u32*)FLASH_NVAPR1_ADDRESS));
      /* Start the operation */
      FLASH->CR0 |= FLASH_WMS_MASK;
    }
  }
  else /* Disable the Debug/ReadOut protection */ 
  {
    /* Set the Set Protection bit */
    FLASH->CR0 |= FLASH_SPR_MASK;
    /* Set the Debug Protection register address */
    FLASH->AR  = FLASH_NVAPR1_ADDRESS;
    /* Data to be programmed to the protection register */
    FLASH->DR0 =(~(1<<(PDS_ProtectionLevel)) & (*(u32*)FLASH_NVAPR1_ADDRESS));
    /* Start the operation */
    FLASH->CR0 |= FLASH_WMS_MASK;
  }
}

/*******************************************************************************
* Function Name  : FLASH_TemporaryProtectionDisable
* Description    : Disable the Debug/Readout protection in a temporary way.
* Input          : None
* Output         : None
* Return         : None
********************************************************************************
* Important Note : 
*   - For the STR73x family this function can work only when the STR73x is on 
*     SystemMemory boot mode. 
*******************************************************************************/
void FLASH_TemporaryProtectionDisable(void)
{
  /* Set the Set Protection bit */
  FLASH->CR0 |= FLASH_SPR_MASK;
  /* Set the Protection register address */
  FLASH->AR  = FLASH_NVAPR0_ADDRESS;
  /* Data to be programmed to the protection register */
  FLASH->DR0 = (*(u32*)FLASH_NVAPR0_ADDRESS) | FLASH_PROTECTION_MASK;
  /* Start the operation */
  FLASH->CR0 |= FLASH_WMS_MASK;
}
/*******************(C)COPYRIGHT 2006 STMicroelectronics *****END OF FILE******/
