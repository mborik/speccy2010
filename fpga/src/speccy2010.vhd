library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity speccy2010_top is
	port(
		CLK_20      : in std_logic;

		SOUND_LEFT  : out std_logic_vector(7 downto 0) := "10000000";
		SOUND_RIGHT : out std_logic_vector(7 downto 0) := "10000000";

		VIDEO_R     : out std_logic_vector(7 downto 0) := "01111111";
		VIDEO_G     : out std_logic_vector(7 downto 0) := "01111111";
		VIDEO_B     : out std_logic_vector(7 downto 0) := "01111111";

		VIDEO_HSYNC : out std_logic := '1';
		VIDEO_VSYNC : out std_logic := '1';

		ARM_AD      : inout std_logic_vector(15 downto 0) := (others => 'Z');
		ARM_A       : in std_logic_vector(23 downto 16);

		ARM_RD      : in std_logic;
		ARM_WR      : in std_logic;
		ARM_ALE     : in std_logic;
		ARM_WAIT    : out std_logic := '0';

		JOY0        : in std_logic_vector(5 downto 0);
		JOY1        : in std_logic_vector(5 downto 0);

		KEYS_CLK    : inout std_logic := 'Z';
		KEYS_DATA   : inout std_logic := 'Z';

		MOUSE_CLK   : inout std_logic := 'Z';
		MOUSE_DATA  : inout std_logic := 'Z';

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

		RESET_n     : in std_logic
	);

end speccy2010_top;

architecture rtl of speccy2010_top is

	constant memFreq : integer := 84;
	constant sysFreq : integer := 42;

	constant AY_MODE_NONE   : integer := 0;
	constant AY_MODE_ABC    : integer := 1;
	constant AY_MODE_ACB    : integer := 2;
	constant AY_MODE_MONO   : integer := 3;

	type  array_3x24   is array (0 to 2) of unsigned( 23 downto 0);
	type  weight_array is array (0 to 3, 0 to 1) of array_3x24;

	signal memclk : std_logic;
	signal sysclk : std_logic;

	signal reset  : std_logic;

	signal clk28m : std_logic;
	signal clk14m : std_logic;
	signal clk7m  : std_logic;
	signal clk35m : std_logic;

	signal counter20       : unsigned(31 downto 0) := x"00000000";
	signal counterMem      : unsigned(31 downto 0) := x"00000000";
	signal counter20_en    : std_logic := '0';
	signal counterMem_en   : std_logic := '0';

	signal cpuHaltReq          : std_logic := '0';
	signal cpuHaltAck          : std_logic := '0';
	signal cpuTraceReq         : std_logic := '0';
	signal cpuTraceAck         : std_logic := '0';
	signal cpuOneCycleWaitReq  : std_logic := '0';
	signal cpuOneCycleWaitAck  : std_logic := '0';

	constant cpuBPMax : integer := 3;
	type cpuBPArray is array ( integer range <> ) of unsigned(16 downto 0);
	signal cpuBP : cpuBPArray( 0 to cpuBPMax );

	signal activeBank   : unsigned( 1 downto 0 ) := "00";
	signal armVideoPage : std_logic_vector(15 downto 0) := x"0000";
	signal armBorder    : std_logic_vector(15 downto 0) := x"0000";

	signal keysDataOut  : std_logic_vector(7 downto 0);
	signal keysDataIn   : std_logic_vector(7 downto 0);

	signal keysRd       : std_logic := '0';
	signal keysWr       : std_logic := '0';

	signal keysReady    : std_logic;
	signal keysFull     : std_logic;

	signal mouseDataOut : std_logic_vector(7 downto 0);
	signal mouseDataIn  : std_logic_vector(7 downto 0);

	signal mouseRd      : std_logic := '0';
	signal mouseWr      : std_logic := '0';

	signal mouseReady   : std_logic;
	signal mouseFull    : std_logic;

	signal joyCode0     : std_logic_vector(15 downto 0) := x"0000";
	signal joyCode1     : std_logic_vector(15 downto 0) := x"0000";

--	-- SD-RAM control signals
	signal refresh	    : std_logic;
	signal memAddress   : std_logic_vector(23 downto 0);
	signal memDataIn    : std_logic_vector(15 downto 0);
	signal memDataOut   : std_logic_vector(15 downto 0);
	signal memDataMask  : std_logic_vector(1 downto 0);
	signal memWr        : std_logic := '0';
	signal memReq       : std_logic := '0';
	signal memAck       : std_logic := '0';

	signal memAddress2  : std_logic_vector(23 downto 0);
	signal memDataOut2  : std_logic_vector(15 downto 0);
	signal memWr2       : std_logic := '0';
	signal memReq2      : std_logic := '0';
	signal memAck2      : std_logic := '0';

	signal Invert       : unsigned(4 downto 0) := "00000";

	signal hCnt         : unsigned(15 downto 0) := x"0000";
	signal vCnt         : unsigned(15 downto 0) := x"0000";
	signal hSize        : integer;
	signal vSize        : integer;

	signal ChrC_Cnt     : unsigned(2 downto 0) := "000";    -- Character column counter
	signal ChrR_Cnt     : unsigned(2 downto 0) := "000";    -- Character row counter
	signal Hor_Cnt      : unsigned(5 downto 0) := "000000"; -- Horizontal counter
	signal Ver_Cnt      : unsigned(5 downto 0) := "000000"; -- Vertical counter

	signal specIntCounter   : unsigned(31 downto 0) := x"00000000";
	signal cpuMemoryWait    : std_logic := '0';

	signal ulaContendentMem : std_logic := '0';
	signal ulaWaitMem       : std_logic := '0';
	signal ulaWaitIo        : std_logic := '0';
	signal ulaWaitCancel    : std_logic := '0';
	signal ulaWait          : std_logic := '0';

	signal specPortFE   : std_logic_vector(7 downto 0);
	signal specPortFF   : std_logic_vector(7 downto 0);
	signal specPort7ffd : std_logic_vector(7 downto 0);
	signal specPort1ffd : std_logic_vector(7 downto 0); -- Scorpion

	signal specPortEff7 : std_logic_vector(7 downto 0);
