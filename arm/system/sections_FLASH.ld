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

        *(.text)
        *(.text*)                   /* remaining code */

        *(.glue_7)
        *(.glue_7t)

    } >FLASH

    .rodata :
    {
	    . = ALIGN(4);

        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)

    } >FLASH

	.init :
	{
		KEEP (*(.init))
		. = ALIGN(4);
	} >FLASH =0xff

	.fini           :
	{
		KEEP (*(.fini))
		. = ALIGN(4);  /* REQUIRED. LD is flaky without it. */
	} >FLASH =0

    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH

    __exidx_start = .;
    .ARM.exidx   : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >FLASH
    __exidx_end = .;

     /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
    . = ALIGN(32 / 8);

    PROVIDE (__preinit_array_start = .);
    .preinit_array     : { KEEP (*(.preinit_array)) } >FLASH = 0xff
    PROVIDE (__preinit_array_end = .);
    PROVIDE (__init_array_start = .);
    .init_array     : { KEEP (*(.init_array)) } >FLASH = 0xff
    PROVIDE (__init_array_end = .);
    PROVIDE (__fini_array_start = .);
    .fini_array     : { KEEP (*(.fini_array)) } >FLASH = 0xff
    PROVIDE (__fini_array_end = .);

	.ctors :
	{
		/*	gcc uses crtbegin.o to find the start of the constructors, so
			we make sure it is first.  Because this is a wildcard, it
			doesn't matter if the user does not actually link against
			crtbegin.o; the linker won't look for a file to match a
			wildcard.  The wildcard also means that it doesn't matter which
			directory crtbegin.o is in.  */
		KEEP (*crtbegin.o(.ctors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >FLASH = 0

	.dtors :
	{
		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >FLASH = 0


	.eh_frame :
	{
		KEEP (*(.eh_frame))
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >FLASH = 0

	.gcc_except_table :
	{
		*(.gcc_except_table)
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */

	    . = ALIGN(4);
        _etext = .;
	    /* This is used by the startup in order to initialize the .data secion */
        _sidata = _etext;
	} >FLASH = 0


    /* This is the initialized data section
    The program executes knowing that the data is in the RAM
    but the loader puts the initial values in the FLASH (inidata).
    It is one task of the startup to copy the initial values from FLASH to RAM. */
    .data  : AT ( _sidata )
    {
	    . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _sdata = . ;

        KEEP( *(.vectorsTableRam) )
        KEEP( *(.jcr) )
        *(.data)
        *(.data*)

	    . = ALIGN(4);
	    /* This is used by the startup in order to initialize the .data secion */
   	 _edata = . ;
    } >RAM

    /* This is the uninitialized data section */
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



