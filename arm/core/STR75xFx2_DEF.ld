/*
Linker subscript for STR75xFx2 definitions
Copyright RAISONANCE 2005
You can use, modify and distribute thisfile freely, but without any waranty.
*/


/* Memory Spaces Definitions */

MEMORY
{
  /*FLASH (rx) : ORIGIN = 0x20000000, LENGTH = 256K*/
  FLASH (rx) : ORIGIN = 0x20008000, LENGTH = 224K

  FLASHB1 (rx) : ORIGIN = 0x200C0000, LENGTH = 16K
  RAM (xrw) : ORIGIN = 0x40000000, LENGTH = 16K
}

/* higher address of the user mode stack */
_estack = 0x40004000;

