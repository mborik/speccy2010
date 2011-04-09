library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity speccy2010_top is
	port(
		CLK_20			: in std_logic;
		CLK_20_ALT		: in std_logic;
		
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
		JOY0_SEL  		: out std_logic := '0';
		JOY1			: in std_logic_vector(5 downto 0);
		JOY1_SEL  		: out std_logic := '0';
		
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

	constant memFreq : integer := 84;
	constant sysFreq : integer := 42;
	
	signal memclk : std_logic;
	signal sysclk : std_logic;
	
	signal reset : std_logic;

	signal clk28m : std_logic;
	signal clk14m : std_logic;
	signal clk7m : std_logic;
	signal clk35m : std_logic;

    signal counter20 : unsigned(31 downto 0) := x"00000000";    
    signal counterMem : unsigned(31 downto 0) := x"00000000";    
    signal counter20_en : std_logic := '0';    
    signal counterMem_en : std_logic := '0';

    signal cpuHaltReq : std_logic := '0';
    signal cpuHaltAck : std_logic := '0';
	signal cpuTraceReq : std_logic := '0';
	signal cpuTraceAck : std_logic := '0';
	signal cpuOneCycleWaitReq : std_logic := '0';
	signal cpuOneCycleWaitAck : std_logic := '0';
	
	--signal cpuBP0 	: std_logic_vector(15 downto 0) := x"0000";
	--cpuBP0_en : std_logic := '0';

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
	signal refresh	   : std_logic;
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

	signal Invert		: unsigned(4 downto 0) := "00000";

	signal hCnt			: unsigned(15 downto 0) := x"0000";
	signal vCnt			: unsigned(15 downto 0) := x"0000";
	signal hSize 		: integer;
	signal vSize 		: integer;
	
	signal ChrC_Cnt		: unsigned(2 downto 0) := "000";	-- Character column counter
	signal ChrR_Cnt		: unsigned(2 downto 0) := "000";	-- Character row counter
	signal Hor_Cnt		: unsigned(5 downto 0) := "000000";	-- Horizontal counter
	signal Ver_Cnt		: unsigned(5 downto 0) := "000000";	-- Vertical counter
	
	signal specIntCounter : unsigned(31 downto 0) := x"00000000";
    signal cpuMemoryWait : std_logic := '0';
    
    signal ulaContendentMem : std_logic := '0';
    signal ulaWaitMem : std_logic := '0';
    signal ulaWaitIo : std_logic := '0';
    signal ulaWaitCancel : std_logic := '0';
    signal ulaWait : std_logic := '0';

	signal specPortFe	: std_logic_vector(7 downto 0);
	signal specPortFf	: std_logic_vector(7 downto 0);
	signal specPort7ffd	: std_logic_vector(7 downto 0);
	
	signal specPortEff7	: std_logic_vector(7 downto 0);
	--signal specPortDff7	: std_logic_vector(7 downto 0);	
	
	signal specTrdosToggleFlag : std_logic := '0';
	signal specTrdosFlag : std_logic := '0';
	signal specTrdosWait : std_logic := '0';
	signal specTrdosWr : std_logic := '0';
	signal specTrdosCounter	: unsigned(15 downto 0) := x"0000";
	signal specTrdosPortFF	: std_logic_vector(7 downto 0) := x"00";
	
	signal specMode		: unsigned(7 downto 0) := x"00";
	signal syncMode		: unsigned(7 downto 0) := x"00";
    signal cpuTurbo 	: unsigned(7 downto 0 ) := x"00";
	signal videoMode	: unsigned(7 downto 0) := x"00";
    --signal subcarrierDelta : unsigned( 23 downto 0 ) := to_unsigned( 885662, 24 ); -- 885521
	signal dacMode		: unsigned(7 downto 0) := x"00";
	signal ayMode		: unsigned(7 downto 0) := x"00";
	signal videoAspectRatio	: unsigned(7 downto 0) := x"00";
	signal testVideo	: unsigned(7 downto 0) := x"00";
	
	signal borderAttr	: std_logic_vector(2 downto 0);
	signal speaker 		: std_logic;
	
	signal tapeIn 		: std_logic := '0';
	signal keyboard		: std_logic_vector(39 downto 0) := "1111111111111111111111111111111111111111";
	signal joystick		: std_logic_vector(7 downto 0) := x"00";
	signal mouseX		: std_logic_vector(7 downto 0) := x"00";
	signal mouseY		: std_logic_vector(7 downto 0) := x"00";
	signal mouseButtons	: std_logic_vector(7 downto 0) := x"00";
	signal sound 		: std_logic_vector(15 downto 0) := x"0000";
	
	signal soundLeft	: std_logic_vector(15 downto 0) := x"0000";
	signal soundRight	: std_logic_vector(15 downto 0) := x"0000";
	
	signal romPage		: std_logic_vector(2 downto 0);
	signal ramPage		: std_logic_vector(7 downto 0);
	signal vramPage		: std_logic_vector(7 downto 0);
	
	signal attr			: std_logic_vector(15 downto 0);
	signal shift		: std_logic_vector(15 downto 0);
	
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
	signal ayADDR		: std_logic;
	signal ayWR			: std_logic;
	signal ayRD			: std_logic;
	
	signal ayDin        : std_logic_vector(7 downto 0);
	signal ayDout       : std_logic_vector(7 downto 0);

	signal ayOUT        : std_logic_vector(7 downto 0);
	signal ayOUT_A      : std_logic_vector(7 downto 0);
	signal ayOUT_B      : std_logic_vector(7 downto 0);
	signal ayOUT_C      : std_logic_vector(7 downto 0);

	signal tdaData		: std_logic := '1';
	signal tdaWs		: std_logic := '1';
	signal tdaBck		: std_logic := '1';
	
	signal cpuCLK_nowait		: std_logic;
	signal cpuCLK_nowait_prev	: std_logic;
	signal cpuCLK				: std_logic;
	
	signal cpuWait		: std_logic;
	signal cpuReset		: std_logic := '0';
	
	signal cpuINT		: std_logic;
	signal cpuM1		: std_logic;
	signal cpuRFSH		: std_logic;
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
	
	signal vgaHSync		: std_logic;
	signal vgaVSync		: std_logic;
	signal vgaR			: std_logic_vector(7 downto 0);
	signal vgaG			: std_logic_vector(7 downto 0);
	signal vgaB			: std_logic_vector(7 downto 0);	
	
	signal tapeFifoWrTmp 	: std_logic_vector( 15 downto 0 );
	signal tapeFifoWr		: std_logic;
	signal tapeFifoRdTmp 	: std_logic_vector( 15 downto 0 );
	signal tapeFifoRd		: std_logic;
	
	signal tapeFifoReady	: std_logic;
	signal tapeFifoFull		: std_logic;
	
	signal trdosFifoReadWrTmp 	: std_logic_vector( 7 downto 0 );
	signal trdosFifoReadWr		: std_logic;
	signal trdosFifoReadRdTmp 	: std_logic_vector( 7 downto 0 );
	signal trdosFifoReadRd		: std_logic;
	
	signal trdosFifoReadRst		: std_logic;
	signal trdosFifoReadReady	: std_logic;
	signal trdosFifoReadFull	: std_logic;
	
	signal trdosFifoWriteWrTmp 	: std_logic_vector( 7 downto 0 );
	signal trdosFifoWriteWr		: std_logic;
	signal trdosFifoWriteRdTmp 	: std_logic_vector( 7 downto 0 );
	signal trdosFifoWriteRd		: std_logic;
	
	signal trdosFifoWriteRst		: std_logic;
	signal trdosFifoWriteReady	: std_logic;
	signal trdosFifoWriteFull	: std_logic;
	signal trdosFifoWriteCounter	: unsigned( 10 downto 0 ) := "00000000000";

begin

	U00 : entity work.pll
		port map(
			inclk0 => CLK_20,
			c0     => memclk,
			c1     => sysclk		
		);
		
	
	U01 : entity work.t80se
		port map(
			RESET_n => not ( cpuReset or reset ),
			CLK_n   => sysclk,
			CLKEN   => cpuCLK,
			WAIT_n  => '1',
			INT_n   => cpuINT,
			NMI_n   => '1',
			BUSRQ_n => '1',
			M1_n    => cpuM1,
			MREQ_n  => cpuMREQ,
			IORQ_n  => cpuIORQ,
			RD_n    => cpuRD,
			WR_n    => cpuWR,
			RFSH_n  => cpuRFSH,
			HALT_n  => open,
			BUSAK_n => open,
			A       => cpuA,
			--D       => cpuDout,
			DO      => cpuDout,
			DI      => cpuDin,
			
			SavePC => cpuSavePC,
			SaveINT => cpuSaveINT,
			RestorePC => cpuRestorePC,
			RestoreINT => cpuRestoreINT,
			
			RestorePC_n => cpuRestorePC_n
		);				
		
	U02 : entity work.YM2149
		port map(
			
			RESET_L => not ( cpuReset or reset ),
			CLK     => sysclk,
			ENA		=> ayCLK and not cpuHaltAck,
			I_SEL_L => '1',
			
			I_DA    => ayDin,
			O_DA    => ayDout,
			
			busctrl_addr => ayADDR,
			busctrl_we => ayWR,
			busctrl_re => ayRD,
			
			O_AUDIO	=> ayOUT,
			O_AUDIO_A	=> ayOUT_A,
			O_AUDIO_B	=> ayOUT_B,
			O_AUDIO_C	=> ayOUT_C
		);				
		
	U03 : entity work.vencode
		generic map(
			freq => sysFreq
		)
		
		port map(
			clk => sysclk,
			clk_en => '1',
			reset => '0',
			videoR => rgbR(7 downto 2),
			videoG => rgbG(7 downto 2),
			videoB => rgbB(7 downto 2),
			videoHS_n => VideoHS_n,
			videoVS_n => VideoVS_n,
			videoPS_n => palSync,
			videoY => videoY,
			videoC => videoC,
			videoV => videoV
		);
		
	U04 : entity work.sdram
		generic map(
			freq => memFreq
		)
		
		port map(
			clk => memclk,
			reset => reset,
			refresh	=> refresh,
			
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
		
	U05: entity work.ps2fifo
		port map(
			clk_i => memclk,
			rst_i => reset,

			data_o => keysDataOut,
			data_i => keysDataIn,
			
			rd_i => keysRd,
			wr_i => keysWr,
			
			out_ready_o => keysReady,
			out_full_o => keysFull,
			
			in_full_o => open,

			ps2_clk_io => KEYS_CLK,
			ps2_data_io => KEYS_DATA
		);
			
	U06: entity work.ps2fifo
		port map(
			clk_i => memclk,
			rst_i => reset,

			data_o => mouseDataOut,
			data_i => mouseDataIn,
			
			rd_i => mouseRd,
			wr_i => mouseWr,
			
			out_ready_o => mouseReady,
			out_full_o => mouseFull,
			
			in_full_o => open,

			ps2_clk_io => MOUSE_CLK,
			ps2_data_io => MOUSE_DATA
		);
		
	U07 : entity work.fifo
		generic map(
			fifo_width => 16,
			fifo_depth => 1024 
		)
		
		port map(
			clk_i => memclk,
			rst_i => reset,

			data_o => tapeFifoRdTmp,
			data_i => tapeFifoWrTmp,

			wr_i => tapeFifoWr,
			rd_i => tapeFifoRd,

			out_ready_o => tapeFifoReady,
			in_full_o => tapeFifoFull
		);

	U08 : entity work.fifo
		generic map(
			fifo_width => 8,
			fifo_depth => 1024 
		)
		
		port map(
			clk_i => memclk,
			rst_i => reset or trdosFifoReadRst,

			data_o => trdosFifoReadRdTmp,
			data_i => trdosFifoReadWrTmp,

			wr_i => trdosFifoReadWr,
			rd_i => trdosFifoReadRd,

			out_ready_o => trdosFifoReadReady,
			in_full_o => trdosFifoReadFull
		);

	U09 : entity work.fifo
		generic map(
			fifo_width => 8,
			fifo_depth => 1024 
		)
		
		port map(
			clk_i => memclk,
			rst_i => reset or trdosFifoWriteRst,

			data_o => trdosFifoWriteRdTmp,
			data_i => trdosFifoWriteWrTmp,

			wr_i => trdosFifoWriteWr,
			rd_i => trdosFifoWriteRd,

			out_ready_o => trdosFifoWriteReady,
			in_full_o => trdosFifoWriteFull
		);

	    process( sysclk )
		
		variable iCLK_20 : std_logic_vector( 1 downto 0 ) := "11";
		variable counter20_en_prev : std_logic := '0';
		
	begin
	
		if sysclk'event and sysclk = '1' then
			
		if counter20_en = '1' and iCLK_20 = "01" then
				if counter20_en_prev = '0' then
					counter20 <= x"00000000";
				else
					counter20 <= counter20 + 1;
				end if;
			end if;
			
			counter20_en_prev := counter20_en;
			iCLK_20 := iCLK_20(0) & CLK_20;
			
		end if;
		
	end process;
				
    process( sysclk )
    
		variable divCounter28 : unsigned(15 downto 0) := x"0000";
		variable mulCounter28 : unsigned(7 downto 0) := x"00";
		
		variable counterMem_en_prev : std_logic := '0';
		
		variable freqDiv : integer;
		
    begin
    
        if sysclk'event and sysclk = '1' then
		
			if counterMem_en = '1' then
				if counterMem_en_prev = '0' then
					counterMem <= x"00000000";
				else
					counterMem <= counterMem + 2;
				end if;
			end if;
			
			counterMem_en_prev := counterMem_en;
			
			divCounter28 := divCounter28 + 4;
			
			clk28m <= '0';
			clk14m <= '0';
			clk7m <= '0';
			clk35m <= '0';
			ayCLK <= '0';
			
			freqDiv := 6;
			
			if videoMode = 3 then
				freqDiv := 5;
			elsif videoMode = 4 then
				freqDiv := 4;				
			end if;
			
			if divCounter28 >= freqDiv then
			
				divCounter28 := divCounter28 - freqDiv;
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

--	romPage <= 	"010" when ( specTrdosFlag xor specTrdosToggleFlag ) = '1' else
--					"001" when specPort7ffd(4) = '1' else
--					"000";
	romPage <= 	"0" & not ( specTrdosFlag xor specTrdosToggleFlag ) & specPort7ffd(4);

	ramPage <= 	"00000101" when cpuA( 15 downto 14 ) = "01" else
				"00000010" when cpuA( 15 downto 14 ) = "10" else
				"00" & specPort7ffd( 7 downto 5 ) & specPort7ffd( 2 downto 0 ) when specMode = 2 else
				"00000" & specPort7ffd( 2 downto 0 );
					
				
	ayDin <= cpuDout;
	
    process( memclk )
    
		variable addressReg : unsigned(23 downto 0) := "000000000000000000000000";

		variable ArmWr	: std_logic_vector(1 downto 0);
		variable ArmRd	: std_logic_vector(1 downto 0);
		variable ArmAle	: std_logic_vector(1 downto 0);
		    
		variable armReq : std_logic := '0';
		variable rtcRamReq : std_logic := '0';
		
		variable portFfReq : std_logic := '0';
		
		variable kbdTmp : std_logic_vector(4 downto 0);
		variable cpuReq : std_logic;		
		
		type rtcRamType is array ( 0 to 63 ) of std_logic_vector( 7 downto 0 );
		variable rtcRam : rtcRamType;
		
		variable rtcRamAddressRd : unsigned(7 downto 0) := x"00";
		variable rtcRamDataRd : std_logic_vector(7 downto 0) := x"00";
		variable rtcRamDataRdPrev : std_logic_vector(7 downto 0) := x"00";
		
		variable iCpuWr : std_logic_vector(1 downto 0);
		variable iCpuRd : std_logic_vector(1 downto 0);		
		
    begin

		if memclk'event and memclk = '1' then
        
			if reset = '1' then				
				cpuReset <= '0';
				cpuHaltReq <= '0';
			end if;		
        
			rtcRamDataRd := rtcRamDataRdPrev;
			rtcRamDataRdPrev := rtcRam( to_integer( rtcRamAddressRd ) );
			
			if cpuReset = '1' then
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
			
			------------------------------------------------------------------------
			
			keysRd <= '0';
			keysWr <= '0';			        
			mouseRd <= '0';
			mouseWr <= '0';
			
			tapeFifoWr <= '0';
			
			trdosFifoReadRd <= '0';
			trdosFifoReadWr <= '0';
			trdosFifoReadRst <= '0';
			
			trdosFifoWriteRd <= '0';
			trdosFifoWriteWr <= '0';
			trdosFifoWriteRst <= '0';

			----------------------------------------------------------------------------------
			
			if ulaWait /= '1' and cpuTurbo = 3 then
				iCpuWr := iCpuWr(0) & cpuWR;
				iCpuRd := iCpuRd(0) & cpuRD;     
			end if;

			----------------------------------------------------------------------------------

			if cpuRD /= '0' then
				cpuDin <= x"ff";
				portFfReq := '0';
				ayRD <= '0';				
			elsif portFfReq = '1' then
				cpuDin <= specPortFf;
			elsif ayRD = '1' then
				cpuDin <= ayDout;
			end if;

			if cpuWR /= '0' then
				ayADDR <= '0';
				ayWR <= '0';
			end if;

			--if cpuCLK_prev = '1' and cpuWR = '0' then
			if iCpuWr = "10" then
				
				if cpuMREQ = '0' then
					
					if cpuA( 15 downto 14 ) /= "00" then
						memAddress <= "000" & ramPage & cpuA( 13 downto 1 );
					
						memDataIn <= cpuDout & cpuDout;
						memDataMask(0) <= not cpuA(0);
						memDataMask(1) <= cpuA(0);
						memWr <= '1';
						
						cpuReq := '1';
						memReq <= '1';
						
						cpuMemoryWait <= '1';
						
					end if;
					
				elsif cpuIORQ = '0' and cpuM1 = '1' then
				
					if specTrdosFlag = '1' and cpuA( 4 downto 0 ) = "11111" then 									
			
						if cpuA( 7 downto 0 ) = x"7F" and trdosFifoWriteCounter > 0 then
							trdosFifoWriteWrTmp <= cpuDout;
							trdosFifoWriteWr <= '1';							
							trdosFifoWriteCounter <= trdosFifoWriteCounter - 1;
						else
							specTrdosWait <= '1';
							specTrdosWr <= '1';
						end if;
						
					else
					
						if cpuA( 7 downto 0 ) = x"FE" then
							specPortFe <= cpuDout;
						elsif cpuA( 15 ) = '0' and cpuA( 7 downto 0 ) = x"fd" then
							if specMode = 2 or specPort7ffd(5) = '0' then
								specPort7ffd <= cpuDout;
							end if;
						elsif cpuA = x"eff7" then	
							specPortEff7 <= cpuDout;
						elsif cpuA = x"dff7" and specPortEff7(7) = '1' then	
							--specPortDff7 <= cpuDout;
							rtcRamAddressRd := unsigned(cpuDout);
						--elsif cpuA = x"bff7" and specPortEff7(7) = '1' then	
							--rtcRam( to_integer( unsigned( specPortDff7 ) ) ) := cpuDout;
						elsif ayMode /= 0 and cpuA( 15 downto 14 ) = "11" and cpuA(1 downto 0) = "01" then
							ayADDR <= '1';
						elsif ayMode /= 0 and cpuA( 15 downto 14 ) = "10" and cpuA(1 downto 0) = "01" then
							ayWR <= '1';
														
						end if;
						
					end if;
					
				end if;
				
			--elsif cpuCLK_prev = '1' and cpuRD = '0' then
			elsif iCpuRd = "10" then
				
				cpuDin <= x"FF";
				
				if cpuMREQ = '0' then
					
					if cpuA( 15 downto 14 ) = "00" then
						memAddress <= "001" & "00000" & romPage & cpuA( 13 downto 1 );
					else
						memAddress <= "000" & ramPage & cpuA( 13 downto 1 );
					end if;						
					
					memWr <= '0';
					
					cpuReq := '1';
					memReq <= '1';
					
					cpuMemoryWait <= '1';
					
				elsif cpuIORQ = '0' and cpuM1 = '1' then

					if cpuA( 7 downto 0 ) = x"FE" then
						kbdTmp := ( keyboard(  4 downto  0 ) or ( cpuA(8) & cpuA(8) & cpuA(8) & cpuA(8) & cpuA(8) ) )
								and ( keyboard(  9 downto  5 ) or ( cpuA(9) & cpuA(9) & cpuA(9) & cpuA(9) & cpuA(9) ) )
								and ( keyboard( 14 downto 10 ) or ( cpuA(10) & cpuA(10) & cpuA(10) & cpuA(10) & cpuA(10) ) )
								and ( keyboard( 19 downto 15 ) or ( cpuA(11) & cpuA(11) & cpuA(11) & cpuA(11) & cpuA(11) ) )
								and ( keyboard( 24 downto 20 ) or ( cpuA(12) & cpuA(12) & cpuA(12) & cpuA(12) & cpuA(12) ) )
								and ( keyboard( 29 downto 25 ) or ( cpuA(13) & cpuA(13) & cpuA(13) & cpuA(13) & cpuA(13) ) )
								and ( keyboard( 34 downto 30 ) or ( cpuA(14) & cpuA(14) & cpuA(14) & cpuA(14) & cpuA(14) ) )
								and ( keyboard( 39 downto 35 ) or ( cpuA(15) & cpuA(15) & cpuA(15) & cpuA(15) & cpuA(15) ) );
						cpuDin <= "1" & tapeIn & "1" & kbdTmp;
						
					elsif cpuA( 15 downto 0 ) = x"FADF" then
						cpuDin <= mouseButtons;
					elsif cpuA( 15 downto 0 ) = x"FBDF" then
						cpuDin <= mouseX;
					elsif cpuA( 15 downto 0 ) = x"FFDF" then
						cpuDin <= mouseY;
					--elsif cpuA = x"7ffd" then
						--cpuDin <= specPort7ffd;						
					elsif cpuA = x"bff7" and specPortEff7(7) = '1' then
						--cpuDin <= rtcRam( to_integer( unsigned( specPortDff7( 5 downto 0 ) ) ) );
						cpuDin <= rtcRamDataRd;
						
					elsif specTrdosFlag = '1' and cpuA( 4 downto 0 ) = "11111" then
						
						if cpuA( 7 downto 0 ) = x"FF" then 
						
							cpuDin <= specTrdosPortFF;
						
						elsif cpuA( 7 downto 0 ) = x"7F" and trdosFifoReadReady = '1' then 
						
							cpuDin <= trdosFifoReadRdTmp;
							trdosFifoReadRd <= '1';
						
						else
				
							specTrdosWait <= '1';
							specTrdosWr <= '0';
						
						end if;
						
					else
					
						if cpuA( 7 downto 0 ) = x"1F" then
							cpuDin <= joystick;
						elsif ayMode /= 0 and cpuA( 15 downto 14 ) = "11" and cpuA(1 downto 0) = "01" then
							ayRD <= '1';
						elsif cpuA( 7 downto 0 ) = x"FF" then
							portFfReq := '1';
						end if;
											
					end if;
					
				end if;
				
			elsif armReq = '1' and ARM_WR = '0' then
				
				armReq := '0';
				
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
						--elsif addressReg( 7 downto 0 ) = x"09" then
							--cpuBP0_en <= ARM_AD(0);
						--elsif addressReg( 7 downto 0 ) = x"0A" then
							--cpuBP0 <= ARM_AD;
							
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
							tapeFifoWrTmp <= ARM_AD;
							tapeFifoWr <= '1';
							
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
							specTrdosPortFF <= ARM_AD( 7 downto 0 );
							
						elsif addressReg( 7 downto 0 ) = x"60" then
						
							if ARM_AD( 15 ) = '1' then						
								trdosFifoReadRst <= '1';							
							else						
								trdosFifoReadWrTmp <= ARM_AD( 7 downto 0 );
								trdosFifoReadWr <= '1';								
							end if;
							
						elsif addressReg( 7 downto 0 ) = x"61" then
						
							trdosFifoWriteRst <= ARM_AD( 15 );
							trdosFifoWriteCounter <= unsigned( ARM_AD( 10 downto 0 ));
						
						elsif addressReg( 7 downto 0 ) = x"1e" then							
							mouseX <= ARM_AD( 7 downto 0 );
							mouseY <= ARM_AD( 15 downto 8 );
						elsif addressReg( 7 downto 0 ) = x"1f" then							
							mouseButtons <= ARM_AD( 7 downto 0 );

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
							specMode <= unsigned( ARM_AD(7 downto 0) );
						elsif addressReg( 7 downto 0 ) = x"41" then
							syncMode <= unsigned( ARM_AD(7 downto 0) );
						elsif addressReg( 7 downto 0 ) = x"42" then
							cpuTurbo <= unsigned( ARM_AD(7 downto 0) );
						elsif addressReg( 7 downto 0 ) = x"43" then
							videoMode <= unsigned( ARM_AD(7 downto 0) );
						--elsif addressReg( 7 downto 0 ) = x"44" then
							--subcarrierDelta( 15 downto 0 ) <= unsigned( ARM_AD( 15 downto 0 ) );
						--elsif addressReg( 7 downto 0 ) = x"45" then
							--subcarrierDelta( 23 downto 16 ) <= unsigned( ARM_AD( 7 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"46" then
							dacMode <= unsigned( ARM_AD(7 downto 0) );
						elsif addressReg( 7 downto 0 ) = x"47" then
							ayMode <= unsigned( ARM_AD(7 downto 0) );
						elsif addressReg( 7 downto 0 ) = x"48" then
							videoAspectRatio <= unsigned( ARM_AD(7 downto 0) );
						elsif addressReg( 7 downto 0 ) = x"49" then
							testVideo <= unsigned( ARM_AD(7 downto 0) );							

						elsif addressReg( 7 downto 0 ) = x"50" then
							counter20_en <= ARM_AD(0);
							counterMem_en <= ARM_AD(0);
							
						end if;
						
					elsif addressReg( 23 downto 8 ) = x"c001" then																	
						rtcRam( to_integer( addressReg( 7 downto 0 ) ) ) := ARM_AD( 7 downto 0 );
						
					end if;
					
					ARM_WAIT <= '1';
				end if;
				
			elsif armReq = '1' and ARM_RD = '0' then
				
				armReq := '0';

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
							
						elsif addressReg( 7 downto 0 ) = x"15" then
							ARM_AD <= x"00" & "0000000" & tapeFifoFull;
							
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
						elsif addressReg( 7 downto 0 ) = x"61" then
							ARM_AD <= trdosFifoWriteReady & "0000000" & trdosFifoWriteRdTmp;
							if trdosFifoWriteReady = '1' then
								trdosFifoWriteRd <= '1';
							end if;
							
						elsif addressReg( 7 downto 0 ) = x"30" then
							ARM_AD <= joyCode0;
						elsif addressReg( 7 downto 0 ) = x"31" then
							ARM_AD <= joyCode1;
						elsif addressReg( 7 downto 0 ) = x"32" then
							ARM_AD <= keysReady & keysFull & "000000" & keysDataOut;
							if keysReady = '1' then
								keysRd <= '1';
							end if;
						elsif addressReg( 7 downto 0 ) = x"33" then
							ARM_AD <= mouseReady & mouseFull & "000000" & mouseDataOut;
							if mouseReady = '1' then								
								mouseRd <= '1';
							end if;
							
						elsif addressReg( 7 downto 0 ) = x"50" then
							ARM_AD <= std_logic_vector( counter20( 15 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"51" then
							ARM_AD <= std_logic_vector( counter20( 31 downto 16 ) );
						elsif addressReg( 7 downto 0 ) = x"52" then
							ARM_AD <= std_logic_vector( counterMem( 15 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"53" then
							ARM_AD <= std_logic_vector( counterMem( 31 downto 16 ) );
							
						else
							ARM_AD <= x"ffff";							
						end if;					
						
						ARM_WAIT <= '1';
						
					elsif addressReg( 23 downto 8 ) = x"c001" then
						
						--ARM_AD <= x"00" & rtcRam( to_integer( addressReg( 5 downto 0 ) ) );
						--rtcRamReq := '1';						
						
					else
					
						ARM_AD <= x"ffff";
						ARM_WAIT <= '1';
						
					end if;					
					
				end if;
				
			end if;			
			
			---------------------------------------------------------
			
			if ulaWait /= '1' then
				iCpuWr := iCpuWr(0) & cpuWR;
				iCpuRd := iCpuRd(0) & cpuRD;     
			end if;

			if ( ArmAle = "10" ) then
				addressReg := unsigned( ARM_A ) & unsigned( ARM_AD );
				
			elsif ( ArmWr = "10" ) then
				armReq := '1';
			
			elsif ( ArmWr = "01" ) then
				ARM_WAIT <= '0';
				
			elsif ( ArmRd = "10" ) then
				armReq := '1';
				
			elsif ( ArmRd = "01" ) then			
				ARM_AD <= (others => 'Z');
				ARM_WAIT <= '0';
				
			end if;			
			
			ArmAle := ArmAle(0) & ARM_ALE;
			ArmWr := ArmWr(0) & ARM_WR;
			ArmRd := ArmRd(0) & ARM_RD;

        end if;
        
    end process;
    
	--cpuDout <= cpuDin when cpuRD = '0' else "ZZZZZZZZ";
	
	------------------------------------------------------------------------------
	
	process(sysclk)
	
		variable cpuSaveInt7_prev : std_logic := '0';

	begin
		if sysclk'event and sysclk = '1' then
		
			if reset = '1' then				
				cpuHaltAck <= '0';
			end if;		
	
			if cpuTraceReq = '1' then
				
				cpuHaltAck <= '0';
				
			elsif cpuHaltAck = '0' and cpuSaveINT( 7 ) = '0' and cpuSaveInt7_prev = '1' then

				if cpuHaltReq = '1' then
					cpuHaltAck <= '1';
				--elsif cpuBP0_en = '1' and cpuSavePC = cpuBP0 then
					--cpuHaltAck <= '1';
				end if;			
								
			end if;
			
			cpuSaveInt7_prev := cpuSaveINT( 7 );
			
		end if;
		
	end process;
	
	process(sysclk)
	
		variable specTrdosPreCounter : unsigned( 15 downto 0 ) := x"0000";
		variable waitCounter : std_logic := '0';

	begin
				
		if sysclk'event and sysclk = '1' then
		
			if reset = '1' then				
				specTrdosPreCounter := x"0000";
				specTrdosCounter <= x"0000";
			end if;		
		
			if waitCounter = '0' then
				
				specTrdosPreCounter := specTrdosPreCounter + 1;
				
				if specTrdosPreCounter >= sysFreq * 10 then
				
					specTrdosPreCounter := x"0000";
					specTrdosCounter <= specTrdosCounter + 1;
					
				end if;
				
			end if;
			
			if cpuCLK_nowait = '1' then
				waitCounter := cpuWait;
			end if;
			
		end if;                                                      
	end process;
	
	---------------------------------------------------------------------------------------

    process( memclk )
    
		variable tapeCounter		: unsigned( 14 downto 0 ) := "000000000000000";
		variable tapePulse			: std_logic := '1';
		
    begin

		if memclk'event and memclk = '1' then
		
			if sysclk = '1' and tapeCounter > 0 then 
			
				if tapePulse = '1' then
					
					if cpuCLK_nowait = '1' and cpuHaltAck = '0' and cpuMemoryWait = '0' then
				
						tapeCounter := tapeCounter - 1;
					
						if tapeCounter = 0 then
							tapeIn <= not tapeIn;
						end if;
					
					end if;
					
				else
				
					if clk35m = '1' then
				
						tapeCounter := tapeCounter - 1;
						
						if tapeCounter = 0 then
							tapeIn <= '0';
						end if;
						
					end if;				
				
				end if;
			
			end if;
			
			tapeFifoRd <= '0';

			if tapeCounter = 0 then
		
				if tapeFifoReady = '1' then
				
					tapeCounter := unsigned( tapeFifoRdTmp( 14 downto 0 ) );
					tapePulse := tapeFifoRdTmp( 15 );
					tapeFifoRd <= '1';
					
				else
			
					tapeCounter := "111111111111111";
					tapePulse := '0';
					
				end if;
					
			end if;			
			
		end if;
		
	end process;
		
	---------------------------------------------------------------------------------------

    paper <= '0' when Hor_Cnt(5) = '0' and Ver_Cnt(5) = '0' and ( Ver_Cnt(4) = '0' or Ver_Cnt(3) = '0' ) else '1';      
	hsync <= '0' when Hor_Cnt(5 downto 2) = "1010" else '1';
	vsync1 <= '0' when Hor_Cnt(5 downto 1) = "00110" or Hor_Cnt(5 downto 1) = "10100" else '1';
	vsync2 <= '1' when Hor_Cnt(5 downto 2) = "0010" or Hor_Cnt(5 downto 2) = "1001" else '0';
	
	ulaWaitMem <= '0' when hCnt(3 downto 1) = "000" or hCnt(3 downto 1) = "111" else '1';
	ulaWaitIo <= '0' when hCnt(3 downto 2) = "11" else '1';
	
	--ulaWaitMem <= '0';
	--ulaWaitIo <= '0';
	
	ulaContendentMem <= '1' when cpuA( 15 downto 14 ) = "01" or ( cpuA( 15 downto 14 ) = "11" and specPort7ffd( 0 ) = '1' ) else '0';

	ulaWait <= '1' when ( cpuTurbo = 0 and syncMode <= 1 ) and 
		paper = '0' and
		(
			( cpuMREQ = '0' and ulaContendentMem = '1' and ulaWaitMem = '1' and ulaWaitCancel = '0' )
			or 
			( ( cpuIORQ = '0' and cpuA( 0 ) = '0' ) and ulaWaitIo = '1' and ulaWaitCancel ='0' )
			or
			--( cpuIORQ = '0' and ( cpuA( 15 downto 14 ) = "01" ) and ulaWaitIo = '1' and ulaWaitCancel = '0' )
			( cpuMREQ = '1' and ( cpuA( 15 downto 14 ) = "01" ) and ulaWaitIo = '1' and ulaWaitCancel = '0' )
		) else '0';
		
	cpuWait <= cpuHaltAck or cpuMemoryWait or specTrdosWait or cpuOneCycleWaitReq or ulaWait;
	
	ChrC_Cnt <= hCnt( 2 downto 0 );
	Hor_Cnt <= hCnt( 8 downto 3 );
	
	ChrR_Cnt <= vCnt( 2 downto 0 );
	Ver_Cnt <= vCnt( 8 downto 3 );
	
	cpuCLK_nowait <= clk7m and not ChrC_Cnt(0) when cpuTurbo = 0 else
					clk7m when cpuTurbo = 1 else
					clk14m when cpuTurbo = 2 else
					clk28m;
					
	cpuCLK <= cpuCLK_nowait and not cpuWait;

	process( sysclk )
	
		variable cpuIntSkip : std_logic := '0';
		
	begin
		if sysclk'event and sysclk = '1' then
		
			refresh <= '0';
				
			if cpuReset = '1' or cpuHaltAck = '1' or specTrdosWait = '1' or ( cpuMREQ = '0' and cpuRFSH = '0' ) then
				refresh <= '1';
			end if;
			
			cpuCLK_nowait_prev <= cpuCLK_nowait;
		
			if cpuCLK_nowait = '1' then
				
				if specIntCounter = 32 then
					cpuINT <= '1';
				end if;

				if cpuHaltAck = '0' and specTrdosWait = '0' then
					specIntCounter <= specIntCounter + 1;
				end if;
				
				cpuOneCycleWaitAck <= cpuOneCycleWaitReq;
				
			end if;
			
			if cpuCLK = '1' then
				
				if ( cpuIORQ = '0' and cpuA( 15 downto 14 ) /= "01" ) or cpuMREQ = '0' then
					ulaWaitCancel <= '1';
				else
					ulaWaitCancel <= '0';
				end if;
					
				cpuTraceAck <= cpuTraceReq;
				
			end if;
			
			---------------------------------------------------------------------
				
			if syncMode = 1 then
				hSize <= 456;
				vSize <= 311;
			elsif syncMode = 2 then
				hSize <= 448;
				vSize <= 320;
			else
				hSize <= 448;
				vSize <= 312;
			end if;

			if clk7m = '1' then
			
				if hCnt >= hSize - 1 then
					hCnt <= x"0000";
				else
					hCnt <= hCnt + 1;
				end if;				
					
				if hCnt = 319 then					
				
					if vCnt >= vSize - 1 then
						vCnt <= x"0000";
						Invert <= Invert + 1;
					else
						vCnt <= vCnt + 1;
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
				
				if syncMode = 2 then
					--if vCnt = 239 and hCnt = 316 then
					if vCnt = 240 and hCnt = 320 then
						cpuINT <= cpuIntSkip;
						specIntCounter <= x"00000000";
						cpuIntSkip := '0';
					end if;
				else
					if vCnt = 248 and hCnt = 444 then
					--if vCnt = 248 and hCnt = 442 then
						cpuINT <= cpuIntSkip;
						specIntCounter <= x"00000000";
						cpuIntSkip := '0';
					end if;
				end if;
				
				--if cpuHaltAck = '1' or specTrdosWait = '1' then
				if cpuHaltAck = '1' then
					cpuINT <= '1';
					cpuIntSkip := '1';
				end if;						

			end if;
			
		end if;
	end process;
	
	------------------------------------------------------------------------------------------
	
	vramPage <= "000001" & specPort7ffd(3) & "1";
		
	process( memclk )
		variable videoPage : std_logic_vector(10 downto 0);

		variable portFfTemp : std_logic_vector(7 downto 0) := x"FF";
		
		variable vidReadPos : unsigned( 3 downto 0 ) := x"0";
		variable vidReq : std_logic := '0';
		variable vidReqUla : std_logic := '0';		
		
	begin
		if memclk'event and memclk = '1' then
			
			if cpuCLK_nowait = '1' then
				--specPortFf <= x"ff";
				--specPortFf <= std_logic_vector( Hor_Cnt( 3 downto 0) & "0" & ChrC_Cnt );				
				portFfTemp := x"ff";
			end if;				
		
			if Paper = '0' then

				if vidReq = '0' and Hor_Cnt(0) = '0' then
					
					if ( vidReqUla = '0' and ChrC_Cnt = 0 ) or
						( vidReqUla = '1' and ChrC_Cnt = 2 ) then						
					--if ChrC_Cnt = 0 then
						
						vidReadPos := x"0";
						vidReq := '1';

					end if;

				end if;
				
				if vidReq = '1' and memReq2 = '0' and ( ( videoMode = x"04" and vidReadPos(0) = '1' ) or cpuCLK_nowait_prev = '1' ) then
					
					memWr2 <= '0';
					memReq2 <= '1';
					
					if vidReadPos = 0 or vidReadPos = 2 then
						memAddress2 <= videoPage & std_logic_vector( "0" & Ver_Cnt(4 downto 3) & ChrR_Cnt & Ver_Cnt(2 downto 0) & Hor_Cnt(4 downto 1) );
					else
						memAddress2 <= videoPage & std_logic_vector( "0110" & Ver_Cnt(4 downto 0) & Hor_Cnt(4 downto 1) );
					end if;					

				elsif memAck2 = '1' then
				
					memReq2 <= '0';
					vidReq := '0';

					if vidReadPos = 0  then
						Shift <= memDataOut2;
						portFfTemp := memDataOut2( 7 downto 0 );						
						vidReq := '1';
						
					elsif vidReadPos = 1  then
						Attr <= memDataOut2;
						portFfTemp := memDataOut2( 7 downto 0 );
						if cpuTurbo = 0 then
							vidReq := '1';
						end if;						
						
					elsif vidReadPos = 2  then
						Shift( 15 downto 8 ) <= memDataOut2( 15 downto 8 );
						portFfTemp := memDataOut2( 15 downto 8 );
						vidReq := '1';
						
					else
						Attr( 15 downto 8 ) <= memDataOut2( 15 downto 8 );
						portFfTemp := memDataOut2( 15 downto 8 );
						
					end if;
					
					vidReadPos := vidReadPos + 1;
				
				end if;					
					
			end if;
		
			if syncMode >= 2 or cpuTurbo /= 0 then
				vidReqUla := '0';
			else
				vidReqUla := '1';
			end if;			
			
			if armVideoPage(15) = '1' then
				videoPage := armVideoPage( 10 downto 0 );
			else
				videoPage := "000" & vramPage;
			end if;
			
			specPortFf <= portFfTemp;

		end if;
		
	end process;
	
	process( sysclk, clk7m )
	
		variable hBorderSize : integer := 0;
		variable vBorderSize : integer := 0;
		
		variable hPosBegin : integer := 0;
		variable hPosEnd : integer := 0;
		variable vPosBegin : integer := 0;
		variable vPosEnd : integer := 0;
		
	begin
	
		if sysclk'event and sysclk = '1' and clk7m = '1' then
			if ChrC_Cnt = 7 then
				
				if Hor_Cnt(0) = '0' then
					Attr_r <= Attr( 7 downto 0 );
					Shift_r <= Shift( 7 downto 0 );
				else
					Attr_r <= Attr( 15 downto 8 );
					Shift_r <= Shift( 15 downto 8 );
				end if;
				
				paper_r <= paper;
				
			else
				Shift_r <= Shift_r(6 downto 0) & "0";					
			end if;
			
				
			if ChrC_Cnt = 7 then
				if videoMode >= 3 then
					hBorderSize := 48;
					vBorderSize := 54;
				elsif videoAspectRatio = 0 then 
					hBorderSize := 6 * 8;
					vBorderSize := 5 * 8;
				elsif videoAspectRatio = 1 then 
					hBorderSize := 6 * 8;
					vBorderSize := 6 * 8;
				else 
					hBorderSize := 8 * 8;
					vBorderSize := 2 * 8;
				end if;

				hPosBegin := hSize - hBorderSize;
				hPosEnd := 256 + hBorderSize;
				vPosBegin := vSize - vBorderSize;
				vPosEnd := 192 + vBorderSize;

				if ( ( hCnt >= hPosEnd and hCnt < hPosBegin ) or 
						( vCnt >= vPosEnd and vCnt < vPosBegin ) ) then
							blank_r <= '0';
				else 
					blank_r <= '1';
				end if;
			end if;
				
		end if;
	end process;
	
	------------------------------------------------------------------------------------------------
	
	borderAttr <= specPortFe( 2 downto 0 );
	speaker <= specPortFe( 4 );

	process( sysclk, clk7m )
	begin
		if sysclk'event and sysclk = '1' and clk7m = '1' then
			if paper_r = '0' then
				if( Shift_r(7) xor ( Attr_r(7) and Invert(4) ) ) = '1' then
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
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT( 7 downto 0 ) & "000000" ) + unsigned( "00" & ayOUT( 7 downto 0 ) & "000000" ) );
					
	soundRight	<= sound when ayMode = 0 else
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT_C( 7 downto 0 ) & "0000000" ) + unsigned( "00" & ayOUT_B( 7 downto 0 ) & "000000" ) ) when ayMode = 1 else
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT_B( 7 downto 0 ) & "0000000" ) + unsigned( "00" & ayOUT_C( 7 downto 0 ) & "000000" ) ) when ayMode = 2 else
					std_logic_vector( unsigned( sound ) + unsigned( "0" & ayOUT( 7 downto 0 ) & "000000" ) + unsigned( "00" & ayOUT( 7 downto 0 ) & "000000" ) );

	SOUND_LEFT	<= soundLeft( 15 downto 8 ) when dacMode = 0 else
		tdaData & tdaWs & tdaBck & "00000";

	SOUND_RIGHT	<= soundRight( 15 downto 8 ) when dacMode = 0 else
		"00000000";

	process( sysclk, clk14m )
		variable outLeft : unsigned(15 downto 0) := x"0000";
		variable outRight : unsigned(15 downto 0) := x"0000";
		variable outData : unsigned(47 downto 0) := x"000000000000";
		
		variable leftDataTemp : unsigned(19 downto 0) := x"00000";
		variable rightDataTemp : unsigned(19 downto 0) := x"00000";
		
		variable tdaCounter : unsigned(7 downto 0) := "00000000";
		variable skipCounter : unsigned(7 downto 0) := x"00";
		
	begin
		if sysclk'event and sysclk = '1' and clk14m = '1' then
		
			if tdaCounter = 48 * 2 then
				tdaCounter := x"00";

				outRight := rightDataTemp( 19 downto 4 );
				rightDataTemp := x"00000";

				outLeft := leftDataTemp( 19 downto 4 );
				leftDataTemp := x"00000";
				
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
						tdaWs <= '1';
					elsif tdaCounter( 7 downto 1 ) = 24 + 7 then
						tdaWs <= '0';
					end if;
					
				else
			
					if tdaCounter( 7 downto 1 ) = 0 then
						tdaWs <= '0';
					elsif tdaCounter( 7 downto 1 ) = 24 then
						tdaWs <= '1';
					end if;

				end if;

				if skipCounter >= 2 then
							
					rightDataTemp := rightDataTemp + unsigned( soundRight );
					leftDataTemp := leftDataTemp + unsigned( soundLeft );
					skipCounter := x"00";					
					
				else
				
					skipCounter := skipCounter + 1;
					
				end if;
			
			end if;
		
			tdaBck <= tdaCounter(0);
			tdaCounter := tdaCounter + 1;
			
		end if;
	end process;
	
	------------------------------------------------------------------------------------------------
	
	process( sysclk, clk14m )
		variable vgaHCounter : unsigned(15 downto 0) := x"0000";
		variable vgaVCounter : unsigned(15 downto 0) := x"0000";
		variable vgaHPorch	: std_logic;
		variable vgaVPorch	: std_logic;
		
		type doubleByfferType is array ( 0 to 1023 ) of std_logic_vector( 23 downto 0 );
		variable doubleByffer : doubleByfferType;
		
		variable bufferReadAddress : unsigned(9 downto 0) := "0000000000";
		variable bufferWriteAddress : unsigned(9 downto 0) := "0000000000";
		
		variable temp : std_logic_vector(23 downto 0) := x"000000";
		
		variable prevH : std_logic := '0';
		variable prevV : std_logic := '0';
		
		variable vgaHSize : integer;
		variable vgaVSize : integer;
		
	begin
		if sysclk'event and sysclk = '1' and clk14m = '1' then
		
			vgaR <= temp( 23 downto 16 );
			vgaG <= temp( 15 downto 8 );
			vgaB <= temp( 7 downto 0 );
			
			------------------------------------------------------------------------------
			
			if vgaHCounter = 0 and vgaVCounter( 0 ) = '1' then
				bufferWriteAddress := vgaVCounter( 1 ) & "000000000";
			elsif vgaHCounter( 0 ) = '0' then
				bufferWriteAddress := bufferWriteAddress + 1;
			end if;
			
			if vgaHCounter = 0 then
				bufferReadAddress := ( vgaVCounter( 1 ) & "000000000" ) xor "1000000000";
			else
				bufferReadAddress := bufferReadAddress + 1;
			end if;

			doubleByffer( to_integer( bufferWriteAddress ) ) := rgbR & rgbG & rgbB;
			temp := doubleByffer( to_integer( bufferReadAddress ) );
			
			------------------------------------------------------------------------------

			vgaHSize := hSize;
			vgaVSize := vSize * 2;
			
			vgaHCounter := vgaHCounter + 1;
			
			if vgaHCounter >= vgaHSize then
			
				vgaHCounter := x"0000";				
				vgaVCounter := vgaVCounter + 1;				
				
				if vgaVCounter >= vgaVSize then
					vgaVCounter := x"0000";										
				end if;
			
			end if;
			
			if prevH = '1' and VideoHS_n = '0' then
				vgaHCounter := to_unsigned( vgaHSize - 8, 16 );
			end if;
			
			if vgaHCounter < 40 then
				vgaHSync <= '0';
			else
				vgaHSync <= '1';			
			end if;
			
			if prevV = '1' and VideoVS_n = '0' then
				if videoAspectRatio = 1 or videoMode /= 2 then
					vgaVCounter := to_unsigned( 0, 16 );
				elsif videoAspectRatio = 0 then
					vgaVCounter := to_unsigned( vgaVSize - 16, 16 );
				else
					vgaVCounter := to_unsigned( vgaVSize - 64, 16 );
				end if;
			end if;

			if vgaVCounter < 2 then
				vgaVSync <= '0';
			else
				vgaVSync <= '1';			
			end if;			
			
			prevH := VideoHS_n;
			prevV := VideoVS_n;
			
		end if;
	end process;

	process( sysclk )
		
	begin
		if sysclk'event and sysclk = '1' then
			if clk7m = '1' then
				if testVideo = 0 then
					if specY = '0' then
				
						rgbR <= specR & specR & "000000";
						rgbG <= specG & specG & "000000";
						rgbB <= specB & specB & "000000";
					
					else
				
						rgbR <= specR & specR & specR & specR & specR & specR & specR & specR;
						rgbG <= specG & specG & specG & specG & specG & specG & specG & specG;
						rgbB <= specB & specB & specB & specB & specB & specB & specB & specB;
						
					end if;
				
				else
				
					rgbR <= x"00";
					rgbG <= x"00";
					rgbB <= x"00";
					
					if Paper = '0' then
						
						if testVideo( 0 ) = '1' then rgbB <= std_logic_vector( hCnt( 7 downto 0 ) ); end if;
						if testVideo( 1 ) = '1' then rgbR <= std_logic_vector( hCnt( 7 downto 0 ) ); end if;
						if testVideo( 2 ) = '1' then rgbG <= std_logic_vector( hCnt( 7 downto 0 ) ); end if;
					
					elsif Blank_r = '1' and ( hCnt(3) xor vCnt(3) ) = '1' then

						rgbR <= x"FF";
						rgbG <= x"FF";
						rgbB <= x"FF";
						
					end if;
					
				end if;
			end if;
			
		end if;
			
	end process;

	VIDEO_R <= videoV when videoMode = 0 else 						-- ( Composite --> SCART 20, 17 )
			"0" & rgbR( 7 downto 1 ) when videoMode = 1 else		-- ( VGA 1, 6  --> SCART 15, 13 )
			"0" & vgaR( 7 downto 1 );
	VIDEO_G <= "0" & videoC( 7 downto 1 ) when videoMode = 0 else 	-- ( S-Video 4, 2 --> SCART 15, 13 )
			"0" & rgbG( 7 downto 1 ) when videoMode = 1 else      	-- ( VGA 2, 7  --> SCART 11, 9 )
			"0" & vgaG( 7 downto 1 );
	VIDEO_B <= videoY when videoMode = 0 else 						-- ( S-Video 3, 1 --> SCART 20, 17 )
			"0" & rgbB( 7 downto 1 ) when videoMode = 1 else      	-- ( VGA 3, 8  --> SCART 7, 5 )
			"0" & vgaB( 7 downto 1 );
			
	VIDEO_HSYNC <= '1' when videoMode = 0 or videoMode = 1 else 	-- VGA 13 --> SCART 16
					vgaHSync;
	VIDEO_VSYNC <= palSync when videoMode = 0 or videoMode = 1 else -- VGA 14 --> SCART 20
					vgaVSync;

end rtl;
