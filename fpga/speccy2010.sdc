## Generated SDC file "speccy2010.sdc"

## Copyright (C) 1991-2013 Altera Corporation
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
## VERSION "Version 13.0.1 Build 232 06/12/2013 Service Pack 1 SJ Web Edition"

## DATE    "Thu Jun 27 00:57:30 2019"

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

create_clock -name {clk20} -period 50.000 -waveform { 0.000 25.000 } [get_ports {CLK_20}]


#**************************************************************
# Create Generated Clock
#**************************************************************

create_generated_clock -name {clk84} -source [get_ports {CLK_20}] -multiply_by 21 -divide_by 5 -master_clock {clk20} [get_nets {U00|altpll_component|_clk0}] 
create_generated_clock -name {clk42} -source [get_ports {CLK_20}] -multiply_by 21 -divide_by 10 -master_clock {clk20} [get_nets {U00|altpll_component|_clk1}] 
create_generated_clock -name {clk84_pin} -source [get_ports {CLK_20}] -multiply_by 21 -divide_by 5 -master_clock {clk20} [get_ports {pMemClk}] 


#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************



#**************************************************************
# Set Input Delay
#**************************************************************

set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[0]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[0]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[1]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[1]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[2]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[2]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[3]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[3]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[4]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[4]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[5]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[5]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[6]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[6]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[7]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[7]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[8]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[8]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[9]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[9]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[10]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[10]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[11]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[11]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[12]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[12]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[13]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[13]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[14]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[14]}]
set_input_delay -add_delay -max -clock [get_clocks {clk84_pin}]  6.200 [get_ports {pMemDat[15]}]
set_input_delay -add_delay -min -clock [get_clocks {clk84_pin}]  2.500 [get_ports {pMemDat[15]}]


#**************************************************************
# Set Output Delay
#**************************************************************

set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[0]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[0]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[1]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[1]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[2]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[2]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[3]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[3]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[4]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[4]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[5]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[5]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[6]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[6]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[7]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[7]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[8]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[8]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[9]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[9]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[10]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[10]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[11]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[11]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemAdr[12]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemAdr[12]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemBa0}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemBa0}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemBa1}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemBa1}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemCas_n}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemCas_n}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemCke}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemCke}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemCs_n}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemCs_n}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[0]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[0]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[1]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[1]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[2]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[2]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[3]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[3]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[4]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[4]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[5]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[5]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[6]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[6]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[7]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[7]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[8]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[8]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[9]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[9]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[10]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[10]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[11]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[11]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[12]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[12]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[13]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[13]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[14]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[14]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemDat[15]}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemDat[15]}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemLdq}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemLdq}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemRas_n}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemRas_n}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemUdq}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemUdq}]
set_output_delay -add_delay -max -clock [get_clocks {clk84_pin}]  2.000 [get_ports {pMemWe_n}]
set_output_delay -add_delay -min -clock [get_clocks {clk84_pin}]  -1.500 [get_ports {pMemWe_n}]


#**************************************************************
# Set Clock Groups
#**************************************************************



#**************************************************************
# Set False Path
#**************************************************************

set_false_path  -from  [get_clocks {clk20}]  -to  [get_clocks {clk84}]
set_false_path  -from  [get_clocks {clk20}]  -to  [get_clocks {clk42}]
set_false_path  -from  [get_clocks {clk84}]  -to  [get_clocks {clk84}]
set_false_path  -from  [get_clocks {clk42}]  -to  [get_clocks {clk84}]
set_false_path  -from  [get_clocks {clk84_pin}]  -to  [get_clocks {clk84}]
set_false_path  -from  [get_clocks {clk84}]  -to  [get_clocks {clk42}]
set_false_path  -from  [get_clocks {clk42}]  -to  [get_clocks {clk42}]
set_false_path  -from  [get_clocks {clk84}]  -to  [get_clocks {clk84_pin}]


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

