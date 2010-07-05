library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity speccy2010_top is
	port(
		CLK_20			: in std_logic;
		
		SOUND_LEFT		: out std_logic_vector(7 downto 0) := "10000000";
		SOUND_RIGHT		: out std_logic_vector(7 downto 0) := "10000000";
		
		VIDEO_R			: out std_logic_vector(7 downto 0) := "01111111";
		VIDEO_G			: out std_logic_vector(7 downto 0) := "01111111";
		VIDEO_B			: out std_logic_vector(7 downto 0) := "01111111";
		
		VIDEO_HSYNC		: out std_logic := '1';
		VIDEO_VSYNC		: out std_logic := '1';
		
		ARM_AD			: inout std_logic_vector(15 downto 0) := (others => 'Z');
		ARM_A			: in std_logic_vector(23 downto 16);
		
		ARM_RD			: in std_logic;
		ARM_WR			: in std_logic;
		ARM_ALE			: in std_logic;
		ARM_WAIT		: out std_logic := '0';
		
		JOY0			: in std_logic_vector(5 downto 0);
		JOY0_SEL  		: out std_logic := '1';
		JOY1			: in std_logic_vector(5 downto 0);
		JOY1_SEL  		: out std_logic := '1';
		
		KEYS_CLK		: inout std_logic := 'Z';
		KEYS_DATA		: inout std_logic := 'Z';

		MOUSE_CLK		: inout std_logic := 'Z';
		MOUSE_DATA		: inout std_logic := 'Z';

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
		pMemDat     : inout std_logic_vector(15 downto 0);  -- SD-RAM Data
		
		RESET_n			: in std_logic
	);
  
end speccy2010_top;

architecture rtl of speccy2010_top is

	constant freq : integer := 84;
	
	signal memclk : std_logic;
	signal reset : std_logic;

	signal clk28m : std_logic;
	signal clk14m : std_logic;
	signal clk7m : std_logic;
	signal clk35m : std_logic;

    signal counter : unsigned(31 downto 0) := x"00000000";    

    signal cpuHaltReq : std_logic := '0';
    signal cpuHaltAck : std_logic := '0';
	signal cpuTraceReq : std_logic := '0';
	signal cpuTraceAck : std_logic := '0';
	signal cpuOneCycleWaitReq : std_logic := '0';
	signal cpuOneCycleWaitAck : std_logic := '0';
	
	signal cpuBP0 	: std_logic_vector(15 downto 0) := x"0000";
	signal cpuBP0_en : std_logic := '0';

    signal cpuTurbo : std_logic_vector( 1 downto 0 ) := "00";
    
    signal activeBank : unsigned( 1 downto 0 ) := "00";
	signal armVideoPage	: std_logic_vector(15 downto 0) := x"0000";
	signal armBorder	: std_logic_vector(15 downto 0) := x"0000";
	
	signal dendyDat : std_logic;
	signal dendyCodePrev : std_logic_vector(15 downto 0) := "1111111111111111";
	signal dendyCode : std_logic_vector(15 downto 0) := "1111111111111111";
	
	signal keysDataOut : std_logic_vector(7 downto 0);
	signal keysDataIn : std_logic_vector(7 downto 0);
	
	signal keysRd : std_logic := '0';
	signal keysWr : std_logic := '0';
	
	signal keysReady : std_logic;
	signal keysFull : std_logic;
	
	signal mouseDataOut : std_logic_vector(7 downto 0);
	signal mouseDataIn : std_logic_vector(7 downto 0);
	
	signal mouseRd : std_logic := '0';
	signal mouseWr : std_logic := '0';
	
	signal mouseReady : std_logic;
	signal mouseFull : std_logic;
	
	signal joyCode0 : std_logic_vector(15 downto 0) := x"0000";
	signal joyCode1 : std_logic_vector(15 downto 0) := x"0000";

