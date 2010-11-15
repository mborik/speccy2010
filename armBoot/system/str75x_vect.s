.global UndefinedHandler
.global SWIHandler
.global PrefetchAbortHandler
.global DataAbortHandler
.global IRQHandler
.global FIQHandler
.global WAKUPIRQHandler
.global TIM2_OC2IRQHandler
.global TIM2_OC1IRQHandler
.global TIM2_IC12IRQHandler
.global TIM2_UPIRQHandler
.global TIM1_OC2IRQHandler
.global TIM1_OC1IRQHandler
.global TIM1_IC12IRQHandler
.global TIM1_UPIRQHandler
.global TIM0_OC2IRQHandler
.global TIM0_OC1IRQHandler
.global TIM0_IC12IRQHandler
.global TIM0_UPIRQHandler
.global PWM_OC123IRQHandler
.global PWM_EMIRQHandler
.global PWM_UPIRQHandler
.global I2CIRQHandler
.global SSP1IRQHandler
.global SSP0IRQHandler
.global UART2IRQHandler
.global UART1IRQHandler
.global UART0IRQHandler
.global CANIRQHandler
.global USB_LPIRQHandler
.global USB_HPIRQHandler
.global ADCIRQHandler
.global DMAIRQHandler
.global EXTITIRQHandler
.global MRCCIRQHandler
.global FLASHSMIIRQHandler
.global RTCIRQHandler
.global TBIRQHandler


;/* Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs */
.set  Mode_USR, 0x10            ;/* User Mode */
.set  Mode_FIQ, 0x11            ;/* FIQ Mode */
.set  Mode_IRQ, 0x12            ;/* IRQ Mode */
.set  Mode_SVC, 0x13            ;/* Supervisor Mode */
.set  Mode_ABT, 0x17            ;/* Abort Mode */
.set  Mode_UNDEF, 0x1B          ;/* Undefined Mode */
.set  Mode_SYS, 0x1F            ;/* System Mode */

.equ  I_Bit, 0x80               ;/* when I bit is set, IRQ is disabled */
.equ  F_Bit, 0x40               ;/* when F bit is set, FIQ is disabled */

/*; --- EIC Registers offsets*/
EIC_base_addr       =    0xFFFFF800         /*; EIC base address*/
ICR_off_addr        =    0x00               /*; Interrupt Control register offset*/
CICR_off_addr       =    0x04               /*; Current Interrupt Channel Register*/
CIPR_off_addr       =    0x08               /*; Current Interrupt Priority Register offset*/
IVR_off_addr        =    0x18               /*; Interrupt Vector Register offset*/
FIR_off_addr        =    0x1C               /*; Fast Interrupt Register offset*/
IER_off_addr        =    0x20               /*; Interrupt Enable Register offset*/
IPR_off_addr        =    0x40               /*; Interrupt Pending Bit Register offset*/
SIR0_off_addr       =    0x60               /*; Source Interrupt Register 0*/

/*;*******************************************************************************
;                         Exception Handlers
;********************************************************************************/

.section .text

/*;*******************************************************************************
;* Macro Name     : SaveContext
;* Description    : This macro used to save the context before entering
;                   an exception handler.
;* Input          : The range of registers to store.
;* Output         : none
;********************************************************************************/

       .macro SaveContext $r0,$r12
        STMFD  sp!,{r0-r12,lr} /*; Save The workspace plus the current return*/
                               /*; address lr_ mode into the stack.*/
        MRS    r1,spsr         /*; Save the spsr_mode into r1.*/
        STMFD  sp!,{r1}        /*; Save spsr.*/
        .endm

/*;*******************************************************************************
;* Macro Name     : RestoreContext
;* Description    : This macro used to restore the context to return from
;                   an exception handler and continue the program execution.
;* Input          : The range of registers to restore.
;* Output         : none
;********************************************************************************/

        .macro RestoreContext $r0,$r12
        LDMFD   sp!,{r1}        /*; Restore the saved spsr_mode into r1.*/
        MSR     spsr_cxsf,r1    /*; Restore spsr_mode.*/
        LDMFD   sp!,{r0-r12,pc}^/*; Return to the instruction following...*/
                                /*; ...the exception interrupt.*/
        .endm



/*;*******************************************************************************
;* Function Name  : UndefinedHandler
;* Description    : This function called when undefined instruction
;                   exception is entered.
;* Input          : none
;* Output         : none
;*********************************************************************************/

