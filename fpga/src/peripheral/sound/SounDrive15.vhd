library ieee;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity SounDrive is
  port (

  CLK                 : in  std_logic; 
  I_DA                : in  std_logic_vector(7 downto 0);

  busctrl_we          : std_logic;
  bus_addr			  : std_logic_vector(15 downto 0);
  covox_mode          : unsigned(2 downto 0);

  O_AUDIO_L1           : out std_logic_vector(7 downto 0);
  O_AUDIO_L2           : out std_logic_vector(7 downto 0);
  O_AUDIO_R1           : out std_logic_vector(7 downto 0);
  O_AUDIO_R2           : out std_logic_vector(7 downto 0)
  );
end;

architecture RTL of SounDrive is

  -- registers
  signal reg_0                  : std_logic_vector(7 downto 0);
  signal reg_1                  : std_logic_vector(7 downto 0);
  signal reg_2                  : std_logic_vector(7 downto 0);
  signal reg_3                  : std_logic_vector(7 downto 0);

begin

  --p_wdata                : process(busctrl_we, sd_mode)
  process(clk)
  begin
	if clk'event and clk = '1' then
		if busctrl_we = '1' then
			case '0' & covox_mode is --no covox
				when x"0" => reg_0 <= x"00";
							 reg_1 <= x"00";
							 reg_2 <= x"00";
							 reg_3 <= x"00";
				when x"1" => 
					case bus_addr(7 downto 0) is --sd 1
						when x"0F" => reg_0  <= I_DA;
						when x"1F" => reg_1  <= I_DA;
						when x"4F" => reg_2  <= I_DA;
						when x"5F" => reg_3  <= I_DA;
						when others => null;
					end case;
				when x"2" => 
					case bus_addr(7 downto 0) is --sd 2
						when x"F1" => reg_0  <= I_DA;
						when x"F3" => reg_1  <= I_DA;
						when x"F9" => reg_2  <= I_DA;
						when x"FB" => reg_3  <= I_DA;
						when others => null;
					end case;
				when x"3" => 
					if bus_addr(7 downto 0) = x"B3"	then --GS
					    reg_0  <= I_DA;
					    reg_1  <= I_DA;
					    reg_2  <= I_DA;
					    reg_3  <= I_DA;
					end if;
				when x"4" => 
					if bus_addr(7 downto 0) = x"FB"	then --Pentagon
					    reg_0  <= I_DA;
					    reg_1  <= I_DA;
					    reg_2  <= I_DA;
					    reg_3  <= I_DA;
					end if;
				when x"5" => 
					if bus_addr(7 downto 0) = x"DD"	then --Scorpion
					    reg_0  <= I_DA;
					    reg_1  <= I_DA;
					    reg_2  <= I_DA;
					    reg_3  <= I_DA;
					end if;
				when x"6" => 
					if bus_addr(7) = '0'	then --Profi
						if bus_addr(6 downto 5) = "01" then
							reg_0  <= I_DA;
							reg_1  <= I_DA;
						elsif bus_addr(6 downto 5) = "10" then 
							reg_2  <= I_DA;
							reg_3  <= I_DA;
						end if;
					end if;
				when others => null;
			end case;
		end if;
	end if;
  end process;

  process(clk)
  begin
	if clk'event and clk = '1' then
		O_AUDIO_L1 <= reg_0;
		O_AUDIO_L2 <= reg_1;
		O_AUDIO_R1 <= reg_2;
		O_AUDIO_R2 <= reg_3;
	end if;
  end process;


end architecture RTL;