--	signal specPortDff7 : std_logic_vector(7 downto 0);

	signal specDiskIfWait   : std_logic := '0';
	signal specDiskIfWr     : std_logic := '0';

	signal specTrdosEnabled : std_logic := '0';
	signal specTrdosTglFlag : std_logic := '0';
	signal specTrdosFlag    : std_logic := '0';
	signal specTrdosPortFF  : std_logic_vector(7 downto 0) := x"00";

	signal divmmcEnabled    : std_logic := '0';
	signal divmmcSramPage   : std_logic_vector(5 downto 0) := "000000";
	signal divmmcMapram     : std_logic := '0';
	signal divmmcConmem     : std_logic := '0';
	signal divmmcCardSelect : std_logic := '0';
	signal divmmcAmapRq     : std_logic := '0';
	signal divmmcAmap       : std_logic := '0';
	signal divmmcMemAddr    : std_logic_vector(23 downto 0) := x"000000";

	signal mb02Enabled      : std_logic := '0';
	signal mb02SramPage     : std_logic_vector(4 downto 0) := "00000";
	signal mb02MemSram      : std_logic := '0';
	signal mb02MemEprom     : std_logic := '0';
	signal mb02MemWriteRom  : std_logic := '0';
	signal mb02MemAddr      : std_logic_vector(23 downto 0) := x"000000";

	signal specMode         : unsigned(2 downto 0) := "000";  -- 0: ZX 48 | 1: ZX 128 | 2: Pentagon 1024 | 3: Scorpion
	signal syncMode         : unsigned(2 downto 0) := "000";  -- 0: ZX 48 | 1: ZX 128 | 2: Pentagon | 3: Scorpion
	signal cpuTurbo         : unsigned(1 downto 0) := "00";   -- 0: 3.5MHz | 1: 7MHz | 2: 14MHz | 3: 28MHz

	signal videoMode        : unsigned(3 downto 0) := "0000"; -- 0: Comp/SVideo | 1: RGB | 2: VGA 50Hz | 3: 60Hz | 4: 75Hz
	signal videoAspectRatio : unsigned(2 downto 0) := "000";  -- 0: 4/3 | 1: 5/4 | 2: 16/9
	signal videoInterlace   : std_logic := '0';

	signal covoxMode    : unsigned(2 downto 0) := "000"; -- 1: SD1 | 2: SD2 | 3: GS#B3 | 4: Pent#FB| 5: Scorp#DD | 6: Profi/BB55
	signal dacMode      : unsigned(1 downto 0) := "00";  -- 0: R-2R | 1: TDA1543 | 2: TDA1543A

	signal turboSound   : std_logic := '0';
	signal ayymMode     : std_logic := '0';              -- 0: YM  | 1: AY
	signal ayMode       : unsigned(2 downto 0) := "000"; -- 1: ABC | 2: ACB | 3: Mono

	signal borderAttr   : std_logic_vector(2 downto 0);
	signal speaker      : std_logic;

	signal tapeIn       : std_logic := '0';
	signal keyboard     : std_logic_vector(39 downto 0) := (others => '1');
	signal joystick     : std_logic_vector(7 downto 0) := x"00";
	signal mouseX       : std_logic_vector(7 downto 0) := x"00";
	signal mouseY       : std_logic_vector(7 downto 0) := x"00";
	signal mouseButtons : std_logic_vector(7 downto 0) := x"00";
	signal sound        : std_logic_vector(15 downto 0) := x"0000";

	signal soundLeft    : std_logic_vector(15 downto 0) := x"0000";
	signal soundRight   : std_logic_vector(15 downto 0) := x"0000";

	signal ayLeft       : std_logic_vector(15 downto 0) := x"0000";
	signal ayRight      : std_logic_vector(15 downto 0) := x"0000";

	signal romPage      : std_logic_vector(2 downto 0);
	signal ramPage      : std_logic_vector(7 downto 0);
	signal vramPage     : std_logic_vector(7 downto 0);

	signal attr         : std_logic_vector(15 downto 0);
	signal shift        : std_logic_vector(15 downto 0);

	signal Paper_r      : std_logic;
	signal Blank_r      : std_logic;
	signal Attr_r       : std_logic_vector(7 downto 0);
	signal Shift_r      : std_logic_vector(7 downto 0);

	signal paper        : std_logic;
	signal hsync        : std_logic;
	signal vsync1       : std_logic;
	signal vsync2       : std_logic;

	signal palSync      : std_logic;

	signal specR        : std_logic_vector(1 downto 0);
	signal specG        : std_logic_vector(1 downto 0);
	signal specB        : std_logic_vector(1 downto 0);
	signal specY        : std_logic;

	signal rgbR         : std_logic_vector(7 downto 0);
	signal rgbG         : std_logic_vector(7 downto 0);
	signal rgbB         : std_logic_vector(7 downto 0);
	signal rgbSum       : unsigned(9 downto 0);

	signal VideoHS_n    : std_logic;                       -- Holizontal Sync
	signal VideoVS_n    : std_logic;                       -- Vertical Sync
--	signal VideoCS_n    : std_logic;                       -- Composite Sync
	signal videoY       : std_logic_vector(7 downto 0);    -- Svideo_Y
	signal videoC       : std_logic_vector(7 downto 0);    -- Svideo_C
	signal videoV       : std_logic_vector(7 downto 0);    -- CompositeVideo

	signal ayCLK        : std_logic;
	signal ayADDR       : std_logic;
	signal ayWR         : std_logic;
	signal ayRD         : std_logic;

	signal ayDin        : std_logic_vector(7 downto 0);
	signal ayDout       : std_logic_vector(7 downto 0);

	signal ayOUT_A      : std_logic_vector(7 downto 0);
	signal ayOUT_B      : std_logic_vector(7 downto 0);
	signal ayOUT_C      : std_logic_vector(7 downto 0);

	signal ay2WR        : std_logic;
	signal ay2RD        : std_logic;
	signal ay2Dout      : std_logic_vector(7 downto 0);

	signal ay2OUT_A     : std_logic_vector(7 downto 0);
	signal ay2OUT_B     : std_logic_vector(7 downto 0);
	signal ay2OUT_C     : std_logic_vector(7 downto 0);

	signal selectedAY2  : std_logic := '0';

	signal covoxOUT_L1  : std_logic_vector(7 downto 0);
	signal covoxOUT_L2  : std_logic_vector(7 downto 0);
	signal covoxOUT_R1  : std_logic_vector(7 downto 0);
	signal covoxOUT_R2  : std_logic_vector(7 downto 0);
	signal covoxDin     : std_logic_vector(7 downto 0);
	signal covoxAddr    : std_logic_vector(15 downto 0);
	signal covoxWR      : std_logic;

	signal weightAY_L   : array_3x24;
	signal weightAY_R   : array_3x24;

	signal covoxEnabled : std_logic := '0';
	signal ay1Enabled   : std_logic := '0';
	signal ay2Enabled   : std_logic := '0';


	signal mixedOutputL : std_logic_vector(15 downto 0);
	signal mixedOutputR : std_logic_vector(15 downto 0);

	signal tdaData      : std_logic := '1';
	signal tdaWs        : std_logic := '1';
	signal tdaBck       : std_logic := '1';

	signal cpuCLK_nowait        : std_logic;
	signal cpuCLK_nowait_prev   : std_logic;
	signal cpuCLK               : std_logic;

	signal cpuWait      : std_logic;
	signal cpuReset     : std_logic := '0';

	signal cpuInvokeNMI : std_logic := '0';
	signal accessRom     : std_logic := '0';

	signal cpuINT       : std_logic;
	signal cpuNMI       : std_logic;
	signal cpuM1        : std_logic;
	signal cpuRFSH      : std_logic;
	signal cpuMREQ      : std_logic;
	signal cpuIORQ      : std_logic;
	signal cpuRD        : std_logic;
	signal cpuWR        : std_logic;
	signal cpuA         : std_logic_vector(15 downto 0);

	signal cpuDout      : std_logic_vector(7 downto 0);
	signal cpuDin       : std_logic_vector(7 downto 0);

	signal cpuRegisters     : std_logic_vector(212 downto 0) := (others => '0');
	signal cpuRegNumber     : std_logic_vector(3 downto 0);
	signal cpuRegValue      : std_logic_vector(15 downto 0);
	signal cpuRegStore_n    : std_logic := '1';

	signal vgaHSync         : std_logic;
	signal vgaVSync         : std_logic;
	signal vgaR             : std_logic_vector(7 downto 0);
	signal vgaG             : std_logic_vector(7 downto 0);
	signal vgaB             : std_logic_vector(7 downto 0);

	signal tapeFifoWrTmp    : std_logic_vector( 15 downto 0 );
	signal tapeFifoWr       : std_logic;
	signal tapeFifoRdTmp    : std_logic_vector( 15 downto 0 );
	signal tapeFifoRd       : std_logic;

	signal tapeFifoReady    : std_logic;
	signal tapeFifoFull     : std_logic;

	-- A B C (L)  A B C (R)
	constant weigthYmTable : weight_array := (
		((x"000000",x"000000",x"000000"),(x"000000",x"000000",x"000000")),
		((x"000092",x"000007",x"000066"),(x"000007",x"000092",x"000066")),
		((x"000092",x"000066",x"000007"),(x"000007",x"000066",x"000092")),
		((x"000055",x"000055",x"000055"),(x"000055",x"000055",x"000055"))
	);

	constant weigthAyTable : weight_array := (
		((x"000000",x"000000",x"000000"),(x"000000",x"000000",x"000000")),
		((x"00007e",x"000029",x"000058"),(x"000029",x"00007e",x"000058")),
		((x"00007e",x"000058",x"000029"),(x"000029",x"000058",x"00007e")),
		((x"000055",x"000055",x"000055"),(x"000055",x"000055",x"000055"))
	);

	alias vid_mode_arm : std_logic is armVideoPage(15);
