/*
Default linker script for STR75xFx0
Copyright RAISONANCE S.A.S. 2006
*/

/* include the common STRxxx sub-script */
INCLUDE "STR75x_COMMON.ld"

/* include the memory spaces definitions sub-script */
INCLUDE "STR75xFx2_DEF.ld"

/* include the sections management sub-script for FLASH mode */
INCLUDE "sections_FLASH.ld"


