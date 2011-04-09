library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity fifo is

	generic (
		fifo_width : integer := 8;
		fifo_depth : integer := 16
	);
	
	port (
		clk_i 		: in std_logic;
		rst_i 		: in std_logic;

		data_o    	: out std_logic_vector( fifo_width - 1 downto 0 );
		data_i    	: in  std_logic_vector( fifo_width - 1 downto 0 );

		wr_i 		: in  std_logic;
		rd_i 		: in  std_logic;

		out_ready_o	: out std_logic;
		in_full_o	: out std_logic
	);
	
end fifo;

architecture rtl of fifo is

	signal out_ready	: std_logic := '0';
	signal in_full		: std_logic := '0';
	
begin
		
	process (clk_i, rst_i)

		type buffType is array( 0 to fifo_depth - 1 ) of std_logic_vector( fifo_width - 1 downto 0 );
		variable buff : buffType;
		
		subtype small_int is integer range 0 to fifo_depth - 1;		
		variable readPtr : small_int := 0;
		variable writePtr : small_int := 0;
	
	begin
	
		if clk_i'event and clk_i = '1' then
		
			if rst_i = '1' then
				
				out_ready <= '0';
				in_full <= '0';

				readPtr  := 0;
				writePtr := 0;
			
			else
			
				data_o <= buff( readPtr );				
				out_ready_o <= out_ready;
				
				if wr_i = '1' and in_full = '0' then
					
					buff( writePtr ) := data_i;					
					writePtr := writePtr + 1;					
					
					out_ready <= '1';
					if writePtr = readPtr then
						in_full <= '1';
					end if;
					
				end if;
				
				if rd_i = '1' and out_ready = '1' then
					
					readPtr := readPtr + 1;					
					out_ready_o <= '0';
					
					in_full <= '0';					
					if writePtr = readPtr then
						out_ready <= '0';
					end if;
					
				end if;
				
				in_full_o <= in_full;
				
			end if;
			
		end if;
			
	end process;

end rtl;

