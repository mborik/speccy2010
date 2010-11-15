library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sdram is
	generic(
		freq : integer := 84
	);

	port(
		clk			 : in std_logic;		
		reset		 : in std_logic;
		refresh		 : in std_logic;
		
		memAddress  : in std_logic_vector(23 downto 0);
		memDataIn   : in std_logic_vector(15 downto 0);
		memDataOut  : out std_logic_vector(15 downto 0);
		memDataMask : in std_logic_vector(1 downto 0);
		memWr       : in std_logic;
		memReq      : in std_logic;
		memAck      : out std_logic := '0';

		memAddress2  : in std_logic_vector(23 downto 0);
		memDataIn2   : in std_logic_vector(15 downto 0);
		memDataOut2  : out std_logic_vector(15 downto 0);
		memDataMask2 : in std_logic_vector(1 downto 0);
		memWr2       : in std_logic;
		memReq2      : in std_logic;
		memAck2      : out std_logic := '0';

		-- SD-RAM ports
		pMemClk     : out std_logic;                        -- SD-RAM Clock
		pMemCke     : out std_logic;                        -- SD-RAM Clock enable
		pMemCs_n    : out std_logic;                        -- SD-RAM Chip select
		pMemRas_n   : out std_logic;                        -- SD-RAM Row/RAS
		pMemCas_n   : out std_logic;                        -- SD-RAM /CAS
		pMemWe_n    : out std_logic;                        -- SD-RAM /WE
		pMemUdq     : out std_logic;                        -- SD-RAM UDQM
		pMemLdq     : out std_logic;                        -- SD-RAM LDQM
		pMemBa1     : out std_logic;                        -- SD-RAM Bank select address 1
		pMemBa0     : out std_logic;                        -- SD-RAM Bank select address 0
		pMemAdr     : out std_logic_vector(12 downto 0);    -- SD-RAM Address
		pMemDat     : inout std_logic_vector(15 downto 0)   -- SD-RAM Data
	);
  
end sdram;

architecture rtl of sdram is

--	-- SD-RAM control signals
	signal SdrCmd      : std_logic_vector(3 downto 0);
	signal SdrBa0      : std_logic;
	signal SdrBa1      : std_logic;
	signal SdrUdq      : std_logic;
	signal SdrLdq      : std_logic;
	signal SdrAdr      : std_logic_vector(12 downto 0);
	signal SdrDat      : std_logic_vector(15 downto 0);
	
	signal iMemAck		: std_logic;
	signal iMemAck2		: std_logic;

	constant SdrCmd_de : std_logic_vector(3 downto 0) := "1111"; -- deselect
	constant SdrCmd_pr : std_logic_vector(3 downto 0) := "0010"; -- precharge all
	constant SdrCmd_re : std_logic_vector(3 downto 0) := "0001"; -- refresh
	constant SdrCmd_ms : std_logic_vector(3 downto 0) := "0000"; -- mode regiser set
	constant SdrCmd_xx : std_logic_vector(3 downto 0) := "0111"; -- no operation
	constant SdrCmd_ac : std_logic_vector(3 downto 0) := "0011"; -- activate
	constant SdrCmd_rd : std_logic_vector(3 downto 0) := "0101"; -- read
	constant SdrCmd_wr : std_logic_vector(3 downto 0) := "0100"; -- write		