UndefinedHandler:
        SaveContext r0,r12    /*; Save the workspace plus the current*/
                              /*; return address lr_ und and spsr_und.*/
        BL      Undefined_Handler/*; Branch to Undefined_Handler*/
        RestoreContext r0,r12 /*; Return to the instruction following...*/
                              /*; ...the undefined instruction.*/

/*;*******************************************************************************
;* Function Name  : SWIHandler
;* Description    : This function called when SWI instruction executed.
;* Input          : none
;* Output         : none
;********************************************************************************/

SWIHandler:
        SaveContext r0, r12
        ldr r0, =SWI_Handler
        ldr lr, =SWI_Handler_end
        bx r0
SWI_Handler_end:
        RestoreContext r0,r12


/*;*******************************************************************************
;* Function Name  : IRQHandler
;* Description    : This function called when IRQ exception is entered.
;* Input          : none
;* Output         : none
;********************************************************************************/


IRQHandler:
    SUB    lr,lr,#4                 /* ; Update the link register */
	SaveContext r0, r12					/*; Save the context of the current task. */

	LDR    r0, =EIC_base_addr
	LDR    r1, =IVR_off_addr
	LDR    lr, =ReturnAddress			/*; Load the return address. */
	ADD    pc,r0,r1						/*; Branch to the IRQ handler. */