begin

	U00 : entity work.pll
		port map(
			inclk0 => CLK_20,
			c0     => memclk,
			c1     => sysclk
		);

	-- Z80
	U01 : entity work.t80se
		port map(
			RESET_n => not ( cpuReset or reset ),
			CLK_n   => sysclk,
			CLKEN   => cpuCLK,
			WAIT_n  => '1',
			INT_n   => cpuINT,
			NMI_n   => cpuNMI,
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
			DO      => cpuDout,
			DI      => cpuDin,

			REG => cpuRegisters,
			DIR => cpuRegValue,
			DIRNumber => cpuRegNumber,
			DIRSet_n => cpuRegStore_n
		);

	-- PSG chip
	U02 : entity work.YM2149
		port map(
			RESET_L => not ( cpuReset or reset ),
			CLK     => sysclk,
			ENA     => ayCLK and not cpuHaltAck,
			I_SEL_L => '1',

			I_DA    => ayDin,
			O_DA    => ayDout,

			busctrl_addr => ayADDR,
			busctrl_we   => ayWR,
			busctrl_re   => ayRD,
			ctrl_aymode  => ayymMode,

			O_AUDIO_A    => ayOUT_A,
			O_AUDIO_B    => ayOUT_B,
			O_AUDIO_C    => ayOUT_C
		);

	U02B : entity work.YM2149
		port map(
			RESET_L => not ( cpuReset or reset ),
			CLK     => sysclk,
			ENA     => ayCLK and not cpuHaltAck,
			I_SEL_L => '1',

			I_DA    => ayDin,
			O_DA    => ay2Dout,

			busctrl_addr => ayADDR,
			busctrl_we   => ay2WR,
			busctrl_re   => ay2RD,
			ctrl_aymode  => ayymMode,

			O_AUDIO_A    => ay2OUT_A,
			O_AUDIO_B    => ay2OUT_B,
			O_AUDIO_C    => ay2OUT_C
		);

	-- video encoder
	U03 : entity work.vencode
		generic map( freq => sysFreq )

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

	-- memory
	U04 : entity work.sdram
		generic map( freq => memFreq )

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
			memDataIn2 => x"0000",
			memDataOut2 => memDataOut2,
			memDataMask2 => "00",
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

	-- keyboard
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

	-- mouse
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

	-- tape
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

	-- Covox
	U08 : entity work.SounDrive
		port map(
			CLK         => sysclk,

			I_DA        => covoxDin,

			bus_addr    => covoxAddr,
			busctrl_we  => covoxWR,
			covox_mode  => covoxMode,

			O_AUDIO_L1  => covoxOUT_L1,
			O_AUDIO_L2  => covoxOUT_L2,
			O_AUDIO_R1  => covoxOUT_R1,
			O_AUDIO_R2  => covoxOUT_R2
		);

	-- SoundMixer L/R
	U09_L : entity work.SoundMixer
		port map (
			CLK     => sysclk,
			I_AUDIO1_AY_A => ayOUT_A,
			I_AUDIO1_AY_B => ayOUT_B,
			I_AUDIO1_AY_C => ayOUT_C,

			I_AUDIO2_AY_A => ay2OUT_A,	--AY2
			I_AUDIO2_AY_B => ay2OUT_B,
			I_AUDIO2_AY_C => ay2OUT_C,

			WEIGHT_AUDIO1_AY_A => weightAY_L(0),	-- AY1 - WEIGHT
			WEIGHT_AUDIO1_AY_B => weightAY_L(1),	-- sum should be <=1
			WEIGHT_AUDIO1_AY_C => weightAY_L(2),

			WEIGHT_AUDIO2_AY_A => weightAY_L(0),	-- AY2 - WEIGHT
			WEIGHT_AUDIO2_AY_B => weightAY_L(1),	-- sum should be <=1
			WEIGHT_AUDIO2_AY_C => weightAY_L(2),

			I_AUDIO_AUX_1 => covoxOUT_L1,			-- COVOX C1
			I_AUDIO_AUX_2 => covoxOUT_L2,			-- COVOX C2

			WEIGHT_AUDIO_AUX_1 => x"000100",		-- COVOX C1
			WEIGHT_AUDIO_AUX_2 => x"000100",		-- COVOX C2

			I_AUDIO_BEEPER => speaker,
			I_AUDIO_TAPE   => tapeIn,

			WEIGHT_AUDIO_BEEPER => x"000055",
			WEIGHT_AUDIO_TAPE   => x"000020",

			I_AY1_ENABLED  => ay1Enabled,
			I_AY2_ENABLED  => ay2Enabled,
			I_AUX1_ENABLED => covoxEnabled,
			I_AUX2_ENABLED => covoxEnabled,
			O_AUDIO        => mixedOutputL
		);

	U09_R : entity work.SoundMixer
		port map (
			CLK     => sysclk,
			I_AUDIO1_AY_A => ayOUT_A,
			I_AUDIO1_AY_B => ayOUT_B,
			I_AUDIO1_AY_C => ayOUT_C,

			I_AUDIO2_AY_A => ay2OUT_A,	--AY2
			I_AUDIO2_AY_B => ay2OUT_B,
			I_AUDIO2_AY_C => ay2OUT_C,

			WEIGHT_AUDIO1_AY_A => weightAY_R(0),	-- AY1 - WEIGHT
			WEIGHT_AUDIO1_AY_B => weightAY_R(1),	-- sum should be <=1
			WEIGHT_AUDIO1_AY_C => weightAY_R(2),

			WEIGHT_AUDIO2_AY_A => weightAY_R(0),	-- AY2 - WEIGHT
			WEIGHT_AUDIO2_AY_B => weightAY_R(1),	-- sum should be <=1
			WEIGHT_AUDIO2_AY_C => weightAY_R(2),

			I_AUDIO_AUX_1 => covoxOUT_R1,			-- COVOX C1
			I_AUDIO_AUX_2 => covoxOUT_R2,			-- COVOX C2

			WEIGHT_AUDIO_AUX_1 => x"000100",		-- COVOX C1
			WEIGHT_AUDIO_AUX_2 => x"000100",		-- COVOX C2

			I_AUDIO_BEEPER => speaker,
			I_AUDIO_TAPE   => tapeIn,

			WEIGHT_AUDIO_BEEPER => x"000055",
			WEIGHT_AUDIO_TAPE   => x"000020",

			I_AY1_ENABLED  => ay1Enabled,
			I_AY2_ENABLED  => ay2Enabled,
			I_AUX1_ENABLED => covoxEnabled,
			I_AUX2_ENABLED => covoxEnabled,
			O_AUDIO        => mixedOutputR
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


--	paging ------------------------------------------------------------------------------

	reset <= not RESET_n;

	specTrdosTglFlag <=	'1' when specTrdosFlag = '0' and cpuM1 = '0' and cpuMREQ = '0' and specPort7ffd(4) = '1' and cpuA( 15 downto 8 ) = x"3D" else
						'1' when specTrdosFlag = '0' and cpuM1 = '0' and cpuMREQ = '0' and cpuNMI = '0' and accessRom = '0' else
						'1' when specTrdosFlag = '1' and cpuM1 = '0' and cpuMREQ = '0' and cpuNMI = '1' and accessRom = '0' else
						'0';

	-- 000 > ROM 0 (128k BASIC and menu)
	-- 001 > ROM 1 (classic 48k BASIC)
	-- 010 > Service ROM
	-- 011 > TR-DOS ROM or DivMMC firmware
	romPage <=	"00" & specPort7ffd(4) when specTrdosEnabled = '0' else
				"0" & ( specTrdosFlag xor specTrdosTglFlag ) & specPort7ffd(4) when specMode /= 3 else
				"010" when specPort1ffd(1) = '1' else
				"011" when ( specTrdosFlag xor specTrdosTglFlag ) = '1' else
				"00" & specPort7ffd(4);

	-- 0000-3FFF > bank #00
	-- 4000-7FFF > bank #05
	-- 8000-BFFF > bank #02
	ramPage <=	"00000000" when cpuA( 15 downto 14 ) = "00" else
				"00000101" when cpuA( 15 downto 14 ) = "01" else
				"00000010" when cpuA( 15 downto 14 ) = "10" else
	-- C000-FFFF > bank  #00-3F (Pentagon 128/1024)
				"00" & specPort7ffd(5) & specPort7ffd( 7 downto 6 ) & specPort7ffd( 2 downto 0 ) when specMode = 2 else
	-- C000-FFFF > bank  #00-0F (Scorpion)
				"0000"  & specPort1ffd(4) & specPort7ffd( 2 downto 0 ) when specMode = 3 else
	-- C000-FFFF > bank #00-#07 (ZX Spectrum 128)
				"00000" & specPort7ffd( 2 downto 0 );

	divmmcMemAddr <=
	-- 0000-1FFF > DivMMC bank #03
		"000001000011" & cpuA( 12 downto 1 ) when cpuA(13) = '0' and divmmcMapram = '1' and divmmcAmap = '1' and divmmcConmem = '0' else
	-- 0000-1FFF > DivMMC (E)EPROM (firmware in ROM3)
		"001000000110" & cpuA( 12 downto 1 ) when cpuA(13) = '0' and (divmmcConmem = '1' or divmmcAmap = '1') else
	-- 2000-3FFF > DivMMC bank #00-#3F (controlled by port #E3)
		"000001" & divmmcSramPage & cpuA( 12 downto 1 ) when cpuA(13) = '1' and (divmmcConmem = '1' or divmmcAmap = '1') else
	-- 0000-3FFF > ZX ROM otherwise
		"0010000000" & specPort7ffd(4) & cpuA( 13 downto 1 );

	mb02MemAddr <=
		-- 0000-3FFF > MB-02 SRAM bank #00-#1F (controlled by port #17)
		"000001" & mb02SramPage & cpuA( 13 downto 1 ) when mb02MemSram = '1' else
		-- 0000-3FFF > MB-02 dummy 2kB EPROM (in ROM3 position)
		"00100000011000" & cpuA( 10 downto 1 ) when mb02MemEprom = '1' else
		-- 0000-3FFF > ZX ROM otherwise
		"0010000000" & specPort7ffd(4) & cpuA( 13 downto 1 );

	accessRom <= '1' when cpuA( 15 downto 14 ) = "00" and ( specMode /= 3 or specPort1ffd(0) = '0' ) else '0';

	ayDin <= cpuDout;
	covoxDin  <= cpuDout;


	process( memclk )

		variable addressReg : unsigned(23 downto 0) := x"000000";

		variable ArmWr	: std_logic_vector(1 downto 0);
		variable ArmRd	: std_logic_vector(1 downto 0);
		variable ArmAle	: std_logic_vector(1 downto 0);

		variable armReq : std_logic := '0';
		variable portFfReq : std_logic := '0';

		variable kbdTmp : std_logic_vector(4 downto 0);
		variable cpuReq : std_logic;

		type rtcRamType is array ( integer range <> ) of std_logic_vector( 7 downto 0 );
		variable rtcRam : rtcRamType( 0 to 27 );

		variable rtcRamAddressRd : unsigned(7 downto 0) := x"00";
		variable rtcRamDataRd : std_logic_vector(7 downto 0) := x"00";
		variable rtcRamDataRdPrev : std_logic_vector(7 downto 0) := x"00";

		variable iCpuWr : std_logic_vector(1 downto 0);
		variable iCpuRd : std_logic_vector(1 downto 0);

		variable breakpointReq: integer range 0 to cpuBPMax;

	begin

		if memclk'event and memclk = '1' then

			if reset = '1' then
				addressReg := x"c000f0";
				cpuReset <= '0';
				cpuHaltReq <= '0';
				cpuInvokeNMI <= '0';
				specPort1ffd <= x"00";
				specPort7ffd <= x"00";
				selectedAY2 <= '0';
			end if;

			rtcRamDataRd := rtcRamDataRdPrev;
			if rtcRamAddressRd < 12 then
				rtcRamDataRdPrev := rtcRam( to_integer( rtcRamAddressRd ) );
			elsif rtcRamAddressRd = 16 then
				rtcRamDataRdPrev := x"80";
			elsif rtcRamAddressRd = 17 then
				rtcRamDataRdPrev := x"aa";
			else
				rtcRamDataRdPrev := x"00";
			end if;

			if cpuReset = '1' then
				specDiskIfWait <= '0';
				specDiskIfWr <= '0';

				if divmmcEnabled = '1' then
					divmmcSramPage <= (others => '0');
					divmmcMapram <= divmmcMapram or '0';
					divmmcConmem <= '0';
					divmmcCardSelect <= '0';
					divmmcAmapRq <= '0';
					divmmcAmap <= '0';
				elsif mb02Enabled = '1' then
					mb02SramPage <= (others => '0');
					mb02MemSram <= '0';
					mb02MemEprom <= '1'; -- set after reset
					mb02MemWriteRom <= '0';
				end if;
			end if;

			if specTrdosTglFlag = '1' then
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

----------------------------------------------------------------------------------

			if ulaWait /= '1' and cpuTurbo = 3 then
				iCpuWr := iCpuWr(0) & cpuWR;
				iCpuRd := iCpuRd(0) & cpuRD;
			end if;

----------------------------------------------------------------------------------

			-- DivMMC automapper
			if divmmcEnabled = '1' and reset = '0' then
				if cpuMREQ = '0' and cpuRD = '0' and cpuM1 = '0' then
					if (cpuA = x"0000" or cpuA = x"0008" or cpuA = x"0038" or
						cpuA = x"0066" or cpuA = x"04C6" or cpuA = x"0562") then
						-- activate automapper after this cycle
						divmmcAmapRq <= '1';
					elsif cpuA(15 downto 8) = x"3D" then
						-- activate automapper immediately
						divmmcAmap <= '1';
						divmmcAmapRq <= '1';
					elsif cpuA(15 downto 3) & "000" = x"1FF8" then
						-- deactivate automapper after this cycle on 0x1FF8-0x1FFF
						divmmcAmapRq <= '0';
					end if;
				end if;

				if cpuM1 = '1' then
					divmmcAmap <= divmmcAmapRq;
				end if;
			end if;

			-- NMI signal in MB-02 switching the 0th SRAM bank (BS-ROM)
			if mb02Enabled = '1' and cpuM1 = '0' and cpuMREQ = '0' and cpuNMI = '0' then
				mb02SramPage <= (others => '0');
				mb02MemSram <= '1'; -- set after NMI
				mb02MemEprom <= '0';
				mb02MemWriteRom <= '0';
			end if;

----------------------------------------------------------------------------------

			if cpuRD /= '0' then
				cpuDin <= x"ff";
				portFfReq := '0';
				ayRD <= '0';
				ay2RD <= '0';
			elsif portFfReq = '1' then
				cpuDin <= specPortFF;
			elsif ayRD = '1' then
				cpuDin <= ayDout;
			elsif ay2RD = '1' then
				cpuDin <= ay2Dout;
			end if;

			if cpuWR /= '0' then
				ayADDR <= '0';
				ayWR <= '0';
				ay2WR     <= '0';
				covoxWR   <= '0';
				covoxAddr <= x"0000";
			end if;

--			if cpuCLK_prev = '1' and cpuWR = '0' then
			if iCpuWr = "10" then

				if cpuMREQ = '0' then
					-- memory write into RAM
					if accessRom = '0' then
						memAddress <= "000" & ramPage & cpuA( 13 downto 1 );
						memDataIn <= cpuDout & cpuDout;
						memDataMask(0) <= not cpuA(0);
						memDataMask(1) <= cpuA(0);
						memWr <= '1';
						cpuReq := '1';
						memReq <= '1';
						cpuMemoryWait <= '1';

					-- memory write into #2000-3FFF segment of DivMMC SRAMx
					elsif divmmcEnabled = '1' and cpuA( 15 downto 13 ) = "001"
						and (divmmcConmem = '1' or divmmcAmap = '1')
						and not (divmmcMapram = '1' and divmmcSramPage = "000011") then

						memAddress <= divmmcMemAddr;
						memDataIn <= cpuDout & cpuDout;
						memDataMask(0) <= not cpuA(0);
						memDataMask(1) <= cpuA(0);
						memWr <= '1';
						cpuReq := '1';
						memReq <= '1';
						cpuMemoryWait <= '1';

					-- memory write into ROM segment when MB-02 SRAM paged
					elsif mb02Enabled = '1' and cpuA( 15 downto 14 ) = "00"
						and mb02MemWriteRom = '1' and mb02MemSram = '1' then

						memAddress <= mb02MemAddr;
						memDataIn <= cpuDout & cpuDout;
						memDataMask(0) <= not cpuA(0);
						memDataMask(1) <= cpuA(0);
						memWr <= '1';
						cpuReq := '1';
						memReq <= '1';
						cpuMemoryWait <= '1';
					end if;
				elsif cpuIORQ = '0' and cpuM1 = '1' then

					-- Betadisk control or data-transfer ports
					if specTrdosEnabled = '1' and ( specTrdosFlag = '1' or specMode = 3 ) and cpuA( 4 downto 0 ) = "11111" then
						specDiskIfWait <= '1';
						specDiskIfWr <= '1';

					-- DivMMC or MB-02 real time clock at #0n03 ports
					elsif ( divmmcEnabled = '1' or mb02Enabled = '1' ) and cpuA( 15 downto 12 ) & cpuA( 7 downto 0 ) = x"003" then
						rtcRam( to_integer(unsigned(cpuA( 11 downto 8 ))) + 12 ) := "0000" & cpuDout( 3 downto 0 );

					-- DivMMC control or data-transfer ports
					elsif divmmcEnabled = '1' and cpuA( 7 downto 4 ) & cpuA( 1 downto 0 ) = "111011" then
						if cpuA ( 3 downto 2 ) = "00" then		-- #E3
							divmmcSramPage <= cpuDout( 5 downto 0 );
							divmmcMapram <= divmmcMapram or cpuDout(6);
							divmmcConmem <= cpuDout(7);
						elsif cpuA ( 3 downto 2 ) = "01" then	-- #E7
							divmmcCardSelect <= cpuDout(0);
						elsif cpuA ( 3 downto 2 ) = "10" then	-- #EB
							specDiskIfWait <= '1';
							specDiskIfWr <= '1';
						end if;

					-- MB-02 control or data-transfer ports
					elsif mb02Enabled = '1' and ( cpuA(7) & cpuA( 5 downto 3 ) & cpuA( 1 downto 0 ) ) = "001011" then
						if cpuA ( 7 downto 0 ) = x"17" then		-- #17
							-- SRAM+EPROM bits connected to reset signal
							if cpuDout( 7 downto 6 ) = "11" then
								cpuReset <= '1';
							else
								mb02SramPage <= cpuDout( 4 downto 0 );
								mb02MemWriteRom <= cpuDout(5);
								mb02MemSram <= cpuDout(6);
								mb02MemEprom <= cpuDout(7);
							end if;

						elsif cpuA ( 7 downto 0 ) = x"53" then	-- #53
							specDiskIfWait <= '1';
							specDiskIfWr <= '1';
						end if;

					else
						if cpuA( 7 downto 0 ) = x"FE" then
							specPortFE <= cpuDout;
						elsif cpuA = x"eff7" then
							specPortEff7 <= cpuDout;
						elsif cpuA = x"dff7" and specPortEff7(7) = '1' then
							rtcRamAddressRd := unsigned(cpuDout);
						elsif ayMode /= AY_MODE_NONE and cpuA( 15 downto 14 ) = "11" and cpuA(1 downto 0) = "01" then
							ayADDR <= '1';

							if cpuDout = x"FE" then
								selectedAY2 <= '1';
							elsif cpuDout = x"FF" then
								selectedAY2 <= '0';
							end if;
						elsif ayMode /= AY_MODE_NONE and cpuA( 15 downto 14 ) = "10" and cpuA(1 downto 0) = "01" then
							if turboSound = '1' and selectedAY2 = '1' then
								ay2WR <= '1';
							else
								ayWR <= '1';
							end if;

						-- Scorpion
						elsif specMode = 3 then
							if cpuA( 15 downto 12 ) = "0001" and cpuA(1) = '0' then
							--if cpuA( 15 downto 0 ) = x"1ffd" then
								specPort1ffd <= cpuDout;
							elsif cpuA( 15 downto 14 ) = "01" and cpuA(5) = '1' and cpuA( 1 downto 0 ) = "01" and specPort7ffd(5) = '0' then
								specPort7ffd <= cpuDout;
							end if;
						elsif cpuA( 15 ) = '0' and cpuA( 7 downto 0 ) = x"FD" and ( specMode = 2 or specPort7ffd(5) = '0' ) then
							specPort7ffd <= cpuDout;
						end if;

						if ( cpuA(15) = '1' or cpuA(14) = '1' ) then
							covoxWR   <= '1';
							covoxAddr <= cpuA;
						end if;

					end if;

				end if;

--			elsif cpuCLK_prev = '1' and cpuRD = '0' then
			elsif iCpuRd = "10" then

				cpuDin <= x"FF";

				if cpuMREQ = '0' then

					if accessRom = '1' then
						if divmmcEnabled = '1' then
							memAddress <= divmmcMemAddr;
						elsif mb02Enabled = '1' then
							memAddress <= mb02MemAddr;
						else
							memAddress <= "00100000" & romPage & cpuA( 13 downto 1 );
						end if;
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
					elsif specTrdosEnabled = '1' and cpuA = x"bff7" and specPortEff7(7) = '1' then
						cpuDin <= rtcRamDataRd;

					elsif specTrdosEnabled = '1' and ( specTrdosFlag = '1' or specMode = 3 ) and cpuA( 4 downto 0 ) = "11111" then
						if cpuA( 7 downto 0 ) = x"FF" then
							cpuDin <= specTrdosPortFF;
						else
							specDiskIfWait <= '1';
							specDiskIfWr <= '0';
						end if;

					-- DivMMC or MB-02 real time clock at #0n03 ports
					elsif ( divmmcEnabled = '1' or mb02Enabled = '1' ) and cpuA( 15 downto 12 ) & cpuA( 7 downto 0 ) = x"003" then
						cpuDin <= rtcRam( to_integer(unsigned(cpuA( 11 downto 8 ))) + 12 );

					-- DivMMC or MB-02 control and data-transfer ports
					elsif ( divmmcEnabled = '1' and cpuA( 7 downto 0 ) = x"EB" ) or
							( mb02Enabled = '1' and cpuA( 7 downto 0 ) = x"53" ) then
						specDiskIfWait <= '1';
						specDiskIfWr <= '0';

					-- MB-02 paging port
					elsif ( mb02Enabled = '1' and cpuA( 7 downto 0 ) = x"17" ) then
						cpuDin <= mb02MemEprom & mb02MemSram & mb02MemWriteRom & mb02SramPage;

					else

						if cpuA( 7 downto 0 ) = x"1F" then
							cpuDin <= joystick;
						elsif ayMode /= AY_MODE_NONE and cpuA( 15 downto 14 ) = "11" and cpuA(1 downto 0) = "01" then
							if turboSound = '1' and selectedAY2 = '1' then
								ay2RD <= '1';
							else
								ayRD <= '1';
							end if;
						elsif cpuA( 7 downto 0 ) = x"FF" then
							portFfReq := '1';
						end if;

					end if;

				end if;

			elsif armReq = '1' and ARM_WR = '0' then

				armReq := '0';

				if addressReg( 23 ) = '0' and ( cpuHaltAck = '1' or specDiskIfWait = '1' ) then
					memAddress <= std_logic_vector( activeBank ) & std_logic_vector(addressReg( 22 downto 1 ) );
					memDataIn <= ARM_AD( 7 downto 0 ) & ARM_AD( 7 downto 0 );
					memDataMask( 0 ) <= not addressReg( 0 );
					memDataMask( 1 ) <= addressReg( 0 );
					memWr <= '1';

					cpuReq := '0';
					memReq <= '1';

				elsif addressReg( 23 downto 22 ) = "10" and ( cpuHaltAck = '1' or specDiskIfWait = '1' ) then
					memAddress <= std_logic_vector( activeBank ) & std_logic_vector(addressReg( 21 downto 0 ) );
					memDataIn <= ARM_AD;
					memDataMask <= "11";
					memWr <= '1';

					cpuReq := '0';
					memReq <= '1';

				else
					if addressReg( 23 downto 8 ) = x"c000" then
						if addressReg( 7 downto 0 ) = x"00" then
							cpuHaltReq <= ARM_AD( 0 );
							cpuOneCycleWaitReq <= ARM_AD( 2 );
							cpuReset <= ARM_AD( 3 );
							cpuInvokeNMI <= ARM_AD( 4 );

						elsif addressReg( 7 downto 0 ) = x"08" then
							cpuTraceReq <= ARM_AD(0);
						elsif addressReg( 7 downto 0 ) = x"09" then
							breakpointReq := to_integer( unsigned( ARM_AD( 1 downto 0 ) ) );

							if ( ARM_AD(15) = '0' ) then
								cpuBP( breakpointReq ) <= ( '0' & x"0000" );
							end if;
						elsif addressReg( 7 downto 0 ) = x"0A" then
							cpuBP( breakpointReq ) <= unsigned( '1' & ARM_AD );

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
							specPortFE <= ARM_AD(7 downto 0);
						elsif addressReg( 7 downto 0 ) = x"17" then
							specPort7ffd <= ARM_AD(7 downto 0);
						elsif addressReg( 7 downto 0 ) = x"23" then
							specPort1ffd <= ARM_AD(7 downto 0);
						elsif addressReg( 7 downto 0 ) = x"18" then
							specTrdosFlag <= ARM_AD(0);
						elsif addressReg( 7 downto 0 ) = x"19" then
							specDiskIfWait <= '0';
						elsif addressReg( 7 downto 0 ) = x"1b" then
							cpuDin <= ARM_AD(7 downto 0);
						elsif addressReg( 7 downto 0 ) = x"1d" then
							specTrdosPortFF <= ARM_AD( 7 downto 0 );

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
							specMode <= unsigned( ARM_AD(2 downto 0) );
							syncMode <= unsigned( ARM_AD(5 downto 3) );
							cpuTurbo <= unsigned( ARM_AD(7 downto 6) );
						elsif addressReg( 7 downto 0 ) = x"41" then
							videoMode        <= unsigned( ARM_AD(3 downto 0) );
							videoAspectRatio <= unsigned( ARM_AD(6 downto 4) );
							videoInterlace   <= ARM_AD(7);
						elsif addressReg( 7 downto 0 ) = x"42" then
							specTrdosEnabled <= ARM_AD(0);
							divmmcEnabled    <= ARM_AD(1);
							mb02Enabled      <= ARM_AD(2);

							if ARM_AD(15) = '1' then
								if divmmcEnabled = '1' then
									-- hard reset of DivMMC
									divmmcSramPage <= (others => '0');
									divmmcMapram <= '0';
									divmmcConmem <= '0';
									divmmcCardSelect <= '0';
									divmmcAmapRq <= '0';
									divmmcAmap <= '0';
								end if;
							end if;
						elsif addressReg( 7 downto 0 ) = x"45" then
							turboSound <= ARM_AD(0);
							ayMode     <= unsigned( ARM_AD(3 downto 1) );
							ayymMode   <= ARM_AD(4);
						elsif addressReg( 7 downto 0 ) = x"46" then
							covoxMode  <= unsigned( ARM_AD(2 downto 0) );
							dacMode    <= unsigned( ARM_AD(4 downto 3) );

						elsif addressReg( 7 downto 0 ) = x"50" then
							counter20_en <= ARM_AD(0);
							counterMem_en <= ARM_AD(0);

						end if;

					elsif addressReg( 23 downto 4 ) = x"c001f" then
						if addressReg( 3 downto 0 ) = x"F" then
							if ARM_AD = x"0000" then
								cpuRegStore_n <= '1';
							else
								cpuRegStore_n <= '0';
							end if;
						else
							cpuRegNumber <= std_logic_vector(addressReg( 3 downto 0 ));
							cpuRegValue <= ARM_AD;
							cpuRegStore_n <= '0';
						end if;

					elsif addressReg( 23 downto 8 ) = x"c001" then
						rtcRam( to_integer( addressReg( 7 downto 0 ) ) ) := ARM_AD( 7 downto 0 );

					end if;

					ARM_WAIT <= '1';
				end if;

			elsif armReq = '1' and ARM_RD = '0' then

				armReq := '0';

				if addressReg( 23 ) = '0' and ( cpuHaltAck = '1' or specDiskIfWait = '1' ) then
					memAddress <= std_logic_vector( activeBank ) & std_logic_vector(addressReg( 22 downto 1 ) );
					memWr <= '0';

					cpuReq := '0';
					memReq <= '1';
				elsif addressReg( 23 downto 22 ) = "10" and ( cpuHaltAck = '1' or specDiskIfWait = '1' ) then
					memAddress <= std_logic_vector( activeBank ) & std_logic_vector (addressReg( 21 downto 0 ) );
					memWr <= '0';

					cpuReq := '0';
					memReq <= '1';
				else
					if addressReg( 23 downto 8 ) = x"c000" then

						if addressReg( 7 downto 0 ) = x"00" then
							ARM_AD <= x"00" & b"0000000" & cpuHaltAck;

						elsif addressReg( 7 downto 0 ) = x"15" then
							ARM_AD <= x"00" & "0000000" & tapeFifoFull;

						elsif addressReg( 7 downto 0 ) = x"16" then
							ARM_AD <= x"00" & specPortFE;
						elsif addressReg( 7 downto 0 ) = x"17" then
							ARM_AD <= x"00" & specPort7ffd;
						elsif addressReg( 7 downto 0 ) = x"23" then
							ARM_AD <= x"00" & specPort1ffd;
						elsif addressReg( 7 downto 0 ) = x"18" then
							ARM_AD <= x"00" & b"0000000" & specTrdosFlag;
						elsif addressReg( 7 downto 0 ) = x"19" then
							ARM_AD <= x"00" & b"000000" & specDiskIfWr & specDiskIfWait;
						elsif addressReg( 7 downto 0 ) = x"1a" then
							ARM_AD <= cpuA;
						elsif addressReg( 7 downto 0 ) = x"1b" then
							ARM_AD <= x"00" & cpuDout;

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

						elsif addressReg( 7 downto 0 ) = x"40" then
							ARM_AD <= x"00" & std_logic_vector(cpuTurbo) & std_logic_vector(syncMode) & std_logic_vector(specMode);

						elsif addressReg( 7 downto 0 ) = x"42" then
							if divmmcEnabled = '1' then
								ARM_AD <= "00000" & divmmcCardSelect & divmmcAmapRq & divmmcAmap & divmmcConmem & divmmcMapram & divmmcSramPage;
							elsif mb02Enabled = '1' then
								ARM_AD <= x"00" & mb02MemEprom & mb02MemSram & mb02MemWriteRom & mb02SramPage;
							end if;

						elsif addressReg( 7 downto 0 ) = x"50" then
							ARM_AD <= std_logic_vector( counter20( 15 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"51" then
							ARM_AD <= std_logic_vector( counter20( 31 downto 16 ) );
						elsif addressReg( 7 downto 0 ) = x"52" then
							ARM_AD <= std_logic_vector( counterMem( 15 downto 0 ) );
						elsif addressReg( 7 downto 0 ) = x"53" then
							ARM_AD <= std_logic_vector( counterMem( 31 downto 16 ) );

						elsif addressReg( 7 downto 0 ) = x"f0" then
							ARM_AD <= x"f126"; -- for firmware version >= 1.2.5

						else
							ARM_AD <= x"ffff";
						end if;

						ARM_WAIT <= '1';

					elsif addressReg( 23 downto 4 ) = x"c001f" then
						if addressReg( 3 downto 0 ) = x"F" or addressReg( 3 downto 0 ) = x"E" then
							ARM_AD <= x"0000";
						elsif addressReg( 3 downto 0 ) = x"D" then
							ARM_AD <= x"000" & cpuRegisters( 211 downto 208 ); -- IFF2, IFF1, IM
						else
							ARM_AD <= cpuRegisters((to_integer(addressReg( 3 downto 0 )) * 16) + 15 downto (to_integer(addressReg( 3 downto 0 )) * 16));
						end if;
						ARM_WAIT <= '1';

					elsif addressReg( 23 downto 8 ) = x"c001" then
						ARM_AD <= x"00" & rtcRam( to_integer( addressReg( 4 downto 0 ) ) );
						ARM_WAIT <= '1';

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

	------------------------------------------------------------------------------

	process(sysclk)

		variable cpuSaveInt7_prev : std_logic := '0';
		variable cpuBreakpointPC  : unsigned( 16 downto 0 ) := ( '0' & x"0000" );

	begin
		if sysclk'event and sysclk = '1' then

			if reset = '1' then
				cpuHaltAck <= '0';
			end if;

			if cpuTraceReq = '1' then

				cpuHaltAck <= '0';

			elsif cpuHaltAck = '0' then
				if cpuRegisters(212) = '0' and cpuSaveInt7_prev = '1' then

					if cpuHaltReq = '1' then
						cpuHaltAck <= '1';
					end if;

				elsif cpuRegisters(212) = '1' and cpuSaveInt7_prev = '0' then

					for i in 0 to cpuBPMax loop

						if ( cpuBP(i) = cpuBreakpointPC ) then
							cpuHaltAck <= '1';
						end if;

					end loop;

				end if;
			end if;

			cpuBreakpointPC := unsigned( '1' & cpuRegisters( 79 downto 64 ) );
			cpuSaveInt7_prev := cpuRegisters(212);

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

	paper <= '0' when Hor_Cnt(5) = '0' and Ver_Cnt(5) = '0' and ( Ver_Cnt(4) = '0' or Ver_Cnt(3) = '0' ) else '1';
	hsync <= '0' when Hor_Cnt(5 downto 2) = "1010" else '1';
	vsync1 <= '0' when Hor_Cnt(5 downto 1) = "00110" or Hor_Cnt(5 downto 1) = "10100" else '1';
	vsync2 <= '1' when Hor_Cnt(5 downto 2) = "0010" or Hor_Cnt(5 downto 2) = "1001" else '0';

	ulaWaitMem <= '0' when hCnt(3 downto 1) = "000" or hCnt(3 downto 1) = "111" else '1';
	ulaWaitIo <= '0' when hCnt(3 downto 2) = "11" else '1';

	--ulaWaitMem <= '0';
	--ulaWaitIo <= '0';

	ulaContendentMem <= '1' when cpuA( 15 downto 14 ) = "01" or ( cpuA( 15 downto 14 ) = "11" and specPort7ffd( 0 ) = '1' ) else '0';

	ulaWait <= '1' when ( cpuTurbo = 0 and ( syncMode = 0 or syncMode = 1 ) ) and
		paper = '0' and
		(
			( cpuMREQ = '0' and ulaContendentMem = '1' and ulaWaitMem = '1' and ulaWaitCancel = '0' )
			or
			( ( cpuIORQ = '0' and cpuA( 0 ) = '0' ) and ulaWaitIo = '1' and ulaWaitCancel ='0' )
			or
			--( cpuIORQ = '0' and ( cpuA( 15 downto 14 ) = "01" ) and ulaWaitIo = '1' and ulaWaitCancel = '0' )
			( cpuMREQ = '1' and ( cpuA( 15 downto 14 ) = "01" ) and ulaWaitIo = '1' and ulaWaitCancel = '0' )
		) else '0';

	cpuWait <= cpuHaltAck or cpuMemoryWait or specDiskIfWait or cpuOneCycleWaitReq or ulaWait;

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

			if reset = '1' then
				cpuINT <= '1';
				cpuNMI <= '1';
			end if;

			refresh <= '0';

			if cpuReset = '1' or cpuHaltAck = '1' or specDiskIfWait = '1' or ( cpuMREQ = '0' and cpuRFSH = '0' ) then
				refresh <= '1';
			end if;

			cpuCLK_nowait_prev <= cpuCLK_nowait;

			if cpuCLK_nowait = '1' then

				if specIntCounter = 32 then
					cpuINT <= '1';
				end if;

				if cpuHaltAck = '0' and specDiskIfWait = '0' then
					specIntCounter <= specIntCounter + 1;
				end if;

				cpuOneCycleWaitAck <= cpuOneCycleWaitReq;

			end if;

			if cpuCLK = '1' then

				if cpuMREQ = '0' and cpuM1 = '0' then
					if accessRom = '0' or (divmmcEnabled = '1' and divmmcAmap = '0') or mb02Enabled = '1' then
						cpuNMI <= not cpuInvokeNMI;
					elsif accessRom = '1' then
						cpuNMI <= '1';
					end if;
				end if;

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
				elsif syncMode = 1 then
					if vCnt = 248 and hCnt = 8 then
						cpuINT <= cpuIntSkip;
						specIntCounter <= x"00000000";
						cpuIntSkip := '0';
					end if;
				else
					if vCnt = 248 and hCnt = 2 then
						cpuINT <= cpuIntSkip;
						specIntCounter <= x"00000000";
						cpuIntSkip := '0';
					end if;
				end if;

				--if cpuHaltAck = '1' or specDiskIfWait = '1' then
				if cpuHaltAck = '1' then
					cpuINT <= '1';
					cpuIntSkip := '1';
				end if;

			end if;

		end if;
	end process;

	------------------------------------------------------------------------------------------

	vramPage  <= "000001" & specPort7ffd(3) & "1";

	process( memclk )
		variable videoPage  : std_logic_vector(10 downto 0);

		variable portFfTemp : std_logic_vector(7 downto 0) := x"FF";

		variable vidReadPos : unsigned( 3 downto 0 ) := x"0";
		variable vidReq : std_logic := '0';
		variable vidReqUla : std_logic := '0';

	begin
		if memclk'event and memclk = '1' then

			if cpuCLK_nowait = '1' then
				--specPortFF <= x"ff";
				--specPortFF <= std_logic_vector( Hor_Cnt( 3 downto 0) & "0" & ChrC_Cnt );
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

				if vidReq = '1' and memReq2 = '0' and ( ( videoMode = 4 and vidReadPos(0) = '1' ) or cpuCLK_nowait_prev = '1' ) then

					memWr2 <= '0';
					memReq2 <= '1';

					if vidReadPos = 0 or vidReadPos = 2 then
						memAddress2 <= 		std_logic_vector(videoPage) & "0" &
											std_logic_vector(Ver_Cnt(4 downto 3)) &
											std_logic_vector(ChrR_Cnt) &
											std_logic_vector(Ver_Cnt(2 downto 0)) &
											std_logic_vector(Hor_Cnt(4 downto 1));
					else
						memAddress2 <= 		std_logic_vector(videoPage) & "0110" &
											std_logic_vector(Ver_Cnt(4 downto 0)) &
											std_logic_vector(Hor_Cnt(4 downto 1));
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

			if cpuTurbo = 0 and ( syncMode = 0 or syncMode = 1 ) then
				vidReqUla := '1';
			else
				vidReqUla := '0';
			end if;

			if vid_mode_arm = '1' then
				videoPage := armVideoPage( 10 downto 0 );
			else
				videoPage := "000" & vramPage;
			end if;

			specPortFF <= portFfTemp;

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

		if sysclk'event and sysclk = '0' and clk7m = '1' then
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

	borderAttr <= specPortFE( 2 downto 0 );
	speaker <= specPortFE( 4 );

	process( sysclk, clk7m )

	variable pix2 : std_logic_vector (7 downto 0);
	begin
		if sysclk'event and sysclk = '1' and clk7m = '1' then
			if paper_r = '0' then
				if( Shift_r(7) xor ( Attr_r(7) and Invert(4) ) ) = '1' then
					specB <= Attr_r(0) & Attr_r(0);
					specR <= Attr_r(1) & Attr_r(1);
					specG <= Attr_r(2) & Attr_r(2);
				else
					specB <= Attr_r(3) & Attr_r(3);
					specR <= Attr_r(4) & Attr_r(4);
					specG <= Attr_r(5) & Attr_r(5);
				end if;

				specY <= Attr_r(6);

			elsif blank_r = '1' then

				if armBorder(15) = '1' then
					specB <= armBorder(0) & armBorder(0);
					specR <= armBorder(1) & armBorder(1);
					specG <= armBorder(2) & armBorder(2);
				else
					specB <= borderAttr(0) & borderAttr(0);
					specR <= borderAttr(1) & borderAttr(1);
					specG <= borderAttr(2) & borderAttr(2);
				end if;

				specY <= '0';

			else
				specB <= "00";
				specR <= "00";
				specG <= "00";
				specY <= '0';

			end if;
		end if;
	end process;

	------------------------------------------------------------------------------------------------
	process( sysclk )
	begin
		if sysclk'event and sysclk = '1' then
			if ayymMode = '0' then
				weightAY_L <= weigthYmTable(to_integer(ayMode),0);
				weightAY_R <= weigthYmTable(to_integer(ayMode),1);
			else
				weightAY_L <= weigthAyTable(to_integer(ayMode),0);
				weightAY_R <= weigthAyTable(to_integer(ayMode),1);
			end if;
		end if;
	end process;


	covoxEnabled<= '0' when covoxMode = 0 else
						'1';
	ay1Enabled 	<= '0' when ayMode = AY_MODE_NONE else
						'1';
	ay2Enabled 	<= '1' when ayMode /= AY_MODE_NONE and turboSound = '1' else
						'0';

	soundLeft <= mixedOutputL;
	soundRight <= mixedOutputR;

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


				outData := unsigned( x"00" & std_logic_vector(outRight( 15 downto 0 )) & x"00" & std_logic_vector(outLeft( 15 downto 0 )) );

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

			if vgaVCounter( 0 ) = '0' or videoInterlace = '0' then
				vgaR <= temp( 23 downto 16 );
				vgaG <= temp( 15 downto 8 );
				vgaB <= temp( 7 downto 0 );
			else
				vgaR <= std_logic_vector( "0" & unsigned( temp( 23 downto 17 ) ) + unsigned( temp( 23 downto 18 ) ) );
				vgaG <= std_logic_vector( "0" & unsigned( temp( 15 downto 9 ) ) + unsigned( temp( 15 downto 10 ) ) );
				vgaB <= std_logic_vector( "0" & unsigned( temp( 7 downto 1 ) ) + unsigned( temp( 7 downto 2 ) ) );
			end if;

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
				if specY = '0' then
					rgbR <= specR & "000000";
					rgbG <= specG & "000000";
					rgbB <= specB & "000000";
				else
					rgbR <= specR & specR & specR & specR;
					rgbG <= specG & specG & specG & specG;
					rgbB <= specB & specB & specB & specB;
				end if;
			end if;

		end if;

	end process;

	VIDEO_R <= videoV when videoMode = 0 else						-- ( Composite --> SCART 20, 17 )
			"0" & rgbR( 7 downto 1 ) when videoMode = 1 else		-- ( VGA 1, 6  --> SCART 15, 13 )
			"0" & vgaR( 7 downto 1 );
	VIDEO_G <= "0" & videoC( 7 downto 1 ) when videoMode = 0 else	-- ( S-Video 4, 2 --> SCART 15, 13 )
			"0" & rgbG( 7 downto 1 ) when videoMode = 1 else		-- ( VGA 2, 7  --> SCART 11, 9 )
			"0" & vgaG( 7 downto 1 );
	VIDEO_B <= videoY when videoMode = 0 else						-- ( S-Video 3, 1 --> SCART 20, 17 )
			"0" & rgbB( 7 downto 1 ) when videoMode = 1 else		-- ( VGA 3, 8  --> SCART 7, 5 )
			"0" & vgaB( 7 downto 1 );

	VIDEO_HSYNC <= '1' when videoMode = 0 or videoMode = 1 else vgaHSync;		-- VGA 13 --> SCART 16
	VIDEO_VSYNC <= palSync when videoMode = 0 or videoMode = 1 else vgaVSync;	-- VGA 14 --> SCART 20

end rtl;
