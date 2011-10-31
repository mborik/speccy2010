library ieee;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity SoundMixer is
  port (
    CLK           : in  std_logic; 
    I_AUDIO1_AY_A : in  std_logic_vector(7 downto 0);--AY1
    I_AUDIO1_AY_B : in  std_logic_vector(7 downto 0);
    I_AUDIO1_AY_C : in  std_logic_vector(7 downto 0);
    
    I_AUDIO2_AY_A : in  std_logic_vector(7 downto 0);--AY2
    I_AUDIO2_AY_B : in  std_logic_vector(7 downto 0);
    I_AUDIO2_AY_C : in  std_logic_vector(7 downto 0);

	 WEIGHT_AUDIO1_AY_A : in unsigned(23 downto 0);--AY1 - WEIGHT
    WEIGHT_AUDIO1_AY_B : in unsigned(23 downto 0);-- sum should be <=1
    WEIGHT_AUDIO1_AY_C : in unsigned(23 downto 0);
    
    WEIGHT_AUDIO2_AY_A : in unsigned(23 downto 0);--AY2 - WEIGHT
    WEIGHT_AUDIO2_AY_B : in unsigned(23 downto 0);-- sum should be <=1
    WEIGHT_AUDIO2_AY_C : in unsigned(23 downto 0);
	 
    I_AUDIO_AUX_1 : in  std_logic_vector( 7 downto 0);--COVOX C1
    I_AUDIO_AUX_2 : in  std_logic_vector( 7 downto 0);--COVOX C2
    I_AUDIO_AUX_3 : in  std_logic_vector(15 downto 0);--SID

    WEIGHT_AUDIO_AUX_1 : in  unsigned(23 downto 0);--COVOX C1
    WEIGHT_AUDIO_AUX_2 : in  unsigned(23 downto 0);--COVOX C2
    WEIGHT_AUDIO_AUX_3 : in  unsigned(23 downto 0);--SID

	 I_AUDIO_BEEPER	: in std_logic;
	 I_AUDIO_TAPE	  	: in std_logic;
	 
    WEIGHT_AUDIO_BEEPER : in  unsigned(23 downto 0);--BEEPER
    WEIGHT_AUDIO_TAPE   : in  unsigned(23 downto 0);--TAPE
	 
	 
    I_AY1_ENABLED  : in std_logic;
    I_AY2_ENABLED  : in std_logic;
    I_AUX1_ENABLED : in std_logic;
    I_AUX2_ENABLED : in std_logic;
    I_AUX3_ENABLED : in std_logic;
    O_AUDIO       : out std_logic_vector(15 downto 0)
  );
end;

architecture RTL of SoundMixer is
  
  type  array_5xu16_8   is array (0 to 31) of unsigned( 23 downto 0);
--  type  array_8xu16_8   is array (0 to 255) of unsigned( 11 downto 0);
 
  constant dividerTable : array_5xu16_8 :=(
		x"000000",x"000100",x"000080",x"000055",--00,01,02,03 
		x"000040",x"000033",x"00002a",x"000024",--04,05,06,07 
		x"000020",x"00001c",x"000019",x"000017",--08,09,0a,0b 
		x"000015",x"000013",x"000012",x"000011",--0c,0d,0e,0f 
		x"000010",x"00000f",x"00000e",x"00000d",--10,11,12,13 
		x"00000c",x"00000c",x"00000b",x"00000b",--14,15,16,17 
		x"00000a",x"00000a",x"000009",x"000009",--18,19,1a,1b 
		x"000009",x"000008",x"000008",x"000008" --1c,1d,1e,1f
    );
	 
	signal in_ay1   :unsigned (23 downto 0);
	signal in_ay2   :unsigned (23 downto 0);
	signal in_aux1  :unsigned (23 downto 0);
	signal in_aux2  :unsigned (23 downto 0);
	signal in_aux3  :unsigned (23 downto 0);
	signal in_beeper:unsigned (23 downto 0);
	signal in_tape  :unsigned (23 downto 0);

	signal weight_ay1   :unsigned (23 downto 0);
	signal weight_ay2   :unsigned (23 downto 0);
	signal weight_aux1  :unsigned (23 downto 0);
	signal weight_aux2  :unsigned (23 downto 0);
	signal weight_aux3  :unsigned (23 downto 0);
	signal weight_beeper:unsigned (23 downto 0);
	signal weight_tape  :unsigned (23 downto 0);

	signal weight_sum:unsigned (23 downto 0);
	
