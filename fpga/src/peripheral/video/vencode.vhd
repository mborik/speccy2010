library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity vencode is
	port(
		clk84m  : in std_logic;
		reset   : in std_logic;
		
		subcarrierDelta : in unsigned( 23 downto 0 );

		-- Video Input
		videoR : in std_logic_vector(5 downto 0);
		videoG : in std_logic_vector(5 downto 0);
		videoB : in std_logic_vector(5 downto 0);

		videoHS_n : in std_logic;
		videoVS_n : in std_logic;
		videoPS_n : in std_logic;

		-- Video Output
		videoY    : out std_logic_vector(7 downto 0);
		videoC    : out std_logic_vector(7 downto 0);
		videoV    : out std_logic_vector(7 downto 0)
	);
end vencode;

architecture rtl of vencode is

	signal carrier : 	unsigned(23 downto 0);
	signal clk_en  : std_logic;

	signal vcounter : unsigned(8 downto 0);
	signal hcounter : unsigned(12 downto 0);

	signal burphase : std_logic := '0';

	signal window_v : std_logic;
	signal window_h : std_logic;
	signal window_c : std_logic;

	signal ivideoR  : unsigned(5 downto 0);
	signal ivideoG  : unsigned(5 downto 0);
	signal ivideoB  : unsigned(5 downto 0);

	signal Y1 : unsigned(13 downto 0);
	signal Y2 : unsigned(13 downto 0);
	signal Y3 : unsigned(13 downto 0);
	signal U1 : unsigned(13 downto 0);
	signal U2 : unsigned(13 downto 0);
	signal U3 : unsigned(13 downto 0);
	signal V1 : unsigned(13 downto 0);
	signal V2 : unsigned(13 downto 0);
	signal V3 : unsigned(13 downto 0);

	signal burstUV  : signed(13 downto 0);

	signal prevY  : signed( 13 downto 0 );
	signal prevU : signed( 13 downto 0 );
	signal prevV : signed( 13 downto 0 );
	signal prevC : signed( 23 downto 0 );

	signal inY  : std_logic_vector(8 downto 0);
	signal inU : std_logic_vector(8 downto 0);
	signal inV : std_logic_vector(8 downto 0);
	signal inSin : std_logic_vector(15 downto 0);
	signal inCos : std_logic_vector(15 downto 0);

	signal sin : signed( 15 downto 0 );
	signal cos : signed( 15 downto 0 );
	signal pcos : signed( 15 downto 0);

	signal Um : signed( 23 downto 0 );
	signal Vm : signed( 23 downto 0 );

	signal Y  : signed(7 downto 0);
	signal U : signed(7 downto 0);
	signal V : signed(7 downto 0);
	signal C  : signed(7 downto 0);

	constant vref : signed(13 downto 0) := to_signed( 54, 8 ) & "000000";
	constant cent : signed(7 downto 0) := X"80";

	component fir
		port(
			clk : in std_logic;
			clk_en : in std_logic;
			reset : in std_logic;
			input : in std_logic_vector(8 downto 0);
			output : out std_logic_vector(8 downto 0)		
		);
	end component;
  
	component delay
		port(
			clk : in std_logic;
			clk_en : in std_logic;
			reset : in std_logic;
			input : in std_logic_vector(8 downto 0);
			output : out std_logic_vector(8 downto 0)	
		);
	end component;

	component sinus
		port(
			phase : in std_logic_vector(7 downto 0);
			sinus : out std_logic_vector(15 downto 0)
		);
	end component;	
	
begin

	U00 : fir port map( clk84m, clk_en, reset, std_logic_vector( "0" & prevY( 13 downto 6 ) ), inY );	
	U01 : fir port map( clk84m, clk_en, reset, std_logic_vector( prevU( 13 downto 13 ) & prevU( 13 downto 6 ) ), inU );	
	U02 : fir port map( clk84m, clk_en, reset, std_logic_vector( prevV( 13 downto 13 ) & prevV( 13 downto 6 ) ), inV );	
	
	U03 : sinus port map( std_logic_vector( carrier( 23 downto 16 ) ), inSin );
	U04 : sinus port map( std_logic_vector( carrier( 23 downto 16 ) + b"01000000" ), inCos );
	
