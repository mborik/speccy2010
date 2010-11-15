/*
This is the default Startup for STR75x devices for the GNU toolchain

It has been designed by ST Microelectronics and modified by Raisonance
and FreeRTOS.org.

You can use it, modify it, distribute it freely but without any waranty.
*/

/*; Depending on Your Application, Disable or Enable the following Defines*/

/*; ----------------------------------------------------------------------------
  ;                      Memory remapping
  ; ----------------------------------------------------------------------------*/
.set Remap_SRAM, 1   /* remap SRAM at address 0x00 if 1 */

/*  ; ----------------------------------------------------------------------------
  ;                      EIC initialization
  ; ----------------------------------------------------------------------------*/
.set EIC_INIT, 1     /*; Configure and Initialize EIC if 1*/

.extern main

.extern UndefinedHandler
.extern SWIHandler
.extern PrefetchAbortHandler
.extern DataAbortHandler
.extern IRQHandler
.extern FIQHandler
.extern WAKUPIRQHandler
.extern TIM2_OC2IRQHandler
.extern TIM2_OC1IRQHandler
.extern TIM2_IC12IRQHandler
.extern TIM2_UPIRQHandler
.extern TIM1_OC2IRQHandler
.extern TIM1_OC1IRQHandler
.extern TIM1_IC12IRQHandler
.extern TIM1_UPIRQHandler
.extern TIM0_OC2IRQHandler
.extern TIM0_OC1IRQHandler
.extern TIM0_IC12IRQHandler
.extern TIM0_UPIRQHandler
.extern PWM_OC123IRQHandler
.extern PWM_EMIRQHandler
.extern PWM_UPIRQHandler
.extern I2CIRQHandler
.extern SSP1IRQHandler
.extern SSP0IRQHandler
.extern UART2IRQHandler
.extern UART1IRQHandler
.extern UART0IRQHandler
.extern CANIRQHandler
.extern USB_LPIRQHandler
.extern USB_HPIRQHandler
.extern ADCIRQHandler
.extern DMAIRQHandler
.extern EXTITIRQHandler
.extern MRCCIRQHandler
.extern FLASHSMIIRQHandler
.extern RTCIRQHandler
.extern TBIRQHandler

;/* init value for the stack pointer. defined in linker script */
.extern _estack

;/* Stack Sizes. The default values are in the linker script, but they can be overriden. */
.extern _UND_Stack_Init
.extern _SVC_Stack_Init
.extern _ABT_Stack_Init
.extern _FIQ_Stack_Init
.extern _IRQ_Stack_Init
.extern _USR_Stack_Init

SVC_Stack           =     _SVC_Stack_Init
IRQ_Stack           =     _IRQ_Stack_Init
USR_Stack           =     _USR_Stack_Init
FIQ_Stack           =     _FIQ_Stack_Init
ABT_Stack           =     _ABT_Stack_Init
UNDEF_Stack         =     _UND_Stack_Init

;/* the following are useful for initializing the .data section */
.extern _sidata ;/* start address for the initialization values of the .data section. defined in linker script */
.extern _sdata ;/* start address for the .data section. defined in linker script */
.extern _edata ;/* end address for the .data section. defined in linker script */

;/* the following are useful for initializing the .bss section */
.extern _sbss ;/* start address for the .bss section. defined in linker script */
.extern _ebss ;/* end address for the .bss section. defined in linker script */

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

/*; --- System memory locations */

/*; MRCC Register*/
MRCC_PCLKEN_Addr    =    0x60000030  /*; Peripheral Clock Enable register base address*/

/*; CFG Register*/
CFG_GLCONF_Addr     =    0x60000010  /*; Global Configuration register base address*/
SRAM_mask           =    0x0002      /*; to remap RAM at 0x0*/

/*; GPIO Register*/
GPIOREMAP0R_Addr    =    0xFFFFE420
SMI_EN_Mask         =    0x00000001

/*; SMI Register*/
SMI_CR1_Addr        =    0x90000000

/*; --- Stack Addres for each ARM mode*/
/*; add FIQ_Stack, ABT_Stack, UNDEF_Stack here if you need them*/


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

/***************************************************************************************/

.section .vectorsTableRam

        LDR     PC, Reset_Addr_RAM
        LDR     PC, Undefined_Addr_RAM
        LDR     PC, SWI_Addr_RAM
        LDR     PC, Prefetch_Addr_RAM
        LDR     PC, Abort_Addr_RAM
        NOP                          /*; Reserved vector*/
        LDR     PC, IRQ_Addr_RAM
        LDR     PC, FIQ_Addr_RAM

