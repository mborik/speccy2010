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

begin
		
	process (clk_i, rst_i)

		type buffType is array( 0 to fifo_depth - 1 ) of std_logic_vector( fifo_width - 1 downto 0 );
		variable buff : buffType;
		
		variable readPtr : integer := 0;
		variable writePtr : integer := 0;
		variable dataSize : integer := 0;
	
	begin
	
		if clk_i'event and clk_i = '1' then
		
			if rst_i = '1' then
				
				out_ready_o <= '0';
				readPtr  := 0;
				writePtr := 0;
				dataSize := 0;
			
			else
			
				data_o <= buff( readPtr );

				if dataSize < fifo_depth then
					in_full_o <= '0';
				else 
					in_full_o <= '1';
				end if;
				
				if wr_i = '1' and dataSize < fifo_depth then
					
					buff( writePtr ) := data_i;
					
					writePtr := writePtr + 1;
					if writePtr = fifo_depth then
						writePtr := 0;
					end if;
					
					if dataSize = 0 then							
						data_o <= data_i;
						out_ready_o <= '1';
					end if;
					
				end if;
				
				if rd_i = '1' and dataSize > 0 then
					
					readPtr := readPtr + 1;				
					if readPtr = fifo_depth then
						readPtr := 0;
					end if;
					
					if dataSize = 1 then
						out_ready_o <= '0';
					end if;				
					
				end if;
				
				if wr_i = '1' and rd_i = '0' then
					dataSize := dataSize + 1;
				elsif wr_i = '0' and rd_i = '1' then
					dataSize := dataSize - 1;
				end if;

			end if;
			
		end if;
			
	end process;

end rtl;