--	Y =  0.299 R + 0.587 G + 0.114 B
--	U = -0.147 R - 0.289 G + 0.436 B
--	V =  0.615 R - 0.515 G - 0.100 B

	--clk_en <= '1';
	--clk_en <= hcounter( 0 );
	clk_en <= '1' when hcounter( 1 downto 0 ) = "00" else '0';

	Y1 <= ( to_unsigned( integer( 0.299 * 256 / 2 + 0.5 ), 8 ) * ivideoR );
	Y2 <= ( to_unsigned( integer( 0.587 * 256 / 2 + 0.5 ), 8 ) * ivideoG );
	Y3 <= ( to_unsigned( integer( 0.114 * 256 / 2 + 0.5 ), 8 ) * ivideoB );

	U1 <= ( to_unsigned( integer( 0.147 * 256 / 2 + 0.5 ), 8 ) * ivideoR );
	U2 <= ( to_unsigned( integer( 0.289 * 256 / 2 + 0.5 ), 8 ) * ivideoG );
	U3 <= ( to_unsigned( integer( 0.436 * 256 / 2 + 0.5 ), 8 ) * ivideoB );

	V1 <= ( to_unsigned( integer( 0.615 * 256 / 2 + 0.5 ), 8 ) * ivideoR );
	V2 <= ( to_unsigned( integer( 0.515 * 256 / 2 + 0.5 ), 8 ) * ivideoG );
	V3 <= ( to_unsigned( integer( 0.100 * 256 / 2 + 0.5 ), 8 ) * ivideoB );
	
    prevY <= to_signed( 0, 14 ) when videoPS_n = '0' else
		vref + signed( Y1 + Y2 + Y3 ) when ( window_h = '1' and window_v = '1' ) else
		vref;

    burstUV <= to_signed( 32, 8 ) & "000000" when ( window_c = '1' and window_v = '1' ) else
		( others => '0' );
  
	prevU <= signed( U3 - U1 - U2 ) when ( window_h = '1' and window_v = '1' ) else 
		( -burstUV );

	prevV <= signed( V1 - V2 - V3 ) when ( window_h = '1' and window_v = '1' ) else
		( burstUV );
		
	--Y <= prevY( 13 downto 6 );
	--U <= prevU( 13 downto 6 );
	--V <= prevV( 13 downto 6 );
	
    Y <= signed( inY( 7 downto 0 ) );
    U <= signed( inU( 7 downto 0 ) );
    V <= signed( inV( 7 downto 0 ) );
    
    sin <= signed( inSin( 15 downto 0 ) );
    cos <= signed( inCos( 15 downto 0 ) );
    
    pcos <= cos when burphase = '0' else -cos;
    
    Um <= U * sin;
    Vm <= V * pcos;
    
    prevC <= Um + Vm;
    C <= prevC( 22 downto 15 );

 process(clk84m)

	variable ivideoVS_n : std_logic;
	variable ivideoHS_n : std_logic;

	begin

	if ( clk84m'event and clk84m = '1' ) then

		if ( videoHS_n = '0' and ivideoHS_n = '1' ) then
			vcounter <= vcounter + 1;
			burphase <= not burphase;
			hcounter <= ( others => '0' );
		else
			carrier <= carrier + subcarrierDelta;
			hcounter <= hcounter + 1;
		end if;

		if (videoVS_n = '0' and ivideoVS_n = '1') then
		
			vcounter <= ( others => '0' );
			carrier <= ( others => '0' );
			burphase <= '0';
			
		end if;

		if ( vcounter = ( 10 - 1 ) ) then
			window_v <= '1';
		elsif ( vcounter = ( 310 - 1 ) ) then
			window_v <= '0';
		end if;

		if (hcounter = ( integer( 10.5 * 84 ) - 1 ) ) then
			window_h <= '1';
		elsif ( hcounter = ( integer( 62.5 * 84 ) - 1 ) ) then
			window_h <= '0';
		end if;

		if ( hcounter = ( integer( 5.6 * 84 ) - 1 ) ) then
			window_c <= '1';
		elsif ( hcounter = ( integer( 8.1 * 84 ) - 1 ) ) then
			window_c <= '0';
		end if;

		videoY <= std_logic_vector( Y );
		videoC <= std_logic_vector( cent + C );
		videoV <= std_logic_vector( Y + C );

		ivideoR <= unsigned( videoR );
		ivideoG <= unsigned( videoG );
		ivideoB <= unsigned( videoB );

		ivideoVS_n := videoVS_n;
		ivideoHS_n := videoHS_n;

	end if;
	end process;

end rtl;