Reset_Addr_RAM      : .long     Reset_Handler
Undefined_Addr_RAM  : .long     UndefinedHandler
SWI_Addr_RAM        : .long     SWIHandler
Prefetch_Addr_RAM   : .long     PrefetchAbortHandler
Abort_Addr_RAM      : .long     DataAbortHandler
                  .long 0      /*; Reserved vector*/
IRQ_Addr_RAM        : .long     IRQHandler
FIQ_Addr_RAM        : .long     FIQHandler

__wrongvector_RAM:
	ldr     PC, __wrongvector_Addr_RAM
__wrongvector_Addr_RAM:
	.long 0

WAKUP_Addr_RAM         :.long	WAKUPIRQHandler
TIM2_OC2_Addr_RAM      :.long	TIM2_OC2IRQHandler
TIM2_OC1_Addr_RAM      :.long	TIM2_OC1IRQHandler
TIM2_IC12_Addr_RAM     :.long	TIM2_IC12IRQHandler
TIM2_UP_Addr_RAM       :.long	TIM2_UPIRQHandler
TIM1_OC2_Addr_RAM      :.long	TIM1_OC2IRQHandler
TIM1_OC1_Addr_RAM      :.long	TIM1_OC1IRQHandler
TIM1_IC12_Addr_RAM     :.long	TIM1_IC12IRQHandler
TIM1_UP_Addr_RAM       :.long	TIM1_UPIRQHandler
TIM0_OC2_Addr_RAM      :.long	TIM0_OC2IRQHandler
TIM0_OC1_Addr_RAM      :.long	TIM0_OC1IRQHandler
TIM0_IC12_Addr_RAM     :.long	TIM0_IC12IRQHandler
TIM0_UP_Addr_RAM       :.long	TIM0_UPIRQHandler
PWM_OC123_Addr_RAM     :.long	PWM_OC123IRQHandler
PWM_EM_Addr_RAM        :.long	PWM_EMIRQHandler
PWM_UP_Addr_RAM        :.long	PWM_UPIRQHandler
I2C_Addr_RAM           :.long	I2CIRQHandler
SSP1_Addr_RAM          :.long	SSP1IRQHandler
SSP0_Addr_RAM          :.long	SSP0IRQHandler
UART2_Addr_RAM         :.long	UART2IRQHandler
UART1_Addr_RAM         :.long	UART1IRQHandler
UART0_Addr_RAM         :.long	UART0IRQHandler
CAN_Addr_RAM           :.long	CANIRQHandler
USB_LP_Addr_RAM        :.long	USB_LPIRQHandler
USB_HP_Addr_RAM        :.long	USB_HPIRQHandler
ADC_Addr_RAM           :.long	ADCIRQHandler
DMA_Addr_RAM           :.long	DMAIRQHandler
EXTIT_Addr_RAM         :.long	EXTITIRQHandler
MRCC_Addr_RAM          :.long	MRCCIRQHandler
FLASHSMI_Addr_RAM      :.long	FLASHSMIIRQHandler
RTC_Addr_RAM           :.long	RTCIRQHandler
TB_Addr_RAM            :.long	TBIRQHandler

.globl _start
.globl _startup

.section .text

_startup:
_start:
        LDR     PC, Reset_Addr
Reset_Addr      : .long     Reset_Handler

Reset_Handler:
        LDR     pc, =NextInst

