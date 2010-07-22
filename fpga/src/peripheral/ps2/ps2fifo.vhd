library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity ps2fifo is
	
	port (
		clk_i 		: in std_logic;   -- Global clk
		rst_i 		: in std_logic;   -- GLobal Asinchronous reset

		data_o		: out std_logic_vector(7 downto 0);  -- Data in
		data_i		: in  std_logic_vector(7 downto 0);  -- Data out
		
		rd_i		: in  std_logic;
		wr_i		: in  std_logic;
		
		out_ready_o	: out std_logic;
		out_full_o	: out std_logic;		
		in_full_o 	: out std_logic;

		ps2_clk_io  : inout std_logic;   -- PS2 Clock line
		ps2_data_io : inout std_logic
	);
	
end entity;

architecture rtl of ps2fifo is

	signal read_data_o			: std_logic_vector(7 downto 0);
	signal read_data_i			: std_logic_vector(7 downto 0);
	signal read_wr_i			: std_logic;
	signal read_rd_i			: std_logic;
	signal read_out_ready_o		: std_logic;
	signal read_in_full_o		: std_logic;
	
	signal write_data_o			: std_logic_vector(7 downto 0);
	signal write_data_i			: std_logic_vector(7 downto 0);
	signal write_wr_i			: std_logic;
	signal write_rd_i			: std_logic;
	signal write_out_ready_o	: std_logic;
	signal write_in_full_o		: std_logic;
	
	signal ps2_data_o    : std_logic_vector(7 downto 0);  -- Data in
	signal ps2_data_i    : std_logic_vector(7 downto 0);  -- Data out
	signal ps2_ibf_clr_i : std_logic;  -- Ifb flag clear input
	signal ps2_obf_set_i : std_logic;  -- Obf flag set input
	signal ps2_ibf_o     : std_logic;  -- Received data available
	signal ps2_obf_o     : std_logic;  -- Data ready to sent

	signal ps2_frame_err_o  : std_logic;  -- Error receiving data
	signal ps2_parity_err_o : std_logic;  -- Error in received data parity
	signal ps2_busy_o       : std_logic;  -- uart busy
	signal ps2_err_clr_i 	: std_logic;  -- Clear error flags

	component fifo is

		generic (
			fifo_width : integer;
			fifo_depth : integer
		);
		
		port (
			clk_i 		: in std_logic;   -- Global clk
			rst_i 		: in std_logic;   -- GLobal Asinchronous reset

			data_o    	: out std_logic_vector( fifo_width - 1 downto 0 );
			data_i    	: in  std_logic_vector( fifo_width - 1 downto 0 );

			wr_i 		: in  std_logic;
			rd_i 		: in  std_logic;

			out_ready_o	: out std_logic;
			in_full_o	: out std_logic
		);
		
	end component;	

	component ps2 is
        port (
			clk_i : in std_logic;   -- Global clk
			rst_i : in std_logic;   -- GLobal Asinchronous reset

			data_o    : out std_logic_vector(7 downto 0);  -- Data in
			data_i    : in  std_logic_vector(7 downto 0);  -- Data out
			ibf_clr_i : in  std_logic;  -- Ifb flag clear input
			obf_set_i : in  std_logic;  -- Obf flag set input
			ibf_o     : out std_logic;  -- Received data available
			obf_o     : out std_logic;  -- Data ready to sent

			frame_err_o  : out std_logic;  -- Error receiving data
			parity_err_o : out std_logic;  -- Error in received data parity
			busy_o       : out std_logic;  -- uart busy
			err_clr_i 	: in std_logic;  -- Clear error flags

			wdt_o 		: out std_logic;  -- Watchdog timer out every 400uS

			ps2_clk_io  : inout std_logic;   -- PS2 Clock line
			ps2_data_io : inout std_logic);  -- PS2 Data line
	end component;
	
begin

	U00 : ps2
		port map(
			clk_i => clk_i,
			rst_i => not rst_i,

			data_o => ps2_data_o,
			data_i => ps2_data_i,
			ibf_clr_i => ps2_ibf_clr_i,
			obf_set_i => ps2_obf_set_i,
			ibf_o => ps2_ibf_o,
			obf_o => ps2_obf_o,

			frame_err_o => ps2_frame_err_o,
			parity_err_o => ps2_parity_err_o,
			busy_o => ps2_busy_o,
			err_clr_i => ps2_err_clr_i,

			wdt_o => open,

			ps2_clk_io => ps2_clk_io,
			ps2_data_io => ps2_data_io
		);				

	U01 : fifo
		generic map(
			fifo_width => 8,
			fifo_depth => 32 
		)
		
		port map(
			clk_i => clk_i,
			rst_i => rst_i,

			data_o => read_data_o,
			data_i => read_data_i,

			wr_i => read_wr_i,
			rd_i => read_rd_i,

			out_ready_o => read_out_ready_o,
			in_full_o => read_in_full_o
		);
		
	U02 : fifo
		generic map(
			fifo_width => 8,
			fifo_depth => 8 
		)
		
		port map(
			clk_i => clk_i,
			rst_i => rst_i,

			data_o => write_data_o,
			data_i => write_data_i,

			wr_i => write_wr_i,
			rd_i => write_rd_i,

			out_ready_o => write_out_ready_o,
			in_full_o => write_in_full_o
		);
				
	process (clk_i, rst_i)
	
		--variable cnt : unsigned( 7 downto 0 ) := x"00";
		
	begin
	
		if rst_i = '1' then			
			
		elsif clk_i'event and clk_i = '1' then
			
			ps2_ibf_clr_i <= '0';
			ps2_obf_set_i <= '0';
			ps2_err_clr_i <= '0';
			
			read_wr_i <= '0';
			write_rd_i <= '0';

			if ps2_ibf_o = '1' and ps2_ibf_clr_i = '0' then
				
				read_data_i <= ps2_data_o;
				read_wr_i <= '1';
				
				ps2_ibf_clr_i <= '1';				
			
			end if;
			
			if ps2_busy_o = '0' and ps2_obf_set_i = '0' and write_out_ready_o = '1' then
			
				ps2_data_i <= write_data_o;
				write_rd_i <= '1';
				
				ps2_obf_set_i <= '1';
				
			end if;

			if ( ps2_frame_err_o = '1' or ps2_parity_err_o = '1' ) and ps2_err_clr_i = '0' then
			
				ps2_err_clr_i <= '1';
				
			end if;
				
		end if;
	
	end process;
	
	data_o <= read_data_o;	
	read_rd_i <= rd_i;
	out_ready_o <= read_out_ready_o;
	out_full_o <= read_in_full_o;
	
	write_data_i <= data_i;
	write_wr_i <= wr_i;
	in_full_o <= write_in_full_o;

end rtl;

