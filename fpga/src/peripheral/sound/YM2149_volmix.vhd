library ieee;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity YM2149 is
  port (

  CLK                 : in  std_logic;  	-- note 6 Mhz
  ENA                 : in  std_logic; 		-- clock enable for higher speed operation
  RESET_L             : in  std_logic;
  I_SEL_L             : in  std_logic;

  I_DA                : in  std_logic_vector(7 downto 0);
  O_DA                : out std_logic_vector(7 downto 0);

  busctrl_addr         : std_logic;
  busctrl_we           : std_logic;
  busctrl_re           : std_logic;
  ctrl_aymode         : std_logic;
  
  O_AUDIO_A           : out std_logic_vector(7 downto 0);
  O_AUDIO_B           : out std_logic_vector(7 downto 0);
  O_AUDIO_C           : out std_logic_vector(7 downto 0);
  
  O_AUDIO             : out std_logic_vector(7 downto 0)
  );
end;

architecture RTL of YM2149 is


  --  signals
  type  array_16x8   is array (0 to 15) of std_logic_vector(7 downto 0);
  type  array_3x12   is array (1 to 3) of std_logic_vector(11 downto 0);

  signal cnt_div              : unsigned(3 downto 0) := (others => '0');
  signal noise_div            : std_logic := '0';
  signal ena_div              : std_logic;
  signal ena_div_noise        : std_logic;
  signal poly17               : std_logic_vector(16 downto 0) := (others => '0');

  -- registers
  signal addr                 : std_logic_vector(7 downto 0);

  signal reg                  : array_16x8;
  signal env_reset            : std_logic;

  signal noise_gen_cnt        : unsigned(4 downto 0);
  signal noise_gen_op         : std_logic;
  signal tone_gen_cnt         : array_3x12 := (others => (others => '0'));
  signal tone_gen_op          : std_logic_vector(3 downto 1) := "000";

  signal env_gen_cnt          : std_logic_vector(15 downto 0);
  signal env_ena              : std_logic;
  signal env_hold             : std_logic;
  signal env_inc              : std_logic;
  signal env_vol              : std_logic_vector(4 downto 0);
  
  signal A 					  : std_logic_vector(4 downto 0);
  signal B 					  : std_logic_vector(4 downto 0);
  signal C 					  : std_logic_vector(4 downto 0);

  type volTableType32 is array (0 to 31) of unsigned(7 downto 0);
  type volTableType16 is array (0 to 15) of unsigned(7 downto 0);

  constant volTableAy : volTableType16 :=(
		x"00", x"03", x"04", x"06",
		x"0a", x"0f", x"15", x"22", 
		x"28", x"41", x"5b", x"72", 
		x"90", x"b5", x"d7", x"ff" 
    );

  constant volTableYm : volTableType32 :=(
		x"00", x"01", x"01", x"02", x"02", x"03", x"03", x"04",
		x"06", x"07", x"09", x"0a", x"0c", x"0e", x"11", x"13",
		x"17", x"1b", x"20", x"25", x"2c", x"35", x"3e", x"47",
		x"54", x"66", x"77", x"88", x"a1", x"c0", x"e0", x"ff"
    );    

