library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity fir is
	generic (
		TAPS : integer := 3
	);
		
	port (
		clk : in std_logic;
		clk_en : in std_logic;
		reset : in std_logic;
		input : in std_logic_vector(8 downto 0);
		output : out std_logic_vector(8 downto 0)
	);
end fir;

architecture behaviour of fir is
begin
	process (clk, reset)
	type tsr is array(0 to TAPS-1) of signed(8 downto 0);
	variable sr : tsr;
	variable y : signed (63 downto 0);
	begin
		if reset='1' then
			for I in 0 to TAPS-1 loop
				sr(I) := (others => '0');
			end loop;
			output <= (others => '0');
		elsif clk'event and clk='1' and	clk_en = '1' then
			for i in (TAPS-1) downto 1 loop
				sr( i ) := sr( i - 1 );
			end loop;
			
			sr(0) := signed(input);
			y := (others => '0');
			
			for i in 0 to TAPS - 1 loop
				y := y + sr(i);
			end loop;
			
			y := y / TAPS;
			output <= std_logic_vector( y( 8 downto 0 ) );
			
		end if;
	end process;

end architecture;
