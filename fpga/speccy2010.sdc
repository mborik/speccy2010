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
create_generated_clock -name {memclk_pin} -source [get_ports {CLK_20}] -multiply_by 84 -divide_by 20 -master_clock {CLK_20} [get_ports {pMemClk}] 


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

set_input_delay -max -clock {memclk_pin}  6.200 [get_ports {pMemDat[*]}]
set_input_delay -min -clock {memclk_pin}  2.500 [get_ports {pMemDat[*]}]

set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemCs_n}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemCke}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemCas_n}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemRas_n}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemWe_n}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemLdq}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemUdq}]

set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemBa*}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemAdr[*]}]
set_output_delay -max -clock {memclk_pin}  2.000 [get_ports {pMemDat[*]}]

set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemCs_n}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemCke}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemCas_n}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemRas_n}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemWe_n}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemLdq}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemUdq}]

set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemBa*}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemAdr[*]}]
set_output_delay -min -clock {memclk_pin}  -1.500 [get_ports {pMemDat[*]}]

#**************************************************************
# Set Clock Groups
#**************************************************************



#**************************************************************
# Set False Path
#**************************************************************

set_false_path  -from  [get_clocks {CLK_20}]  -to  [get_clocks {pll:U00|altpll:altpll_component|_clk0}]


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