NextInst:
/*; Reset all Peripheral Clocks*/
/*; This is usefull only when using debugger to Reset\Run the application*/

        LDR     r0, =0x00000000          /*; Disable peripherals clock*/
        LDR     r1, =MRCC_PCLKEN_Addr
        STR     r0, [r1]

        LDR     r0, =0x1975623F          /*; Peripherals kept under reset*/
        STR     r0, [r1,#4]
        MOV     r0, #0
        NOP                              /*; Wait*/
        NOP
        NOP
        NOP
        STR     r0, [r1,#4]              /*; Disable peripherals reset*/

/*; Initialize stack pointer registers
  ; Enter each mode in turn and set up the stack pointer*/

        MSR     CPSR_c, #Mode_FIQ|I_Bit|F_Bit /*; No interrupts*/
        ldr     sp, =FIQ_Stack

        MSR     CPSR_c, #Mode_IRQ|I_Bit|F_Bit /*; No interrupts*/
        ldr     sp, =IRQ_Stack

        MSR     CPSR_c, #Mode_ABT|I_Bit|F_Bit /*; No interrupts*/
        ldr     sp, =ABT_Stack

        MSR     CPSR_c, #Mode_UNDEF|I_Bit|F_Bit /*; No interrupts*/
        ldr     sp, =UNDEF_Stack

        MSR     CPSR_c, #Mode_SVC|I_Bit|F_Bit /*; No interrupts*/
        ldr     sp, =SVC_Stack

        MSR     CPSR_c, #Mode_SYS /*; No interrupts*/
        ldr     sp, =USR_Stack

/*; ----------------------------------------------------------------------------
; Description  :  Remapping SRAM at address 0x00 after the application has
;                 started executing.
; ----------------------------------------------------------------------------*/
 .if  Remap_SRAM
        MOV     r0, #SRAM_mask
        LDR     r1, =CFG_GLCONF_Addr
        LDR     r2, [r1]                  /*; Read GLCONF Register*/
        BIC     r2, r2, #0x03             /*; Reset the SW_BOOT bits*/
        ORR     r2, r2, r0                /*; Change the SW_BOOT bits*/
        STR     r2, [r1]                  /*; Write GLCONF Register*/
  .endif

/*;-------------------------------------------------------------------------------
;Description  : Initialize the EIC as following :
;              - IRQ disabled
;              - FIQ disabled
;              - IVR contains the load PC opcode
;              - All channels are disabled
;              - All channels priority equal to 0
;              - All SIR registers contains offset to the related IRQ table entry
;-------------------------------------------------------------------------------*/
  .if EIC_INIT
        LDR     r3, =EIC_base_addr
        LDR     r4, =0x00000000
        STR     r4, [r3, #ICR_off_addr]   /*; Disable FIQ and IRQ*/
        STR     r4, [r3, #IER_off_addr]   /*; Disable all interrupts channels*/

        LDR     r4, =0xFFFFFFFF
        STR     r4, [r3, #IPR_off_addr]   /*; Clear all IRQ pending bits*/

        LDR     r4, =0x18
        STR     r4, [r3, #FIR_off_addr]   /*; Disable FIQ channels and clear FIQ pending bits*/

        LDR     r4, =0x00000000
        STR     r4, [r3, #CIPR_off_addr]  /*; Reset the current priority register*/

        LDR     r4, =0xE59F0000           /*; Write the LDR pc,pc,#offset..*/
        STR     r4, [r3, #IVR_off_addr]   /*; ..instruction code in IVR[31:16]*/


        LDR     r2,= 32                   /*; 32 Channel to initialize*/
        LDR     r0, =WAKUP_Addr_RAM           /*; Read the address of the IRQs address table*/
        LDR     r1, =0x00000FFF
        AND     r0,r0,r1
        LDR     r5,=SIR0_off_addr         /*; Read SIR0 address*/
        SUB     r4,r0,#8                  /*; subtract 8 for prefetch*/
        LDR     r1, =0xF7E8               /*; add the offset to the 0x00 address..*/
                                          /*; ..(IVR address + 7E8 = 0x00)*/
                                          /*; 0xF7E8 used to complete the LDR pc,offset opcode*/
        ADD     r1,r4,r1                  /*; compute the jump offset*/
EIC_INI:
        MOV     r4, r1, LSL #16           /*; Left shift the result*/
        STR     r4, [r3, r5]              /*; Store the result in SIRx register*/
        ADD     r1, r1, #4                /*; Next IRQ address*/
        ADD     r5, r5, #4                /*; Next SIR*/
        SUBS    r2, r2, #1                /*; Decrement the number of SIR registers to initialize*/
        BNE     EIC_INI                   /*; If more then continue*/

 .endif

  /* ;copy the initial values for .data section from FLASH to RAM */
	ldr	R1, =_sidata
	ldr	R2, =_sdata
	ldr	R3, =_edata
_reset_inidata_loop:
	cmp	R2, R3
	ldrlO	R0, [R1], #4
	strlO	R0, [R2], #4
	blO	_reset_inidata_loop

	;/* Clear the .bss section */
	mov   r0,#0						;/* get a zero */
	ldr   r1,=_sbss				;/* point to bss start */
	ldr   r2,=_ebss				;/* point to bss end */
_reset_inibss_loop:
	cmp   r1,r2						;/* check if some data remains to clear */
	strlo r0,[r1],#4				;/* clear 4 bytes */
	blo   _reset_inibss_loop	;/* loop until done */

	bl __libc_init_array
    b main







