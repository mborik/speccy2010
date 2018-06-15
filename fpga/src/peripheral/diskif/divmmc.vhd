library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity divmmc is
port (
	CLK_I			: in std_logic;
	EN_I			: in std_logic;
	RESET_I			: in std_logic;
	ADDR_I			: in std_logic_vector(15 downto 0);
	DATA_I			: in std_logic_vector(7 downto 0);
	WR_N_I			: in std_logic;
	RD_N_I			: in std_logic;
	IORQ_N_I		: in std_logic;
	MREQ_N_I		: in std_logic;
	M1_N_I			: in std_logic;
	E3REG_O			: out std_logic_vector(7 downto 0);
	AMAP_O			: out std_logic;
	DISKIF_WAIT		: inout std_logic;
	DISKIF_WR		: inout std_logic;
end divmmc;

architecture rtl of divmmc is
	signal cs			: std_logic := '1';
	signal reg_e7		: std_logic := '0';
	signal automap		: std_logic := '0';
	signal detect		: std_logic := '0';
	signal reg_e3		: std_logic_vector(7 downto 0) := "00000000";
begin

process (RESET_I, CLK_I, WR_N_I, ADDR_I, IORQ_N_I, EN_I, DATA_I)
begin
	if (RESET_I = '1') then
		cs <= '1';
--		reg_e3(5 downto 0) <= (others => '0');
--		reg_e3(7) <= '0';
		reg_e3 <= (others => '0');
	elsif (CLK_I'event and CLK_I = '1') then
--		if (IORQ_N_I = '0' and WR_N_I = '0' and EN_I = '1' and ADDR_I(7 downto 0) = x"E3")	-- #E3
--			then reg_e3 <= DATA_I(7) & (reg_e3(6) or DATA_I(6)) & DATA_I(5 downto 0);
--		end if;
		if (IORQ_N_I = '0' and WR_N_I = '0' and EN_I = '1' and ADDR_I(7 downto 0) = x"E3")	-- #E3
			then reg_e3 <= DATA_I;
		end if;
		if (IORQ_N_I = '0' and WR_N_I = '0' and EN_I = '1' and ADDR_I(7 downto 0) = x"E7")	-- #E7
			then cs <= DATA_I(0);
		end if;
		if (IORQ_N_I = '0' and EN_I = '1' and ADDR_I(7 downto 0) = x"EB") then	-- #EB
			DISKIF_WAIT <= '1';
			DISKIF_WR <= not WR_N_I;
		end if;
	end if;
end process;

process (CLK_I, M1_N_I, MREQ_N_I, ADDR_I, EN_I, detect, automap)
begin
	if (CLK_I'event and CLK_I = '1') then
		if (M1_N_I = '0' and MREQ_N_I = '0' and EN_I = '1' and
			(ADDR_I = x"0000" or ADDR_I = x"0008" or ADDR_I = x"0038" or
			ADDR_I = x"0066" or ADDR_I = x"04C6" or ADDR_I = x"0562" or ADDR_I(15 downto 8) = x"3D"))
			then detect <= '1'; -- activated when the command code is extracted in the M1 cycle on specified entry points
		elsif (M1_N_I = '0' and MREQ_N_I = '0' and EN_I = '1' and ADDR_I(15 downto 3) = "0001111111111")
			then detect <= '0'; -- activated when the command code is extracted in the M1 cycle on 0x1FF8-0x1FFF
		end if;
		if (M1_N_I = '0' and IORQ_N_I = '1' and EN_I = '1' and ADDR_I(15 downto 8) = x"3D")
			then automap <= '1'; -- instant paging without waiting for an opcode to read
		elsif (MREQ_N_I = '0' and EN_I = '1' and WR_N_I = '1' and RD_N_I = '1')
			then automap <= detect; -- paging after reading the opcode
		end if;
	end if;
end process;

E3REG_O <= reg_e3;
AMAP_O  <= automap;

end rtl;
