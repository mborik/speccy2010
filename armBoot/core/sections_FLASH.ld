/*
Common part of the linker scripts for STR71x devices in FLASH mode
(that is, the FLASH is seen at 0)
Copyright RAISONANCE 2005
You can use, modify and distribute thisfile freely, but without any waranty.
*/



/* Sections Definitions */

SECTIONS
{
    /* the program code is stored in the .text section, which goes to Flash */

    .text :
    {
	    KEEP (*crt0.o(.text))

        _etext = .;
        _sidata = _etext;
    } >FLASH

    .data  : AT ( _sidata )
    {
	    . = ALIGN(4);
        _sdata = . ;

        KEEP( *(.vectorsTableRam) )

        *(.text)
        *(.text*)                   /* remaining code */

	    . = ALIGN(4);
        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)

		. = ALIGN(4);
		KEEP (*(.init))
		. = ALIGN(4);
		KEEP (*(.fini))

	    . = ALIGN(4);
        KEEP( *(.jcr) )
        *(.data)
        *(.data*)

	    . = ALIGN(4);
		_edata = . ;

    } >RAM

    .bss :
    {
	    . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss = .;
        __bss_start__ = .;

        *(.bss)
        *(.bss*)
        *(COMMON)

	    . = ALIGN(4);
	    /* This is used by the startup in order to initialize the .bss secion */
   	 	_ebss = . ;
		__bss_end__ = .;

    } >RAM

    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );
    PROVIDE ( __end__ = _ebss );

    PROVIDE ( heap_start = _ebss );
    PROVIDE ( heap_end = _estack );

    /* This is the user stack section
    This is just to check that there is enough RAM left for the User mode stack
    It should generate an error if it's full.
     */
    ._usrstack :
    {
	    . = ALIGN(4);
        _susrstack = . ;

        . = . + _Minimum_Stack_Size ;

	    . = ALIGN(4);
        _eusrstack = . ;
    } >RAM



    /* this is the FLASH Bank1 */
    /* the C or assembly source must explicitly place the code or data there
    using the "section" attribute */
    .b1text :
    {
        *(.b1text)                   /* remaining code */
        *(.b1rodata)                 /* read-only data (constants) */
        *(.b1rodata*)
    } >FLASHB1

    /* this is the EXTMEM */
    /* the C or assembly source must explicitly place the code or data there
    using the "section" attribute */

    /* EXTMEM Bank0 */
    .eb0text :
    {
        *(.eb0text)                   /* remaining code */
        *(.eb0rodata)                 /* read-only data (constants) */
        *(.eb0rodata*)
    } >EXTMEMB0

    /* EXTMEM Bank1 */
    .eb1text :
    {
        *(.eb1text)                   /* remaining code */
        *(.eb1rodata)                 /* read-only data (constants) */
        *(.eb1rodata*)
    } >EXTMEMB1

    /* EXTMEM Bank2 */
    .eb2text :
    {
        *(.eb2text)                   /* remaining code */
        *(.eb2rodata)                 /* read-only data (constants) */
        *(.eb2rodata*)
    } >EXTMEMB2

    /* EXTMEM Bank0 */
    .eb3text :
    {
        *(.eb3text)                   /* remaining code */
        *(.eb3rodata)                 /* read-only data (constants) */
        *(.eb3rodata*)
    } >EXTMEMB3



    /* after that it's only debugging information. */

    /* remove the debugging information from the standard libraries */
    DISCARD :
    {
     libc.a ( * )
     libm.a ( * )
     libgcc.a ( * )
     }

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}