begin
  
  -- LATCHED, useful when emulating a real chip in circuit. Nasty as gated clock.
  --p_waddr                : process(reset_l, busctrl_addr)  
  process(clk)
  begin
	if clk'event and clk = '1' then
		if (RESET_L = '0') then
			addr <= (others => '0');
		elsif  busctrl_addr = '1' then -- yuk
			addr <= I_DA;
		end if;
	end if;
  end process;

  --p_wdata                : process(reset_l, busctrl_we, addr)
  process(clk)
  begin
	if clk'event and clk = '1' then
		if RESET_L = '0' then
			reg <= (others => (others => '0'));
			reg(7) <= x"ff";
      
		elsif busctrl_we = '1' then
			case addr(3 downto 0) is
				when x"0" => reg(0)  <= I_DA;
				when x"1" => reg(1)  <= I_DA;
				when x"2" => reg(2)  <= I_DA;
				when x"3" => reg(3)  <= I_DA;
				when x"4" => reg(4)  <= I_DA;
				when x"5" => reg(5)  <= I_DA;
				when x"6" => reg(6)  <= I_DA;
				when x"7" => reg(7)  <= I_DA;
				when x"8" => reg(8)  <= I_DA;
				when x"9" => reg(9)  <= I_DA;
				when x"A" => reg(10) <= I_DA;
				when x"B" => reg(11) <= I_DA;
				when x"C" => reg(12) <= I_DA;
				when x"D" => reg(13) <= I_DA;
				when x"E" => reg(14) <= I_DA;
				when x"F" => reg(15) <= I_DA;
				when others => null;
			end case;
		end if;
		
		env_reset <= '0';
		if busctrl_we = '1' and addr(3 downto 0) = x"D" then
		  env_reset <= '1';
		end if;
		
    end if;
  end process;

  --p_rdata                : process(busctrl_re, addr, reg)
  process(clk)
  begin
	if clk'event and clk = '1' then
		O_DA <= (others => '0'); -- 'X'
		if (busctrl_re = '1') then -- not necessary, but useful for putting 'X's in the simulator
		  case addr(3 downto 0) is
			when x"0" => O_DA <= reg(0) ;
			when x"1" => O_DA <= "0000" & reg(1)(3 downto 0) ;
			when x"2" => O_DA <= reg(2) ;
			when x"3" => O_DA <= "0000" & reg(3)(3 downto 0) ;
			when x"4" => O_DA <= reg(4) ;
			when x"5" => O_DA <= "0000" & reg(5)(3 downto 0) ;
			when x"6" => O_DA <= "000"  & reg(6)(4 downto 0) ;
			when x"7" => O_DA <= reg(7) ;
			when x"8" => O_DA <= "000"  & reg(8)(4 downto 0) ;
			when x"9" => O_DA <= "000"  & reg(9)(4 downto 0) ;
			when x"A" => O_DA <= "000"  & reg(10)(4 downto 0) ;
			when x"B" => O_DA <= reg(11);
			when x"C" => O_DA <= reg(12);
			when x"D" => O_DA <= "0000" & reg(13)(3 downto 0);
			when x"E" => if (reg(7)(6) = '0') then -- input
						   O_DA <= x"00";
						 else
						   O_DA <= reg(14); -- read output reg
						 end if;
			when x"F" => if (Reg(7)(7) = '0') then
						   O_DA <= x"00";
						 else
						   O_DA <= reg(15);
						 end if;
			when others => null;
		  end case;
		end if;
    end if;
  end process;

--  p_divider              : process
  process(clk,ena)
  begin
	if clk'event and clk = '1' and ENA = '1' then
      ena_div <= '0';
      ena_div_noise <= '0';
      if (cnt_div = "0000") then
        cnt_div <= (not I_SEL_L) & "111";
        ena_div <= '1';

        noise_div <= not noise_div;
        if (noise_div = '1') then
          ena_div_noise <= '1';
        end if;
      else
        cnt_div <= cnt_div - "1";
      end if;
    end if;
  end process;  