begin

	memAck <= iMemAck;
	memAck2 <= iMemAck2;

	process( clk )

		type typSdrRoutine is ( SdrRoutine_Null, SdrRoutine_Init, SdrRoutine_Idle, SdrRoutine_RefreshAll, SdrRoutine_ReadOne, SdrRoutine_WriteOne );
		variable SdrRoutine : typSdrRoutine := SdrRoutine_Null;
		variable SdrRoutineSeq : unsigned(7 downto 0) := X"00";

		variable refreshDelayCounter : unsigned(23 downto 0) := x"000000";
		variable SdrRefreshCounter : unsigned(15 downto 0) := X"0000";
		
		variable SdrPort : std_logic := '0';
		variable SdrAddress : std_logic_vector(23 downto 0);
		
	begin
	
		if clk'event and clk = '1' then
			
			iMemAck <= '0';
			iMemAck2 <= '0';

			case SdrRoutine is
					
				when SdrRoutine_Null =>
					SdrCmd <= SdrCmd_xx;
					SdrDat <= (others => 'Z');
					
					if refreshDelayCounter = 0 then
						SdrRoutine := SdrRoutine_Init;
					end if;
					
				when SdrRoutine_Init =>
					if( SdrRoutineSeq = X"0" ) then
						SdrCmd <= SdrCmd_pr;
						SdrAdr <= "1111111111111";
						SdrBa1 <= '0';
						SdrBa0 <= '0';
						SdrUdq <= '1';
						SdrLdq <= '1';
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"4" or SdrRoutineSeq = X"0C" ) then
						SdrCmd <= SdrCmd_re;
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"14" ) then
						SdrCmd <= SdrCmd_ms;
						SdrAdr <= "00010" & "0" & "010" & "0" & "000";				
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"17" ) then
						SdrCmd <= SdrCmd_xx;
						SdrRoutineSeq := X"00";
						SdrRoutine := SdrRoutine_Idle;
					else
						SdrCmd <= SdrCmd_xx;
						SdrRoutineSeq := SdrRoutineSeq + 1;
					end if;

				when SdrRoutine_Idle =>
					SdrCmd <= SdrCmd_xx;
					SdrDat <= (others => 'Z');
					
					if memReq = '1' and iMemAck = '0' then
						SdrPort := '0';
						SdrAddress := memAddress;
						
						if( memWr = '1' ) then
							SdrRoutine := SdrRoutine_WriteOne;
						else
							SdrRoutine := SdrRoutine_ReadOne;
						end if;

					elsif memReq2 = '1' and iMemAck2 = '0' then
						SdrPort := '1';
						SdrAddress := memAddress2;
						
						if( memWr2 = '1' ) then
							SdrRoutine := SdrRoutine_WriteOne;
						else
							SdrRoutine := SdrRoutine_ReadOne;
						end if;							


					elsif SdrRefreshCounter < 4096 and refresh = '1' then
						SdrRoutine := SdrRoutine_RefreshAll;
						SdrRefreshCounter := SdrRefreshCounter + 1;
					end if;
				
				when SdrRoutine_RefreshAll =>	
					if( SdrRoutineSeq = X"0" ) then
						SdrCmd <= SdrCmd_re;
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"6" ) then
						SdrCmd <= SdrCmd_xx;
						SdrRoutineSeq := X"00";
						SdrRoutine := SdrRoutine_Idle;
					else
						SdrCmd <= SdrCmd_xx;
						SdrRoutineSeq := SdrRoutineSeq + 1;
					end if;
				
				when SdrRoutine_ReadOne =>	
					if( SdrRoutineSeq = X"0" ) then
						SdrCmd <= SdrCmd_ac;
						SdrBa0 <= SdrAddress(21);
						SdrBa1 <= SdrAddress(22);
						SdrAdr <= SdrAddress(23) & SdrAddress(20 downto 9);
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"2" ) then
						SdrCmd <= SdrCmd_rd;
						SdrUdq <= '0';
						SdrLdq <= '0';
						SdrAdr(12 downto 9) <= "0010";
						SdrAdr(8 downto 0) <= SdrAddress(8 downto 0);
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"5" ) then
						if( SdrPort = '0' ) then
							memDataOut <= pMemDat;
							iMemAck <= '1';
						else
							memDataOut2 <= pMemDat;
							iMemAck2 <= '1';
						end if;
						SdrCmd <= SdrCmd_xx;
						SdrRoutineSeq := SdrRoutineSeq + 1;					
					elsif( SdrRoutineSeq = X"6" ) then
						SdrRoutineSeq := X"00";
						SdrRoutine := SdrRoutine_Idle;
					else
						SdrCmd <= SdrCmd_xx;
						SdrRoutineSeq := SdrRoutineSeq + 1;					
					end if;
					
				when SdrRoutine_WriteOne =>	
					if( SdrRoutineSeq = X"0" ) then
						SdrCmd <= SdrCmd_ac;
						SdrBa0 <= SdrAddress(21);
						SdrBa1 <= SdrAddress(22);
						SdrAdr <= SdrAddress(23) & SdrAddress(20 downto 9);
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"2" ) then
						SdrCmd <= SdrCmd_wr;
						SdrAdr(12 downto 9) <= "0010";
						SdrAdr(8 downto 0) <= SdrAddress(8 downto 0);
						if( SdrPort = '0' ) then
							SdrDat <= memDataIn;
							SdrLdq <= not memDataMask(0);
							SdrUdq <= not memDataMask(1);
						else
							SdrDat <= memDataIn2;
							SdrLdq <= not memDataMask2(0);
							SdrUdq <= not memDataMask2(1);
						end if;
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"3" ) then
						if( SdrPort = '0' ) then
							iMemAck <= '1';
						else
							iMemAck2 <= '1';
						end if;
						SdrCmd <= SdrCmd_xx;
						SdrDat <= (others => 'Z');
						SdrRoutineSeq := SdrRoutineSeq + 1;
					elsif( SdrRoutineSeq = X"5" ) then						
						SdrRoutineSeq := X"00";
						SdrRoutine := SdrRoutine_Idle;
					else
						SdrCmd <= SdrCmd_xx;
						SdrRoutineSeq := SdrRoutineSeq + 1;					
					end if;
					
			end case;
			
			refreshDelayCounter := refreshDelayCounter + 1;
			
			if( refreshDelayCounter >= ( freq * 1000 * 64 ) ) then
				refreshDelayCounter := x"000000";
				SdrRefreshCounter := x"0000";
			end if;
			
		end if;

	end process;	
	
	pMemClk   <= clk;
	pMemCke   <= '1';

	pMemCs_n  <= SdrCmd(3);
	pMemRas_n <= SdrCmd(2);
	pMemCas_n <= SdrCmd(1);
	pMemWe_n  <= SdrCmd(0);

	pMemUdq   <= SdrUdq;
	pMemLdq   <= SdrLdq;
	pMemBa1   <= SdrBa1;
	pMemBa0   <= SdrBa0;

	pMemAdr   <= SdrAdr;
	pMemDat   <= SdrDat;

end rtl;