begin

  process(clk)
  
  variable output		:unsigned (23 downto 0); 

  begin
	if clk'event and clk = '1' then
		--output  		:= x"000000";
		--weight_sum	:= x"0000FF";

		if I_AY1_ENABLED = '1' then
			weight_ay1 <= WEIGHT_AUDIO1_AY_A + WEIGHT_AUDIO1_AY_B + WEIGHT_AUDIO1_AY_C;
			in_ay1 <= unsigned(unsigned(x"0000" & I_AUDIO1_AY_A) * unsigned(WEIGHT_AUDIO1_AY_A))(23 downto 0) +
									 unsigned(unsigned(x"0000" & I_AUDIO1_AY_B) * unsigned(WEIGHT_AUDIO1_AY_B))(23 downto 0) + 
									 unsigned(unsigned(x"0000" & I_AUDIO1_AY_C) * unsigned(WEIGHT_AUDIO1_AY_C))(23 downto 0);
		else
			in_ay1     <= x"000000";
			weight_ay1 <= x"000000";
		end if;

		if I_AY2_ENABLED = '1' then
			weight_ay1 <= WEIGHT_AUDIO2_AY_A + WEIGHT_AUDIO2_AY_B + WEIGHT_AUDIO2_AY_C;
			in_ay2 <= unsigned(unsigned(x"0000" & I_AUDIO2_AY_A) * unsigned(WEIGHT_AUDIO2_AY_A))(23 downto 0) +
									 unsigned(unsigned(x"0000" & I_AUDIO2_AY_B) * unsigned(WEIGHT_AUDIO2_AY_B))(23 downto 0) + 
									 unsigned(unsigned(x"0000" & I_AUDIO2_AY_C) * unsigned(WEIGHT_AUDIO2_AY_C))(23 downto 0);
		else
			in_ay2     <= x"000000";
			weight_ay2 <= x"000000";
		end if;

		if I_AUX1_ENABLED = '1' then
			weight_aux1 <= WEIGHT_AUDIO_AUX_1;
			in_aux1 <= unsigned(unsigned(x"0000" & I_AUDIO_AUX_1) * unsigned(WEIGHT_AUDIO_AUX_1))(23 downto 0);
		else
			in_aux1     <= x"000000";
			weight_aux1 <= x"000000";
		end if;
		
		if I_AUX2_ENABLED = '1' then
			weight_aux2 <= WEIGHT_AUDIO_AUX_2;
			in_aux2 <= unsigned(unsigned(x"0000" & I_AUDIO_AUX_2) * unsigned(WEIGHT_AUDIO_AUX_2))(23 downto 0);
		else
			in_aux2     <= x"000000";
			weight_aux2 <= x"000000";
		end if;
		
		if I_AUX3_ENABLED = '1' then
			weight_aux3 <= WEIGHT_AUDIO_AUX_3;
			in_aux3 <= unsigned(unsigned(x"00" & I_AUDIO_AUX_3) * unsigned(WEIGHT_AUDIO_AUX_3))(23 downto 0);
		else
			in_aux3     <= x"000000";
			weight_aux3 <= x"000000";
		end if;
		
		if I_AUDIO_BEEPER = '1' then
			in_beeper <= unsigned(x"0000FF" * unsigned(WEIGHT_AUDIO_BEEPER))(23 downto 0);
		else
			in_beeper <= x"000000";
		end if;

		if I_AUDIO_TAPE = '1' then
			in_tape <= unsigned(x"0000FF" * unsigned(WEIGHT_AUDIO_TAPE))(23 downto 0);
		else
			in_tape <= x"000000";
		end if;
	end if;
  end process;

  weight_sum <= x"0000FF" + weight_ay1 + weight_ay2 + weight_aux1 + weight_aux2 + weight_aux3 + WEIGHT_AUDIO_BEEPER + WEIGHT_AUDIO_TAPE;

  O_AUDIO <= std_logic_vector(
				unsigned(in_ay1 + in_ay2 + in_aux1 + in_aux2 + in_aux3 + in_beeper + in_tape)(23 downto 8) * 
				unsigned(dividerTable(to_integer(weight_sum(12 downto 8)))))(15 downto 0);--assuming input signals were all 8 bit only

end architecture RTL;