--	-- SD-RAM control signals
	signal memAddress  : std_logic_vector(23 downto 0);
	signal memDataIn   : std_logic_vector(15 downto 0);
	signal memDataOut  : std_logic_vector(15 downto 0);
	signal memDataMask : std_logic_vector(1 downto 0);
	signal memWr       : std_logic := '0';
	signal memReq      : std_logic := '0';
	signal memAck      : std_logic := '0';

	signal memAddress2  : std_logic_vector(23 downto 0);
	signal memDataIn2   : std_logic_vector(15 downto 0);
	signal memDataOut2  : std_logic_vector(15 downto 0);
	signal memDataMask2 : std_logic_vector(1 downto 0);
	signal memWr2       : std_logic := '0';
	signal memReq2      : std_logic := '0';
	signal memAck2      : std_logic := '0';

	signal Invert		: unsigned(3 downto 0) := "0000";

	signal ChrC_Cnt		: unsigned(2 downto 0) := "000";	-- Character column counter
	signal ChrR_Cnt		: unsigned(2 downto 0) := "000";	-- Character row counter
	signal Hor_Cnt		: unsigned(5 downto 0) := "000000";	-- Horizontal counter
	signal Ver_Cnt		: unsigned(5 downto 0) := "000000";	-- Vertical counter
	
	signal specIntCounter : unsigned(31 downto 0) := x"00000000";
    signal cpuMemoryWait : std_logic := '0';

	signal specPortFe	: std_logic_vector(7 downto 0);
	signal specPort7ffd	: std_logic_vector(7 downto 0);
	
	signal specTrdosToggleFlag : std_logic := '0';
	signal specTrdosFlag : std_logic := '0';
	signal specTrdosWait : std_logic := '0';
	signal specTrdosWr : std_logic := '0';
	signal specTrdosCounter	: unsigned(15 downto 0) := x"0000";
	signal specTrdosStat	: std_logic_vector(7 downto 0) := x"00";
	
	signal specMode		: unsigned(15 downto 0) := x"0000";
	signal syncMode		: unsigned(15 downto 0) := x"0000";
	signal videoMode	: unsigned(15 downto 0) := x"0000";
    signal subcarrierDelta : unsigned( 23 downto 0 ) := to_unsigned( 885662, 24 ); -- 885521
	signal dacMode		: unsigned(15 downto 0) := x"0000";
	signal ayMode		: unsigned(15 downto 0) := x"0000";
	
	signal borderAttr	: std_logic_vector(2 downto 0);
	signal speaker 		: std_logic;
	
	signal tapeIn 		: std_logic;
	signal keyboard		: std_logic_vector(39 downto 0) := "1111111111111111111111111111111111111111";
	signal joystick		: std_logic_vector(7 downto 0) := x"00";
	signal sound 		: std_logic_vector(15 downto 0) := x"0000";
	
	signal soundLeft	: std_logic_vector(15 downto 0) := x"0000";
	signal soundRight	: std_logic_vector(15 downto 0) := x"0000";
	
	signal romPage		: std_logic_vector(2 downto 0);
	signal ramPage		: std_logic_vector(7 downto 0);
	signal vramPage		: std_logic_vector(7 downto 0);
	
	signal attr			: std_logic_vector(7 downto 0) := "00000111";
	signal shift		: std_logic_vector(7 downto 0) := "00110011";
	
	signal Paper_r 		: std_logic;
	signal Blank_r 		: std_logic;
	signal Attr_r 		: std_logic_vector(7 downto 0);
	signal Shift_r		: std_logic_vector(7 downto 0);

	signal paper		: std_logic;
	signal hsync		: std_logic;
	signal vsync1		: std_logic;
	signal vsync2		: std_logic;
	
	signal palSync		: std_logic;
	
	signal specR		: std_logic;
	signal specG		: std_logic;
	signal specB		: std_logic;
	signal specY		: std_logic;
	
	signal rgbR	 		: std_logic_vector(7 downto 0);
	signal rgbG	 		: std_logic_vector(7 downto 0);
	signal rgbB	 		: std_logic_vector(7 downto 0);
	signal rgbSum 		: unsigned(9 downto 0);
	
	signal VideoHS_n   : std_logic;                       -- Holizontal Sync
	signal VideoVS_n   : std_logic;                       -- Vertical Sync
	--signal VideoCS_n   : std_logic;                       -- Composite Sync
	signal videoY      : std_logic_vector(7 downto 0);   -- Svideo_Y
	signal videoC      : std_logic_vector(7 downto 0);   -- Svideo_C
	signal videoV      : std_logic_vector(7 downto 0);   -- CompositeVideo
	
    signal ayCLK		: std_logic;
	signal ayBC1		: std_logic;
	signal ayBDIR		: std_logic;
	
	--signal ayCS			: std_logic;
	--signal ayASEL		: std_logic;
	--signal ayWR			: std_logic;
	
	signal ayDin        : std_logic_vector(7 downto 0);
	signal ayDout       : std_logic_vector(7 downto 0);
	signal ayDoutLe		: std_logic;

	--signal ayA			: std_logic_vector(7 downto 0);	
	--signal ayB			: std_logic_vector(7 downto 0);
	--signal ayC			: std_logic_vector(7 downto 0);
	signal ayOUT        : std_logic_vector(7 downto 0);
	signal ayOUT_A      : std_logic_vector(7 downto 0);
	signal ayOUT_B      : std_logic_vector(7 downto 0);
	signal ayOUT_C      : std_logic_vector(7 downto 0);

	signal tdaData		: std_logic := '1';
	signal tdaWs		: std_logic := '1';
	signal tdaBck		: std_logic := '1';
	
	signal cpuCLK		: std_logic;
	signal cpuWait		: std_logic;
	signal cpuReset		: std_logic := '0';
	
	signal cpuINT		: std_logic;
	signal cpuM1		: std_logic;
	signal cpuMREQ		: std_logic;
	signal cpuIORQ		: std_logic;
	signal cpuRD		: std_logic;
	signal cpuWR		: std_logic;
	signal cpuA			: std_logic_vector(15 downto 0);
	
	signal cpuDout		: std_logic_vector(7 downto 0);
	signal cpuDin		: std_logic_vector(7 downto 0);	
	
	signal cpuSavePC      : std_logic_vector(15 downto 0);
	signal cpuSaveINT     : std_logic_vector(7 downto 0);
	signal cpuRestorePC   : std_logic_vector(15 downto 0);
	
	signal cpuRestoreINT  : std_logic_vector(7 downto 0);		
	signal cpuRestorePC_n : std_logic := '1';
	
	component pll
		port(
			inclk0 : in std_logic;
			c0     : out std_logic;
			c2     : out std_logic
		);
	end component;	

	component T80s
		port(
			RESET_n		: in std_logic;
			CLK_n		: in std_logic;
			WAIT_n		: in std_logic;
			INT_n		: in std_logic;
			NMI_n		: in std_logic;
			BUSRQ_n		: in std_logic;
			M1_n		: out std_logic;
			MREQ_n		: out std_logic;
			IORQ_n		: out std_logic;
			RD_n		: out std_logic;
			WR_n		: out std_logic;
			RFSH_n		: out std_logic;
			HALT_n		: out std_logic;
			BUSAK_n		: out std_logic;
			A			: out std_logic_vector(15 downto 0);
			DI			: in std_logic_vector(7 downto 0);
			DO			: out std_logic_vector(7 downto 0);
			
			SavePC      : out std_logic_vector(15 downto 0);
			SaveINT     : out std_logic_vector(7 downto 0);
			RestorePC   : in std_logic_vector(15 downto 0);
			RestoreINT  : in std_logic_vector(7 downto 0);
		
			RestorePC_n : in std_logic			
		);
	end component;	

	component T80a
		port(
			RESET_n		: in std_logic;
			CLK_n		: in std_logic;
			WAIT_n		: in std_logic;
			INT_n		: in std_logic;
			NMI_n		: in std_logic;
			BUSRQ_n		: in std_logic;
			M1_n		: out std_logic;
			MREQ_n		: out std_logic;
			IORQ_n		: out std_logic;
			RD_n		: out std_logic;
			WR_n		: out std_logic;
			RFSH_n		: out std_logic;
			HALT_n		: out std_logic;
			BUSAK_n		: out std_logic;
			A			: out std_logic_vector(15 downto 0);
			D			: inout std_logic_vector(7 downto 0);
			
			SavePC      : out std_logic_vector(15 downto 0);
			SaveINT     : out std_logic_vector(7 downto 0);
			RestorePC   : in std_logic_vector(15 downto 0);
			RestoreINT  : in std_logic_vector(7 downto 0);
		
			RestorePC_n : in std_logic			
		);
	end component;	

		component vencode
		port(
			clk84m    : in std_logic;
			reset     : in std_logic;
			videoR    : in std_logic_vector(5 downto 0);
			videoG    : in std_logic_vector(5 downto 0);
			videoB    : in std_logic_vector(5 downto 0);
			videoHS_n : in std_logic;
			videoVS_n : in std_logic;
			videoPS_n : in std_logic;
			videoY    : out std_logic_vector(7 downto 0);
			videoC    : out std_logic_vector(7 downto 0);
			videoV    : out std_logic_vector(7 downto 0);
			subcarrierDelta : unsigned( 23 downto 0 )
		);
	end component;

	component YM2149 is
		port (
			-- data bus
			  I_DA                : in  std_logic_vector(7 downto 0);
			  O_DA                : out std_logic_vector(7 downto 0);
			  O_DA_OE_L           : out std_logic;
			  
			-- control
			  I_A9_L              : in  std_logic;
			  I_A8                : in  std_logic;
			  I_BDIR              : in  std_logic;
			  I_BC2               : in  std_logic;
			  I_BC1               : in  std_logic;
			  I_SEL_L             : in  std_logic;

			  O_AUDIO             : out std_logic_vector(7 downto 0);
			  O_AUDIO_A           : out std_logic_vector(7 downto 0);
              O_AUDIO_B           : out std_logic_vector(7 downto 0);
              O_AUDIO_C           : out std_logic_vector(7 downto 0);			  
			  
			-- port a
			  I_IOA               : in  std_logic_vector(7 downto 0);
			  O_IOA               : out std_logic_vector(7 downto 0);
			  O_IOA_OE_L          : out std_logic;
			-- port b
			  I_IOB               : in  std_logic_vector(7 downto 0);
			  O_IOB               : out std_logic_vector(7 downto 0);
			  O_IOB_OE_L          : out std_logic;

			  ENA                 : in  std_logic; -- clock enable for higher speed operation
			  RESET_L             : in  std_logic;
			  CLK                 : in  std_logic  -- note 6 Mhz
			);
	end component;

	component sdram is
		generic(
			freq : integer
		);

		port(
			clk			 : in std_logic;		
			reset		 : in std_logic;
			
			memAddress  : in std_logic_vector(23 downto 0);
			memDataIn   : in std_logic_vector(15 downto 0);
			memDataOut  : out std_logic_vector(15 downto 0);
			memDataMask : in std_logic_vector(1 downto 0);
			memWr       : in std_logic;
			memReq      : in std_logic;
			memAck      : out std_logic;

			memAddress2  : in std_logic_vector(23 downto 0);
			memDataIn2   : in std_logic_vector(15 downto 0);
			memDataOut2  : out std_logic_vector(15 downto 0);
			memDataMask2 : in std_logic_vector(1 downto 0);
			memWr2       : in std_logic;
			memReq2      : in std_logic;
			memAck2      : out std_logic;

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
				
	end component;
	
	component ps2fifo is
	
		port (
			clk_i 		: in std_logic;   -- Global clk
			rst_i 		: in std_logic;   -- GLobal Asinchronous reset

			data_o		: out std_logic_vector(7 downto 0);  -- Data in
			data_i		: in  std_logic_vector(7 downto 0);  -- Data out
			
			rd_i		: in  std_logic;
			wr_i		: in  std_logic;
			
			out_ready_o : out std_logic;
			in_full_o   : out std_logic;

			ps2_clk_io  : inout std_logic;   -- PS2 Clock line
			ps2_data_io : inout std_logic
		);	
		
	end component;

begin

	U00 : pll
		port map(
			inclk0 => CLK_20,
			c0     => memclk,
			c2     => open
		
		);
		
	U01 : t80a
		port map(
			RESET_n => not ( cpuReset or reset ),
			CLK_n   => cpuCLK,
			WAIT_n  => '1',
			INT_n   => cpuINT,
			NMI_n   => '1',
			BUSRQ_n => '1',
			M1_n    => cpuM1,
			MREQ_n  => cpuMREQ,
			IORQ_n  => cpuIORQ,
			RD_n    => cpuRD,
			WR_n    => cpuWR,
			RFSH_n  => open,
			HALT_n  => open,
			BUSAK_n => open,
			A       => cpuA,
			D       => cpuDout,
			--DO      => cpuDout,
			--DI      => cpuDin,
			
			SavePC => cpuSavePC,
			SaveINT => cpuSaveINT,
			RestorePC => cpuRestorePC,
			RestoreINT => cpuRestoreINT,
			
			RestorePC_n => cpuRestorePC_n
		);				
		
	U02 : YM2149
		port map(
			RESET_L => not ( cpuReset or reset ),
			CLK     => ayCLK,
			I_DA    => ayDin,
			O_DA    => ayDout,
			O_DA_OE_L	=> ayDoutLe,
			
			ENA		=> not cpuHaltAck,
			I_SEL_L => '1',
			
			I_IOA	=> x"00",
			I_IOB	=> x"00",
			
			I_A9_L	=> '0',
			I_A8	=> '1',
			I_BDIR	=> ayBDIR,
			I_BC2	=> '1',
			I_BC1	=> ayBC1,
			
			O_AUDIO	=> ayOUT,

			O_AUDIO_A	=> ayOUT_A,
			O_AUDIO_B	=> ayOUT_B,
			O_AUDIO_C	=> ayOUT_C
		);				
		
	U03 : vencode
		port map(
			memclk, '0', rgbR( 7 downto 2 ), rgbG( 7 downto 2 ), rgbB( 7 downto 2 ), 
			VideoHS_n, VideoVS_n, palSync, videoY, videoC, videoV, subcarrierDelta
		);
		
	U04 : sdram
		generic map(
			freq => freq
		)
		
		port map(
			clk => memclk,
			reset => reset,
			
			memAddress => memAddress,
			memDataIn => memDataIn,
			memDataOut => memDataOut,
			memDataMask => memDataMask,
			memWr => memWr,
			memReq => memReq,
			memAck => memAck,

			memAddress2 => memAddress2,
			memDataIn2 => memDataIn2,
			memDataOut2 => memDataOut2,
			memDataMask2 => memDataMask2,
			memWr2 => memWr2,
			memReq2 => memReq2,
			memAck2 => memAck2,

			pMemClk => pMemClk,
			pMemCke => pMemCke,
			pMemCs_n => pMemCs_n,
			pMemRas_n => pMemRas_n,
			pMemCas_n => pMemCas_n,
			pMemWe_n => pMemWe_n,
			pMemUdq => pMemUdq,
			pMemLdq => pMemLdq,
			pMemBa1 => pMemBa1,
			pMemBa0 => pMemBa0,
			pMemAdr => pMemAdr,
			pMemDat => pMemDat
		);
		
	U05: ps2fifo
		port map(
			clk_i => memclk,
			rst_i => reset,

			data_o => keysDataOut,
			data_i => keysDataIn,
			
			rd_i => keysRd,
			wr_i => keysWr,
			
			out_ready_o => keysReady,
			in_full_o => keysFull,

			ps2_clk_io => KEYS_CLK,
			ps2_data_io => KEYS_DATA
		);
			
	U06: ps2fifo
		port map(
			clk_i => memclk,
			rst_i => reset,

			data_o => mouseDataOut,
			data_i => mouseDataIn,
			
			rd_i => mouseRd,
			wr_i => mouseWr,
			
			out_ready_o => mouseReady,
			in_full_o => mouseFull,

			ps2_clk_io => MOUSE_CLK,
			ps2_data_io => MOUSE_DATA
		);
			
				
    process( memclk )
    
		variable divCounter28 : unsigned(7 downto 0) := x"00";
		variable mulCounter28 : unsigned(7 downto 0) := x"00";
		
    begin
    
        if memclk'event and memclk = '1' then
		
			counter <= counter + 1;
			divCounter28 := divCounter28 + 28;
			
			clk28m <= '0';
			clk14m <= '0';
			clk7m <= '0';
			clk35m <= '0';
			ayCLK <= '0';
			
			if divCounter28 >= freq then
			
				divCounter28 := divCounter28 - freq;
				mulCounter28 := mulCounter28 + 1;
				
				clk28m <= '1';
				
				if mulCounter28( 0 ) = '0' then
					clk14m <= '1';					
				end if;
				
				if mulCounter28( 1 downto 0 ) = "00" then
					clk7m <= '1';
				end if;				
				
				if mulCounter28( 2 downto 0 ) = "000" then
					clk35m <= '1';
				end if;

				if mulCounter28( 3 downto 0 ) = "0000" then
					ayCLK <= '1';
				end if;				
				
			end if;
			
		end if;
		
    end process;    

	joyCode0 <= "0000000000" & not JOY0(5) & not JOY0(1) & not JOY0(4) & not JOY0(3) & not JOY0(2) & not JOY0(0);
	joyCode1 <= "0000000000" & not JOY1(5) & not JOY1(1) & not JOY1(4) & not JOY1(3) & not JOY1(2) & not JOY1(0);

	-- Dendy joystick routine
	
--	dendyDat <= pJoystick0(2);
--
--	process (memclk)
--	
--		variable dendyCounter : unsigned(5 downto 0);
--		variable dendyCodeCounter : unsigned(3 downto 0) := "1111";
--		variable dendyCodeTemp : std_logic_vector(15 downto 0);
--		
--	begin
--		if (memclk'event and memclk = '1' and clk14m = '1') then
--		
--			if( counter( 9 downto 0 ) = 0 ) then
--				if( dendyCounter = 0 ) then
--					if( dendyCodeTemp /= dendyCodePrev ) then
--						dendyCodePrev <= dendyCodeTemp;
--						dendyCodeCounter := "1111";						
--					elsif dendyCodeCounter /= 0 then
--						dendyCodeCounter := dendyCodeCounter - 1;
--					else
--						dendyCode <= dendyCodeTemp;
--					end if;
--					
--					DENDY_LE <= '0';					
--				elsif( dendyCounter = 1 ) then
--					DENDY_LE <= '1';
--				elsif( dendyCounter( 1 downto 0 ) = 0 ) then
--					DENDY_CLK <= '0';
--				elsif( dendyCounter( 1 downto 0 ) = 1 ) then
--					DENDY_CLK <= '1';
--				elsif( dendyCounter( 1 downto 0 ) = 2 ) then
--					dendyCodeTemp := dendyDat & dendyCodeTemp( 15 downto 1 );
--				end if;					
--				
--				dendyCounter := dendyCounter + 1;
--			end if;
--			
--			counter <= counter + 1;
--			
--		end if;
--	end process;

	reset <= not RESET_n;

	specTrdosToggleFlag <= '1' when specTrdosFlag = '0' and cpuM1 = '0' and cpuMREQ = '0' and specPort7ffd(4) = '1' and cpuA( 15 downto 8 ) = "00111101" else
						   '1' when specTrdosFlag = '1' and cpuM1 = '0' and cpuMREQ = '0' and cpuA( 15 downto 14 ) /= "00" else
						   '0';

	romPage <= 	"010" when ( specTrdosFlag xor specTrdosToggleFlag ) = '1' else
					"001" when specPort7ffd(4) = '1' else
					"000";

	ramPage <= 	"00000101" when cpuA( 15 downto 14 ) = "01" else
					"00000010" when cpuA( 15 downto 14 ) = "10" else
					"00" & specPort7ffd( 7 downto 5 ) & specPort7ffd( 2 downto 0 ) when specMode = 2 else
					"00000" & specPort7ffd( 2 downto 0 );
					
				
	ayDin <= cpuDout;
	ayBC1 <= '1' when cpuM1 = '1' and cpuIORQ = '0' and cpuA( 15 downto 14 ) = "11" and cpuA(1 downto 0) = "01" else '0';
	ayBDIR <= '1' when cpuM1 = '1' and cpuIORQ = '0' and cpuWR = '0' and cpuA( 15 ) = '1' and cpuA(1 downto 0) = "01" else '0';
	
    process( memclk )
    
		variable addressReg : unsigned(23 downto 0) := "000000000000000000000000";

		variable ArmWr	: std_logic_vector(1 downto 0);
		variable ArmRd	: std_logic_vector(1 downto 0);
		variable ArmAle	: std_logic_vector(1 downto 0);
		    
		variable iCpuWr	: std_logic_vector(1 downto 0);
		variable iCpuRd	: std_logic_vector(1 downto 0);
		variable iCpuClk	: std_logic_vector(1 downto 0);
		
		variable kbdTmp : std_logic_vector(4 downto 0);
		variable cpuReq : std_logic;
		
		
    begin

		if reset = '1' then
			
			cpuReset <= '0';
			cpuHaltReq <= '0';

		elsif memclk'event and memclk = '1' then
        
			if cpuReset = '1' then
			
				if specMode = 0 then
					specPort7ffd <= x"30";
				else
					specPort7ffd <= x"00";
				end if;
				
				specTrdosFlag <= '0';
				specTrdosWait <= '0';
				
			end if;
			
			if specTrdosToggleFlag = '1' then
				specTrdosFlag <= not specTrdosFlag;
			end if;
			
			if cpuTraceReq = '1' and cpuTraceAck = '1' then
				cpuTraceReq <= '0';
			end if;
			
			if cpuOneCycleWaitReq = '1' and cpuOneCycleWaitAck = '1' then
				cpuOneCycleWaitReq <= '0';
			end if;			

			if( memReq = '1' and memAck = '1' ) then		
			
				if cpuReq = '1' then
					if( memWr = '0' ) then
						if cpuA(0) = '0' then
							cpuDin <= memDataOut( 7 downto 0 );
						else
							cpuDin <= memDataOut( 15 downto 8 );
						end if;					
					end if;
					
					cpuMemoryWait <= '0';
				else
					if( memWr = '0' ) then
						if addressReg( 23 ) = '0' then
							if addressReg( 0 ) = '0' then
								ARM_AD <= x"00" & memDataOut( 7 downto 0 );
							else
								ARM_AD <= x"00" & memDataOut( 15 downto 8 );
							end if;
						else
							ARM_AD <= memDataOut;
						end if;
					end if;
					
					ARM_WAIT <= '1';
				end if;
				
				memReq <= '0';
				
			end if;
			
			if ayDoutLe = '0' and ayMode /= 0 then
				cpuDin <= ayDout;
			end if;
			
			------------------------------------------------------------------------
			
			keysRd <= '0';
			keysWr <= '0';
        
			if ( ArmAle = "10" ) then
				addressReg := unsigned( ARM_A ) & unsigned( ARM_AD );
			end if;
				
			if ( ArmWr = "10" ) then
				if addressReg( 23 ) = '0' and ( cpuHaltAck = '1' or specTrdosWait = '1' ) then
					memAddress <= std_logic_vector( activeBank & addressReg( 22 downto 1 ) );
					memDataIn <= ARM_AD( 7 downto 0 ) & ARM_AD( 7 downto 0 );
					memDataMask( 0 ) <= not addressReg( 0 );
					memDataMask( 1 ) <= addressReg( 0 );
					memWr <= '1';
					
					cpuReq := '0';
					memReq <= '1';
					
				elsif addressReg( 23 downto 22 ) = "10" and ( cpuHaltAck = '1' or specTrdosWait = '1' ) then
					memAddress <= std_logic_vector( activeBank & addressReg( 21 downto 0 ) );
					memDataIn <= ARM_AD;
					memDataMask <= "11";
					memWr <= '1';
					
					cpuReq := '0';
					memReq <= '1';
								
				else
					if addressReg( 23 downto 8 ) = x"c000" then
						if addressReg( 7 downto 0 ) = x"00" then
							cpuHaltReq <= ARM_AD( 0 );
							cpuRestorePC_n <= not ARM_AD( 1 );
							cpuOneCycleWaitReq <= ARM_AD( 2 );
							cpuReset <= ARM_AD( 3 );
							
						elsif addressReg( 7 downto 0 ) = x"01" then
							cpuRestorePC <= ARM_AD;
						elsif addressReg( 7 downto 0 ) = x"02" then
							cpuRestoreINT <= ARM_AD( 7 downto 0 ); 
							
						elsif addressReg( 7 downto 0 ) = x"08" then
							cpuTraceReq <= ARM_AD(0);
						elsif addressReg( 7 downto 0 ) = x"09" then
							cpuBP0_en <= ARM_AD(0);
						elsif addressReg( 7 downto 0 ) = x"0A" then
							cpuBP0 <= ARM_AD;
							
						elsif addressReg( 7 downto 0 ) = x"10" then
							keyboard( 9 downto 0 ) <= ARM_AD( 12 downto 8 ) & ARM_AD( 4 downto 0 );
						elsif addressReg( 7 downto 0 ) = x"11" then
							keyboard( 19 downto 10 ) <= ARM_AD( 12 downto 8 ) & ARM_AD( 4 downto 0 );
						elsif addressReg( 7 downto 0 ) = x"12" then
							keyboard( 29 downto 20 ) <= ARM_AD( 12 downto 8 ) & ARM_AD( 4 downto 0 );
						elsif addressReg( 7 downto 0 ) = x"13" then
							keyboard( 39 downto 30 ) <= ARM_AD( 12 downto 8 ) & ARM_AD( 4 downto 0 );
						elsif addressReg( 7 downto 0 ) = x"14" then
							joystick <= ARM_AD(7 downto 0);
						elsif addressReg( 7 downto 0 ) = x"15" then
							tapeIn <= ARM_AD(0);
						elsif addressReg( 7 downto 0 ) = x"16" then
							specPortFe <= ARM_AD(7 downto 0);
						elsif addressReg( 7 downto 0 ) = x"17" then
							specPort7ffd <= ARM_AD(7 downto 0);
							
						elsif addressReg( 7 downto 0 ) = x"18" then
							specTrdosFlag <= ARM_AD(0);							
						elsif addressReg( 7 downto 0 ) = x"19" then
							specTrdosWait <= '0';
						elsif addressReg( 7 downto 0 ) = x"1b" then
							cpuDin <= ARM_AD(7 downto 0);
						elsif addressReg( 7 downto 0 ) = x"1d" then							
							specTrdosStat <= ARM_AD(7 downto 0);
						
						elsif addressReg( 7 downto 0 ) = x"20" then
							activeBank <= unsigned( ARM_AD( 1 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"21" then
							armVideoPage <= ARM_AD;
						elsif addressReg( 7 downto 0 ) = x"22" then
							armBorder <= ARM_AD;

						elsif addressReg( 7 downto 0 ) = x"32" and keysFull = '0' then
							keysDataIn <= ARM_AD( 7 downto 0 );
							keysWr <= '1';
						elsif addressReg( 7 downto 0 ) = x"33" and mouseFull = '0' then
							mouseDataIn <= ARM_AD( 7 downto 0 );
							mouseWr <= '1';
						elsif addressReg( 7 downto 0 ) = x"40" then
							specMode <= unsigned( ARM_AD );
						elsif addressReg( 7 downto 0 ) = x"41" then
							syncMode <= unsigned( ARM_AD );
						elsif addressReg( 7 downto 0 ) = x"42" then
							cpuTurbo <= ARM_AD( 1 downto 0 );
						elsif addressReg( 7 downto 0 ) = x"43" then
							videoMode <= unsigned( ARM_AD );
						elsif addressReg( 7 downto 0 ) = x"44" then
							subcarrierDelta( 15 downto 0 ) <= unsigned( ARM_AD( 15 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"45" then
							subcarrierDelta( 23 downto 16 ) <= unsigned( ARM_AD( 7 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"46" then
							dacMode <= unsigned( ARM_AD );
						elsif addressReg( 7 downto 0 ) = x"47" then
							ayMode <= unsigned( ARM_AD );

						end if;
						
					end if;
					
					ARM_WAIT <= '1';
				end if;
				
			elsif ( ArmWr = "01" ) then
				addressReg := addressReg + 1;
				ARM_WAIT <= '0';
			end if;
				
			if ( ArmRd = "10" ) then
				if addressReg( 23 ) = '0' and ( cpuHaltAck = '1' or specTrdosWait = '1' ) then
					memAddress <= std_logic_vector( activeBank & addressReg( 22 downto 1 ) );
					memWr <= '0';
					
					cpuReq := '0';
					memReq <= '1';				
				elsif addressReg( 23 downto 22 ) = "10" and ( cpuHaltAck = '1' or specTrdosWait = '1' ) then
					memAddress <= std_logic_vector( activeBank & addressReg( 21 downto 0 ) );
					memWr <= '0';
					
					cpuReq := '0';
					memReq <= '1';				
				else
					if addressReg( 23 downto 8 ) = x"c000" then
						
						if addressReg( 7 downto 0 ) = x"00" then
							ARM_AD <= x"00" & b"0000000" & cpuHaltAck;
						elsif addressReg( 7 downto 0 ) = x"01" then
							ARM_AD <= std_logic_vector( cpuSavePC );
						elsif addressReg( 7 downto 0 ) = x"02" then
							ARM_AD <= x"00" & cpuSaveINT;
							
						elsif addressReg( 7 downto 0 ) = x"16" then
							ARM_AD <= x"00" & specPortFe;
						elsif addressReg( 7 downto 0 ) = x"17" then
							ARM_AD <= x"00" & specPort7ffd;
						elsif addressReg( 7 downto 0 ) = x"18" then
							ARM_AD <= x"00" & b"0000000" & specTrdosFlag;
						elsif addressReg( 7 downto 0 ) = x"19" then
							ARM_AD <= x"00" & b"000000" & specTrdosWr & specTrdosWait;
						elsif addressReg( 7 downto 0 ) = x"1a" then
							ARM_AD <= cpuA;
						elsif addressReg( 7 downto 0 ) = x"1b" then
							ARM_AD <= x"00" & cpuDout;
						elsif addressReg( 7 downto 0 ) = x"1c" then
							ARM_AD <= std_logic_vector( specTrdosCounter );
							
						elsif addressReg( 7 downto 0 ) = x"30" then
							ARM_AD <= joyCode0;
						elsif addressReg( 7 downto 0 ) = x"31" then
							ARM_AD <= joyCode1;
						elsif addressReg( 7 downto 0 ) = x"32" then
							ARM_AD <= x"0000";														
							if keysReady = '1' then
								ARM_AD <= x"80" & keysDataOut;
								keysRd <= '1';
							end if;
						elsif addressReg( 7 downto 0 ) = x"33" then
							ARM_AD <= x"0000";														
							if mouseReady = '1' then
								ARM_AD <= x"80" & mouseDataOut;
								mouseRd <= '1';
							end if;
							
						elsif addressReg( 7 downto 0 ) = x"50" then
							ARM_AD <= std_logic_vector( counter( 15 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"51" then
							ARM_AD <= std_logic_vector( counter( 31 downto 16 ) );
						else
							ARM_AD <= x"ffff";
							
						end if;					
					else
						ARM_AD <= x"ffff";					
					end if;
					
					ARM_WAIT <= '1';
				end if;
				
			elsif ( ArmRd = "01" ) then
			
				ARM_AD <= (others => 'Z');
				addressReg := addressReg + 1;
				ARM_WAIT <= '0';
				
			end if;
			
			ArmAle := ArmAle(0) & ARM_ALE;
			ArmWr := ArmWr(0) & ARM_WR;
			ArmRd := ArmRd(0) & ARM_RD;
			
			----------------------------------------------------------------------------------
			
			iCpuClk := iCpuClk(0) & cpuCLK;
			
			
			--if specTrdosToggleFlag = '0' then 			
			iCpuWr := iCpuWr(0) & cpuWR;
			iCpuRd := iCpuRd(0) & cpuRD;				
			--end if;

			if ( iCpuWr = "10" ) then
				if cpuMREQ = '0' then
					
					if cpuA( 15 downto 14 ) /= "00" then
						memAddress <= "000" & ramPage & cpuA( 13 downto 1 );
					
						memDataIn <= cpuDout & cpuDout;
						memDataMask(0) <= not cpuA(0);
						memDataMask(1) <= cpuA(0);
						memWr <= '1';
						
						cpuReq := '1';
						memReq <= '1';
						
						if cpuTurbo /= "00" then
							cpuMemoryWait <= '1';
						end if;
						
					end if;
					
				elsif cpuIORQ = '0' and cpuM1 = '1' then
				
					if specTrdosFlag = '1' and ( 
						cpuA( 7 downto 0 ) = x"1F" or
						cpuA( 7 downto 0 ) = x"3F" or
						cpuA( 7 downto 0 ) = x"5F" or
						cpuA( 7 downto 0 ) = x"7F" or
						cpuA( 7 downto 0 ) = x"FF" 	) then 
						
						specTrdosWait <= '1';
						specTrdosWr <= '1';
						
					else
					
						if cpuA( 7 downto 0 ) = x"FE" then
							specPortFe <= cpuDout;
						elsif cpuA( 15 ) = '0' and cpuA( 7 downto 0 ) = x"fd" then
							if specMode = 2 or specPort7ffd(5) = '0' then
								specPort7ffd <= cpuDout;
							end if;
						end if;
						
					end if;
					
				end if;				
			end if;

			if ( iCpuRd = "10" ) then
				if cpuMREQ = '0' then
					
					if cpuA( 15 downto 14 ) = "00" then
						memAddress <= "001" & "00000" & romPage & cpuA( 13 downto 1 );
					else
						memAddress <= "000" & ramPage & cpuA( 13 downto 1 );
					end if;						
					
					memWr <= '0';
					
					cpuReq := '1';
					memReq <= '1';
					
					if cpuTurbo /= "00" then
						cpuMemoryWait <= '1';
					end if;
					
				elsif cpuIORQ = '0' and cpuM1 = '1' then

					if specTrdosFlag = '1' then
						if cpuA( 7 downto 0 ) = x"1F" or
							cpuA( 7 downto 0 ) = x"3F" or
							cpuA( 7 downto 0 ) = x"5F" or
							cpuA( 7 downto 0 ) = x"7F" or
							cpuA( 7 downto 0 ) = x"FF" then
						
							specTrdosWait <= '1';
							specTrdosWr <= '0';
							
						end if;
						
					else
					
						if cpuA( 7 downto 0 ) = x"FE" then
											
							kbdTmp := ( keyboard(  4 downto  0 ) or ( cpuA(8) & cpuA(8) & cpuA(8) & cpuA(8) & cpuA(8) ) )
									and ( keyboard(  9 downto  5 ) or ( cpuA(9) & cpuA(9) & cpuA(9) & cpuA(9) & cpuA(9) ) )
									and ( keyboard( 14 downto 10 ) or ( cpuA(10) & cpuA(10) & cpuA(10) & cpuA(10) & cpuA(10) ) )
									and ( keyboard( 19 downto 15 ) or ( cpuA(11) & cpuA(11) & cpuA(11) & cpuA(11) & cpuA(11) ) )
									and ( keyboard( 24 downto 20 ) or ( cpuA(12) & cpuA(12) & cpuA(12) & cpuA(12) & cpuA(12) ) )
									and ( keyboard( 29 downto 25 ) or ( cpuA(13) & cpuA(13) & cpuA(13) & cpuA(13) & cpuA(13) ) )
									and ( keyboard( 34 downto 30 ) or ( cpuA(14) & cpuA(14) & cpuA(14) & cpuA(14) & cpuA(14) ) )
									and ( keyboard( 39 downto 35 ) or ( cpuA(15) & cpuA(15) & cpuA(15) & cpuA(15) & cpuA(15) ) );
											
							cpuDin <= "0" & tapeIn & "0" & kbdTmp;
						elsif cpuA( 7 downto 0 ) = x"1F" then
							cpuDin <= joystick;
						--elsif cpuA = x"7ffd" then
							--cpuDin <= specPort7ffd;
						else
							cpuDin <= x"FF";
						end if;
											
					end if;
					
				end if;				
			end if;
			
			if cpuHaltReq = '1' then
				specTrdosWait <= '0';
			end if;
			
        end if;
        
    end process;
    
	cpuDout <= cpuDin when cpuRD = '0' else "ZZZZZZZZ";
	
	------------------------------------------------------------------------------
	
	process(memclk)
	
			variable cpuSaveInt7_prev : std_logic := '0';

	begin
		if reset = '1' then
			
			cpuHaltAck <= '0';
				
		elsif memclk'event and memclk = '1' then
	
			if cpuTraceReq = '1' then
				
				cpuHaltAck <= '0';
				
			elsif cpuHaltAck = '0' and cpuSaveINT( 7 ) = '0' and cpuSaveInt7_prev = '1' then

				if cpuHaltReq = '1' then
					cpuHaltAck <= '1';
				elsif cpuBP0_en = '1' and cpuSavePC = cpuBP0 then
					cpuHaltAck <= '1';
				end if;			
								
			end if;
			
			cpuSaveInt7_prev := cpuSaveINT( 7 );
			
		end if;
		
	end process;
	
	process(memclk)
	
			variable specTrdosPreCounter : unsigned( 15 downto 0 ) := x"0000";

	begin
		if reset = '1' then
			
			specTrdosPreCounter := x"0000";
			specTrdosCounter <= x"0000";
				
		elsif memclk'event and memclk = '1' then
		
			if cpuWait = '0' then
				
				specTrdosPreCounter := specTrdosPreCounter + 1;
				
				if specTrdosPreCounter >= freq * 10 then
				
					specTrdosPreCounter := x"0000";
					specTrdosCounter <= specTrdosCounter + 1;
					
				end if;
				
			end if;
			
		end if;                                                      
	end process;
	
	---------------------------------------------------------------------------------------

    paper <= '0' when Hor_Cnt(5) = '0' and Ver_Cnt(5) = '0' and ( Ver_Cnt(4) = '0' or Ver_Cnt(3) = '0' ) else '1';      
	hsync <= '0' when Hor_Cnt(5 downto 2) = "1010" else '1';
	vsync1 <= '0' when Hor_Cnt(5 downto 1) = "00110" or Hor_Cnt(5 downto 1) = "10100" else '1';
	vsync2 <= '1' when Hor_Cnt(5 downto 2) = "0010" or Hor_Cnt(5 downto 2) = "1001" else '0';
	
	cpuWait <= cpuHaltAck or cpuMemoryWait or specTrdosWait or cpuOneCycleWaitReq;

	process( memclk )
	
		variable cpuIntSkip : std_logic := '0';
		
	begin
		if memclk'event and memclk = '1' then
		
			if ( cpuTurbo = "00" and clk7m = '1' and ChrC_Cnt(0) = '0' ) or
				( cpuTurbo = "01" and clk7m = '1' ) or
				( cpuTurbo = "10" and clk14m = '1' ) or
				( cpuTurbo = "11" and clk28m = '1' ) then
				
				if specIntCounter = 32 then
					cpuINT <= '1';
				end if;

				specIntCounter <= specIntCounter + 1;
				cpuOneCycleWaitAck <= cpuOneCycleWaitReq;
				
				if cpuWait = '0' then
					cpuCLK <= '0';
					cpuTraceAck <= cpuTraceReq;
				end if;
				
			else
				cpuCLK <= '1';
			end if;

			if clk7m = '1' then
			
				if ChrC_Cnt = 7 then
				
					if Hor_Cnt = 55 then
						Hor_Cnt <= (others => '0');
					else
						Hor_Cnt <= Hor_Cnt + 1;
					end if;
					
					if Hor_Cnt = 39 then					
						if ChrR_Cnt = 7 then
							if ( Ver_Cnt = 38 and syncMode = 0 ) or ( Ver_Cnt = 39 and syncMode = 1 ) then
								Ver_Cnt <= (others => '0');
								Invert <= Invert + 1;
							else
								Ver_Cnt <= Ver_Cnt + 1;
							end if;							
						end if;
						
						ChrR_Cnt <= ChrR_Cnt + 1;
						
					end if;
				end if;
				
				if ChrC_Cnt = 7 then
					
					VideoHS_n <= hsync;					
					
					if Ver_Cnt /= 31 then
						VideoVS_n <= '1';
						palSync <= hsync;
					elsif ChrR_Cnt = 3 or ChrR_Cnt = 4 or ( ChrR_Cnt = 5 and ( Hor_Cnt >= 40 or Hor_Cnt < 12 ) ) then
						VideoVS_n <= '0';
						palSync <= vsync2;
					else
						VideoVS_n <= '1';
						palSync <= vsync1;
					end if;
					
				end if;
				
				if syncMode = 0 then
					if Ver_Cnt = 31 and ChrR_Cnt = 0 and Hor_Cnt = 0 and ChrC_Cnt = 0 then
						cpuINT <= cpuIntSkip;
						specIntCounter <= x"00000000";
						cpuIntSkip := '0';
					end if;
				else
					if Ver_Cnt = 30 and ChrR_Cnt = 0 and Hor_Cnt = 40 and ChrC_Cnt = 0 then
						cpuINT <= cpuIntSkip;
						specIntCounter <= x"00000000";
						cpuIntSkip := '0';
					end if;
				end if;
				
				if cpuHaltAck = '1' or specTrdosWait = '1' then
					cpuINT <= '1';
					cpuIntSkip := '1';
				end if;			
				
				ChrC_Cnt <= ChrC_Cnt + 1;

			end if;
			
		end if;
	end process;
	
	------------------------------------------------------------------------------------------
	
	vramPage <= "000001" & specPort7ffd(3) & "1";
		
	process( memclk )
		variable videoPage : std_logic_vector(10 downto 0);
	begin
		if (memclk'event and memclk = '1') then
		
			if armVideoPage(15) = '1' then
				videoPage := armVideoPage( 10 downto 0 );
			else
				videoPage := "000" & vramPage;
			end if;				
		
			if Paper = '0' then
			
				if memAck2 = '1' then
				
					if ChrC_Cnt < 4 then
						if Hor_Cnt(0) = '0' then
							Shift <= memDataOut2( 7 downto 0 );
						else
							Shift <= memDataOut2( 15 downto 8 );
						end if;
					else
						if Hor_Cnt(0) = '0' then
							Attr <= memDataOut2( 7 downto 0 );
						else
							Attr <= memDataOut2( 15 downto 8 );
						end if;
					end if;
					
					memReq2 <= '0';

				elsif memReq2 = '0' and clk7m = '1' then
					
					if ChrC_Cnt = 0 then
						memAddress2 <= videoPage & std_logic_vector( "0" & Ver_Cnt(4 downto 3) & ChrR_Cnt & Ver_Cnt(2 downto 0) & Hor_Cnt(4 downto 1) );
						memWr2 <= '0';
						memReq2 <= '1';
						
					elsif ChrC_Cnt = 4 then
						memAddress2 <= videoPage & std_logic_vector( "0110" & Ver_Cnt(4 downto 0) & Hor_Cnt(4 downto 1) );
						memWr2 <= '0';
						memReq2 <= '1';
					end if;

				end if;
					
			end if;				

		end if;
	end process;
	
	process( memclk )
	begin
		if memclk'event and memclk = '1' and clk7m = '1' then
			if ChrC_Cnt = 7 then
				
				Attr_r <= Attr;
				Shift_r <= Shift;
				
				if Hor_Cnt(5 downto 2) = 10 or Hor_Cnt(5 downto 2) = 11 or Ver_Cnt = 31 then
					blank_r <= '0';
				else 
					blank_r <= '1';
				end if;
				
				paper_r <= paper;
				
			else
				Shift_r <= Shift_r(6 downto 0) & "0";					
			end if;
		end if;
	end process;
	
	------------------------------------------------------------------------------------------------
	
	borderAttr <= specPortFe( 2 downto 0 );
	speaker <= specPortFe( 4 );

	process( memclk )
	begin
		if memclk'event and memclk = '1' and clk7m = '1' then
			if paper_r = '0' then
				if( Shift_r(7) xor ( Attr_r(7) and Invert(3) ) ) = '1' then
					specB <= Attr_r(0);
					specR <= Attr_r(1);
					specG <= Attr_r(2);
				else
					specB <= Attr_r(3);
					specR <= Attr_r(4);
					specG <= Attr_r(5);
				end if;
				
				specY <= Attr_r(6);
				
			elsif blank_r = '1' then
				
				if armBorder(15) = '1' then
					specB <= armBorder(0);
					specR <= armBorder(1);
					specG <= armBorder(2);
				else
					specB <= borderAttr(0);
					specR <= borderAttr(1);
					specG <= borderAttr(2);
				end if;
				
				specY <= '0';
				
			else
				specB <= '0';
				specR <= '0';
				specG <= '0';
				specY <= '0';

			end if;
		end if;				
	end process;

	------------------------------------------------------------------------------------------------

	sound( 14 ) <= speaker;
	sound( 12 ) <= tapeIn;
	
	soundLeft	<= sound when ayMode = 0 else
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT_A( 7 downto 0 ) & "0000000" ) + unsigned( "00" & ayOUT_B( 7 downto 0 ) & "000000" ) ) when ayMode = 1 else
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT_A( 7 downto 0 ) & "0000000" ) + unsigned( "00" & ayOUT_C( 7 downto 0 ) & "000000" ) ) when ayMode = 2 else
					std_logic_vector( unsigned( sound ) + unsigned( "00" & ayOUT_A( 7 downto 0 ) & "000000" ) + unsigned( "00" & ayOUT_B( 7 downto 0 ) & "000000" ) + unsigned( "00" & ayOUT_C( 7 downto 0 ) & "000000" ) );
					
	soundRight	<= sound when ayMode = 0 else
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT_C( 7 downto 0 ) & "0000000" ) + unsigned( "00" & ayOUT_B( 7 downto 0 ) & "000000" ) ) when ayMode = 1 else
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT_B( 7 downto 0 ) & "0000000" ) + unsigned( "00" & ayOUT_C( 7 downto 0 ) & "000000" ) ) when ayMode = 2 else
					std_logic_vector( unsigned( sound ) + unsigned( "00" & ayOUT_A( 7 downto 0 ) & "000000" ) + unsigned( "00" & ayOUT_B( 7 downto 0 ) & "000000" ) + unsigned( "00" & ayOUT_C( 7 downto 0 ) & "000000" ) );

	SOUND_LEFT	<= soundLeft( 15 downto 8 ) when dacMode = 0 else
		tdaData & tdaWs & tdaBck & "00000";

	SOUND_RIGHT	<= soundRight( 15 downto 8 ) when dacMode = 0 else
		"00000000";

	process( memclk )
		variable outLeft : unsigned(15 downto 0) := x"0000";
		variable outRight : unsigned(15 downto 0) := x"0000";
		variable outData : unsigned(47 downto 0) := x"000000000000";
		
		variable leftDataTemp : unsigned(23 downto 0) := x"000000";
		variable rightDataTemp : unsigned(23 downto 0) := x"000000";
		
		variable tdaCounter : unsigned(7 downto 0) := "00000000";
		
	begin
		if memclk'event and memclk = '1' and clk14m = '1' then
		
			if tdaCounter = 48 * 2 then
				tdaCounter := x"00";

				rightDataTemp := rightDataTemp / 48;
				outRight := rightDataTemp( 15 downto 0 );
				rightDataTemp := x"000000";

				leftDataTemp := leftDataTemp / 48;
				outLeft := leftDataTemp( 15 downto 0 );
				leftDataTemp := x"000000";
				
				--outRight := unsigned( soundRight );
				--outLeft := unsigned( soundLeft );
				
				outRight(15) := not outRight(15);
				outLeft(15) := not outLeft(15);
				
--				outData := unsigned( outRight( 15 ) & outRight( 15 ) & outRight( 15 ) & outRight( 15 ) &
--									outRight( 15 ) & outRight( 15 ) & outRight( 15 ) & outRight( 15 ) &
--									outRight( 15 downto 0 ) & 
--									outLeft( 15 ) & outLeft( 15 ) & outLeft( 15 ) & outLeft( 15 ) &
--									outLeft( 15 ) & outLeft( 15 ) & outLeft( 15 ) & outLeft( 15 ) &
--									outLeft( 15 downto 0 ) );
				

				outData := unsigned( x"00" & outRight( 15 downto 0 ) & x"00" & outLeft( 15 downto 0 ) );

			end if;
			
			if tdaCounter(0) = '0' then
    			
				tdaData <= outData( 47 );
				outData := outData( 46 downto 0 ) & "0";
				
				if dacMode = 1 then
			
					if tdaCounter( 7 downto 1 ) = 7 then
						tdaWs <= '0';
					elsif tdaCounter( 7 downto 1 ) = 24 + 7 then
						tdaWs <= '1';
					end if;
					
				else
			
					if tdaCounter( 7 downto 1 ) = 0 then
						tdaWs <= '0';
					elsif tdaCounter( 7 downto 1 ) = 24 then
						tdaWs <= '1';
					end if;

				end if;

				rightDataTemp := rightDataTemp + unsigned( soundRight );
				leftDataTemp := leftDataTemp + unsigned( soundLeft );
			
			end if;
		
			tdaBck <= tdaCounter(0);
			tdaCounter := tdaCounter + 1;
			
		end if;
	end process;
	
	------------------------------------------------------------------------------------------------

	rgbR <= specR & specR & "0" & "0" & "0" & "0" & "0" & "0" when specY = '0' else
		specR & specR & specR & specR & specR & specR & specR & specR;

	rgbG <= specG & specG & "0" & "0" & "0" & "0" & "0" & "0" when specY = '0' else
		specG & specG & specG & specG & specG & specG & specG & specG;

	rgbB <= specB & specB & "0" & "0" & "0" & "0" & "0" & "0" when specY = '0' else
		specB & specB & specB & specB & specB & specB & specB & specB;

	VIDEO_R <= videoV when videoMode = 0 else -- ( Composite --> SCART 20, 17 )
			"0" & rgbR( 7 downto 1 );		-- ( VGA 1, 6  --> SCART 15, 13 )
	VIDEO_G <= "0" & videoC( 7 downto 1 ) when videoMode = 0 else -- ( S-Video 4, 2 --> SCART 15, 13 )
			"0" & rgbG( 7 downto 1 );      -- ( VGA 2, 7  --> SCART 11, 9 )
	VIDEO_B <= videoY when videoMode = 0 else -- ( S-Video 3, 1 --> SCART 20, 17 )
			"0" & rgbB( 7 downto 1 );      -- ( VGA 3, 8  --> SCART 7, 5 )
	
	VIDEO_HSYNC <= palSync; -- VGA 13 --> SCART 20
	VIDEO_VSYNC <= '1';  -- VGA 14 --> SCART 16

end rtl;
