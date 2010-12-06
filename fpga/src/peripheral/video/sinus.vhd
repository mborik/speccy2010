library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity sinus is
  port(
    phase : in std_logic_vector(7 downto 0);
    sinus : out std_logic_vector(15 downto 0)
    );
end sinus;

architecture rtl of sinus is

  signal addr : unsigned(7 downto 0);
  signal temp : signed(15 downto 0);

  type tableSinusType is array (0 to 64) of signed(15 downto 0);
  constant tableSinus : tableSinusType :=(
		x"0000", x"025b", x"04b6", x"0710", x"0969", x"0bc0", x"0e16", x"106a",
		x"12bb", x"1509", x"1753", x"199b", x"1bde", x"1e1d", x"2057", x"228d",
		x"24bd", x"26e7", x"290c", x"2b2a", x"2d41", x"2f51", x"315b", x"335c",
		x"3556", x"3747", x"3930", x"3b10", x"3ce7", x"3eb4", x"4078", x"4232",
		x"43e2", x"4587", x"4722", x"48b1", x"4a36", x"4bae", x"4d1c", x"4e7d",
		x"4fd2", x"511b", x"5258", x"5387", x"54aa", x"55c0", x"56c8", x"57c4",
		x"58b1", x"5991", x"5a63", x"5b28", x"5bde", x"5c86", x"5d1f", x"5dab",
		x"5e28", x"5e96", x"5ef6", x"5f47", x"5f8a", x"5fbd", x"5fe2", x"5ff9", x"6000"
		
--		x"0000", x"0192", x"0324", x"04b5", x"0646", x"07d6", x"0964", x"0af1",
--		x"0c7c", x"0e06", x"0f8d", x"1112", x"1294", x"1413", x"1590", x"1709",
--		x"187e", x"19ef", x"1b5d", x"1cc6", x"1e2b", x"1f8c", x"20e7", x"223d",
--		x"238e", x"24da", x"2620", x"2760", x"289a", x"29ce", x"2afb", x"2c21",
--		x"2d41", x"2e5a", x"2f6c", x"3076", x"3179", x"3274", x"3368", x"3453",
--		x"3537", x"3612", x"36e5", x"37b0", x"3871", x"392b", x"39db", x"3a82",
--		x"3b21", x"3bb6", x"3c42", x"3cc5", x"3d3f", x"3daf", x"3e15", x"3e72",
--		x"3ec5", x"3f0f", x"3f4f", x"3f85", x"3fb1", x"3fd4", x"3fec", x"3ffb", x"4000"
    );  

begin

  addr <= unsigned( "00" & phase(5 downto 0) ) when phase(6) = '0' else
			x"40" - unsigned( "00" & phase(5 downto 0) );

  temp <= tableSinus( to_integer( addr ) ) when phase(7) = '0' else
			-tableSinus( to_integer( addr ) );

  sinus <= std_logic_vector( temp );

end rtl;
