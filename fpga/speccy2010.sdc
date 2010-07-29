## Generated SDC file "speccy2010.sdc"

## Copyright (C) 1991-2009 Altera Corporation
## Your use of Altera Corporation's design tools, logic functions 
## and other software and tools, and its AMPP partner logic 
## functions, and any output files from any of the foregoing 
## (including device programming or simulation files), and any 
## associated documentation or information are expressly subject 
## to the terms and conditions of the Altera Program License 
## Subscription Agreement, Altera MegaCore Function License 
## Agreement, or other applicable license agreement, including, 
## without limitation, that your use is for the sole purpose of 
## programming logic devices manufactured by Altera and sold by 
## Altera or its authorized distributors.  Please refer to the 
## applicable agreement for further details.


## VENDOR  "Altera"
## PROGRAM "Quartus II"
## VERSION "Version 9.1 Build 222 10/21/2009 SJ Full Version"

## DATE    "Thu Jul 22 22:49:28 2010"

##
## DEVICE  "EP2C8Q208C8"
##


#**************************************************************
# Time Information
#**************************************************************

set_time_format -unit ns -decimal_places 3



#**************************************************************
# Create Clock
#**************************************************************

create_clock -name {CLK_20} -period 50.000 -waveform { 0.000 25.000 } [get_ports {CLK_20}]


#**************************************************************
# Create Generated Clock
#**************************************************************

derive_pll_clocks -use_tan_name
create_generated_clock -name {memclk} -source [get_ports {CLK_20}] -multiply_by 84 -divide_by 20 -master_clock {CLK_20} [get_ports {pMemClk}] 


#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************



#**************************************************************
# Set Input Delay
#**************************************************************



#**************************************************************
# Set Output Delay
#**************************************************************

set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[0]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[1]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[2]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[3]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[4]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[5]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[6]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[7]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[8]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[9]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[10]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[11]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemAdr[12]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemBa0}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemBa1}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemCas_n}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemCke}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemCs_n}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[0]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[1]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[2]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[3]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[4]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[5]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[6]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[7]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[8]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[9]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[10]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[11]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[12]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[13]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[14]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemDat[15]}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemLdq}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemRas_n}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemUdq}]
set_output_delay -add_delay -max -clock [get_clocks {memclk}]  5.000 [get_ports {pMemWe_n}]


#**************************************************************
# Set Clock Groups
#**************************************************************



#**************************************************************
# Set False Path
#**************************************************************



#**************************************************************
# Set Multicycle Path
#**************************************************************



#**************************************************************
# Set Maximum Delay
#**************************************************************



#**************************************************************
# Set Minimum Delay
#**************************************************************



#**************************************************************
# Set Input Transition
#**************************************************************