--  p_noise_gen            : process
  process(clk)
    variable noise_gen_comp : unsigned(4 downto 0);
    variable poly17_zero : std_logic;
  begin
	if clk'event and clk = '1' then
  
		if (reg(6)(4 downto 0) = "00000") then
		  noise_gen_comp := "00000";
		else
		  noise_gen_comp := unsigned( reg(6)(4 downto 0) ) - 1;
		end if;

		poly17_zero := '0';
		if (poly17 = "00000000000000000") then poly17_zero := '1'; end if;

		if (ENA = '1') then

		  if (ena_div_noise = '1') then -- divider ena

			if (noise_gen_cnt >= noise_gen_comp) then
			  noise_gen_cnt <= "00000";
			  poly17 <= (poly17(0) xor poly17(2) xor poly17_zero) & poly17(16 downto 1);
			else
			  noise_gen_cnt <= noise_gen_cnt + 1;
			end if;
		  end if;
		end if;
	end if;
  end process;
  
  noise_gen_op <= poly17(0);

  --p_tone_gens            : process
  process(clk)
    variable tone_gen_freq : array_3x12;
    variable tone_gen_comp : array_3x12;
  begin
	if clk'event and clk = '1' then
		-- looks like real chips count up - we need to get the Exact behaviour ..
		tone_gen_freq(1) := reg(1)(3 downto 0) & reg(0);
		tone_gen_freq(2) := reg(3)(3 downto 0) & reg(2);
		tone_gen_freq(3) := reg(5)(3 downto 0) & reg(4);
		
		-- period 0 = period 1
		for i in 1 to 3 loop
		  if (tone_gen_freq(i) = x"000") then
			tone_gen_comp(i) := x"000";
		  else
			tone_gen_comp(i) := std_logic_vector( unsigned(tone_gen_freq(i)) - 1 );
		  end if;
		end loop;

		if (ENA = '1') then
		  for i in 1 to 3 loop
			if (ena_div = '1') then -- divider ena

			  if (tone_gen_cnt(i) >= tone_gen_comp(i)) then
				tone_gen_cnt(i) <= x"000";
				tone_gen_op(i) <= not tone_gen_op(i);
			  else
				tone_gen_cnt(i) <= std_logic_vector( unsigned(tone_gen_cnt(i)) + 1 );
			  end if;
			end if;
		  end loop;
		end if;
	end if;
  end process;

  --p_envelope_freq        : process
  process(clk)
    variable env_gen_freq : std_logic_vector(15 downto 0);
    variable env_gen_comp : std_logic_vector(15 downto 0);
  begin
	if clk'event and clk = '1' then
	
		env_gen_freq := reg(12) & reg(11);
		-- envelope freqs 1 and 0 are the same.
		if (env_gen_freq = x"0000") then
		  env_gen_comp := x"0000";
		else
		  env_gen_comp := std_logic_vector( unsigned(env_gen_freq) - 1 );
		end if;

		if (ENA = '1') then
		  env_ena <= '0';
		  if (ena_div = '1') then -- divider ena
			if (env_gen_cnt >= env_gen_comp) then
			  env_gen_cnt <= x"0000";
			  env_ena <= '1';
			else
			  env_gen_cnt <= std_logic_vector( unsigned( env_gen_cnt ) + 1 );
			end if;
		  end if;
		end if;
		
	end if;
  end process;

  --p_envelope_shape       : process(env_reset, CLK)
  process(clk)
    variable is_bot    : boolean;
    variable is_bot_p1 : boolean;
    variable is_top_m1 : boolean;
    variable is_top    : boolean;
  begin
        -- envelope shapes
        -- C AtAlH
        -- 0 0 x x  \___
        --
        -- 0 1 x x  /___
        --
        -- 1 0 0 0  \\\\
        --
        -- 1 0 0 1  \___
        --
        -- 1 0 1 0  \/\/
        --           ___
        -- 1 0 1 1  \
        --
        -- 1 1 0 0  ////
        --           ___
        -- 1 1 0 1  /
        --
        -- 1 1 1 0  /\/\
        --
        -- 1 1 1 1  /___
	if clk'event and clk = '1' then
		if env_reset = '1' then
		  -- load initial state
		  if (reg(13)(2) = '0') then -- attack
			env_vol <= "11111";
			env_inc <= '0'; -- -1
		  else
			env_vol <= "00000";
			env_inc <= '1'; -- +1
		  end if;
		  env_hold <= '0';

		else
		  is_bot    := (env_vol = "00000");
		  is_bot_p1 := (env_vol = "00001");
		  is_top_m1 := (env_vol = "11110");
		  is_top    := (env_vol = "11111");

		  if (ENA = '1') then
			if (env_ena = '1') then
			  if (env_hold = '0') then
				if (env_inc = '1') then
				  env_vol <= std_logic_vector( unsigned( env_vol ) + "00001");
				else
				  env_vol <= std_logic_vector( unsigned( env_vol ) + "11111");
				end if;
			  end if;

			  -- envelope shape control.
			  if (reg(13)(3) = '0') then
				if (env_inc = '0') then -- down
				  if is_bot_p1 then env_hold <= '1'; end if;
				else
				  if is_top then env_hold <= '1'; end if;
				end if;
			  else
				if (reg(13)(0) = '1') then -- hold = 1
				  if (env_inc = '0') then -- down
					if (reg(13)(1) = '1') then -- alt
					  if is_bot    then env_hold <= '1'; end if;
					else
					  if is_bot_p1 then env_hold <= '1'; end if;
					end if;
				  else
					if (reg(13)(1) = '1') then -- alt
					  if is_top    then env_hold <= '1'; end if;
					else
					  if is_top_m1 then env_hold <= '1'; end if;
					end if;
				  end if;

				elsif (reg(13)(1) = '1') then -- alternate
				  if (env_inc = '0') then -- down
					if is_bot_p1 then env_hold <= '1'; end if;
					if is_bot    then env_hold <= '0'; env_inc <= '1'; end if;
				  else
					if is_top_m1 then env_hold <= '1'; end if;
					if is_top    then env_hold <= '0'; env_inc <= '0'; end if;
				  end if;
				end if;

			  end if;
			end if;
		  end if;
		end if;
	end if;
  end process;

  --p_chan_mixer_table     : process
  process(clk)
    variable chan_mixed : std_logic_vector(2 downto 0);
  begin
	if clk'event and clk = '1' then
		if (ENA = '1') then
		  chan_mixed(0) := (reg(7)(0) or tone_gen_op(1)) and (reg(7)(3) or noise_gen_op);
		  chan_mixed(1) := (reg(7)(1) or tone_gen_op(2)) and (reg(7)(4) or noise_gen_op);
		  chan_mixed(2) := (reg(7)(2) or tone_gen_op(3)) and (reg(7)(5) or noise_gen_op);

		  A <= "00000";
		  B <= "00000";
		  C <= "00000";

		  if (chan_mixed(0) = '1') then
			if (reg(8)(4) = '0') then
			  A <= reg(8)(3 downto 0) & "1";
			else
			  A <= env_vol(4 downto 0);
			end if;
		  end if;

		  if (chan_mixed(1) = '1') then
			if (reg(9)(4) = '0') then
			  B <= reg(9)(3 downto 0) & "1";
			else
			  B <= env_vol(4 downto 0);
			end if;
		  end if;

		  if (chan_mixed(2) = '1') then
			if (reg(10)(4) = '0') then
			  C <= reg(10)(3 downto 0) & "1";
			else
			  C <= env_vol(4 downto 0);
			end if;
		  end if;
		end if;
	end if;    
  end process;
  
  process(clk)
  variable out_audio_mixed : unsigned(9 downto 0);
  begin
	if clk'event and clk = '1' then
		if RESET_L = '0' then
		  O_AUDIO <= x"00";
		  O_AUDIO_A <= x"00";
		  O_AUDIO_B <= x"00";
		  O_AUDIO_C <= x"00";
		else
		  if(ctrl_aymode = '0') then
		   out_audio_mixed := 	("00" & volTableYm( to_integer( unsigned( A )))) + 
										("00" & volTableYm( to_integer( unsigned( B )))) + 
										("00" & volTableYm( to_integer( unsigned( C ))));
			O_AUDIO   <= std_logic_vector(out_audio_mixed(9 downto 2));
			O_AUDIO_A <= std_logic_vector( volTableYm( to_integer( unsigned( A ) ) ) );
			O_AUDIO_B <= std_logic_vector( volTableYm( to_integer( unsigned( B ) ) ) );
			O_AUDIO_C <= std_logic_vector( volTableYm( to_integer( unsigned( C ) ) ) );
		  else
		   out_audio_mixed := 	( "00" & volTableAy( to_integer( unsigned( A(4 downto 1) )))) + 
										( "00" & volTableAy( to_integer( unsigned( B(4 downto 1) )))) + 
										( "00" & volTableAy( to_integer( unsigned( C(4 downto 1) ))));
			O_AUDIO   <= std_logic_vector(out_audio_mixed(9 downto 2));
			O_AUDIO_A <= std_logic_vector( volTableAy( to_integer( unsigned( A(4 downto 1) ) ) ) );
			O_AUDIO_B <= std_logic_vector( volTableAy( to_integer( unsigned( B(4 downto 1) ) ) ) );
			O_AUDIO_C <= std_logic_vector( volTableAy( to_integer( unsigned( C(4 downto 1) ) ) ) );
		  end if;
		end if;
	end if;
  end process;
  
end architecture RTL;