ReturnAddress:
	LDR    r0, =EIC_base_addr
	LDR    r2, [r0, #CICR_off_addr]		/*; Get the IRQ channel number. */
	MOV    r3,#1
	MOV    r3,r3,LSL r2
	STR    r3,[r0, #IPR_off_addr]		/*; Clear the corresponding IPR bit. */

	RestoreContext r0,r12					/*; Restore the context of the selected task. */

/*;*******************************************************************************
;* Function Name  : PrefetchAbortHandler
;* Description    : This function called when Prefetch Abort
;                   exception is entered.
;* Input          : none
;* Output         : none
;*********************************************************************************/

PrefetchAbortHandler:
		NOP
		B PrefetchAbortHandler

/*;*******************************************************************************
;* Function Name  : DataAbortHandler
;* Description    : This function is called when Data Abort
;                   exception is entered.
;* Input          : none
;* Output         : none
;********************************************************************************/

DataAbortHandler:
		NOP
		NOP
		B DataAbortHandler
                              /*; ...has generated the data abort exception.*/

/*;*******************************************************************************
;* Function Name  : FIQHandler
;* Description    : This function is called when FIQ
;*                  exception is entered.
;* Input          : none
;* Output         : none
;********************************************************************************/

FIQHandler:
        SUB    lr,lr,#4       /*; Update the link register.*/
        SaveContext r0,r7     /*; Save the workspace plus the current*/
                              /*; return address lr_ fiq and spsr_fiq.*/
        ldr r0,= FIQ_Handler
        ldr lr,= FIQ_Handler_end
        bx r0

FIQ_Handler_end:
        RestoreContext r0,r7  /*; Restore the context and return to the...*/
                              /*; ...program execution.*/

/*;*******************************************************************************
;* Macro Name     : IRQ_to_SYS
;* Description    : This macro used to switch form IRQ mode to SYS mode
;* Input          : none.
;* Output         : none
;*******************************************************************************/
       .macro IRQ_to_SYS

        MSR    cpsr_c,#0x1F
        STMFD  sp!,{lr}

       .endm
/*;*******************************************************************************
;* Macro Name     : SYS_to_IRQ
;* Description    : This macro used to switch from SYS mode to IRQ mode
;                   then to return to IRQHnadler routine.
;* Input          : none.
;* Output         : none.
;*******************************************************************************/
      .macro SYS_to_IRQ

        LDMFD  sp!, {lr}      /*; Restore the link register. */
        MSR    cpsr_c, #0xD2  /*; Switch to IRQ mode.*/
        MOV    pc ,lr         /*; Return to IRQHandler routine to clear the*/
                             /*; pending bit.*/
       .endm

/*;*******************************************************************************
;* Function Name  : WAKUPIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the WAKUP_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  WAKUP_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
WAKUPIRQHandler:
        IRQ_to_SYS
        BL     WAKUP_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM2_OC2IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM3_OC2_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM2_OC2_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM2_OC2IRQHandler:
        IRQ_to_SYS
        BL     TIM2_OC2_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM2_OC1IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM2_OC1_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM2_OC1_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM2_OC1IRQHandler:
        IRQ_to_SYS
        BL     TIM2_OC1_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM2_IC12IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM2_IC12_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM2_IC12_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM2_IC12IRQHandler:
        IRQ_to_SYS
        BL     TIM2_IC12_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM2_UPIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM2_UP_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM3_UP_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM2_UPIRQHandler:
        IRQ_to_SYS
        BL     TIM2_UP_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM1_OC2IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM1_OC2_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM1_OC2_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM1_OC2IRQHandler:
        IRQ_to_SYS
        BL     TIM1_OC2_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM1_OC1IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM1_OC1_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM1_OC1_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM1_OC1IRQHandler:
        IRQ_to_SYS
        BL     TIM1_OC1_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM1_IC12IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM1_IC12_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM1_IC12_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM1_IC12IRQHandler:
        IRQ_to_SYS
        BL     TIM1_IC12_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM1_UPIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM1_UP_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM1_UP_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM1_UPIRQHandler:
        IRQ_to_SYS
        BL     TIM1_UP_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM0_OC2IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM0_OC2_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM0_OC2_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM0_OC2IRQHandler:
        IRQ_to_SYS
        BL     TIM0_OC2_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM0_OC1IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM0_OC1_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM0_OC1_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
TIM0_OC1IRQHandler:
        IRQ_to_SYS
        BL     TIM0_OC1_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM0_IC12IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM0_IC12_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM0_IC12_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
TIM0_IC12IRQHandler:
        IRQ_to_SYS
        BL     TIM0_IC12_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TIM0_UPIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TIM0_UP_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TIM0_UP_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
TIM0_UPIRQHandler:
        IRQ_to_SYS
        BL     TIM0_UP_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : PWM_OC123IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the PWM_OC123_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                    PWM_OC123_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
PWM_OC123IRQHandler:
        IRQ_to_SYS
        BL     PWM_OC123_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : PWM_EMIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the PWM_EM_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  PWM_EM_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
PWM_EMIRQHandler:
        IRQ_to_SYS
        BL     PWM_EM_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : PWM_UPIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the PWM_UP_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  PWM_UP_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
PWM_UPIRQHandler:
        IRQ_to_SYS
        BL     PWM_UP_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : I2CIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the I2C_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  I2C_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
I2CIRQHandler:
        IRQ_to_SYS
        BL     I2C_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : SSP1IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the SSP1_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  SSP1_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
SSP1IRQHandler:
        IRQ_to_SYS
        BL     SSP1_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : SSP0IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the SSP0_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  SSP0_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
SSP0IRQHandler:
        IRQ_to_SYS
        BL     SSP0_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : UART2IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the UART2_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  UART2_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
UART2IRQHandler:
        IRQ_to_SYS
        BL     UART2_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : UART1IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the UART1_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  UART1_IRQHandler function termination.
;* Input          : none
;* Output         : none
;*******************************************************************************/
UART1IRQHandler:
        IRQ_to_SYS
        BL     UART1_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : UART0IRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the UART0_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  UART0_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
UART0IRQHandler:
        IRQ_to_SYS
        BL     UART0_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : CANIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the CAN_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  CAN_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
CANIRQHandler:
        IRQ_to_SYS
        BL     CAN_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : USB_LPIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the USB_LP_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  USB_LP_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
USB_LPIRQHandler:
        IRQ_to_SYS
        BL     USB_LP_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : USB_HPIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the USB_HP_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  USB_HP_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
USB_HPIRQHandler:
        IRQ_to_SYS
        BL     USB_HP_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : ADCIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the ADC_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  ADC_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
ADCIRQHandler:
        IRQ_to_SYS
        BL     ADC_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : DMAIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the DMA_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  DMA_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
DMAIRQHandler:
        IRQ_to_SYS
        BL     DMA_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : EXTITIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the EXTIT_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  EXTIT_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
EXTITIRQHandler:
        IRQ_to_SYS
        BL     EXTIT_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : MRCCIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the MRCC_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  MRCC_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
MRCCIRQHandler:
        IRQ_to_SYS
        BL     MRCC_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : FLASHSMIIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the FLASHSMI_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  FLASHSMI_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
FLASHSMIIRQHandler:
        IRQ_to_SYS
        BL     FLASHSMI_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : RTCIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the RTC_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  RTC_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
RTCIRQHandler:
        IRQ_to_SYS
        BL     RTC_IRQHandler
        SYS_to_IRQ

/*;*******************************************************************************
;* Function Name  : TBIRQHandler
;* Description    : This function used to switch to SYS mode before entering
;*                  the TB_IRQHandler function located in 75x_it.c.
;*                  Then to return to IRQ mode after the
;*                  TB_IRQHandler function termination.
;* Input          : none
;* Output         : none
;********************************************************************************/
TBIRQHandler:
        IRQ_to_SYS
        BL     TB_IRQHandler
        SYS_to_IRQ
/*;**********************************************************************************/



