//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//  TW-9639TLBEG03 (9639-15p) Reference Code
//
//    Dot Matrix: 96*39
//    Driver IC : SSD1306 (Solomon Systech)
//    Interface : 8-bit 68XX/80XX Parallel, 3-/4-wire SPI
//    Revision  :
//    Date      : 2011/01/03
//    Author    :
//    Editor    : 
//
//  
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <ioCC2530.h>
//#define	M68				// 8-bit 68XX Parallel
						//   BS1=1; BS2=1
//#define		I80				// 8-bit 80XX Parallel
						//   BS1=0; BS2=1
#define	SPI				// 4-wire SPI
						//   BS1=0; BS2=0
						//   The unused pins should be connected with VSS mostly or floating (D2).
						//   Please refer to the SSD1306 specification for detail.


#define RES    P0_1
extern void HalLcd_HW_Write(unsigned char data);
extern void HalLcd_HW_Control(unsigned int cmd);
extern void HalLcd_HW_WaitUs(unsigned int i);
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Delay Time
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void uDelay(unsigned char l)
{

    HalLcd_HW_WaitUs(l); // 10 ms
}


void Delay(unsigned char n)
{
unsigned char i,j,k;

	for(k=0;k<n;k++)
	{
		for(i=0;i<131;i++)
		{
			for(j=0;j<15;j++)
			{
				uDelay(203);	
			}
		}
	}
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Read/Write Sequence
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifdef SPI					// 4-wire SPI
void Write_Command(unsigned char Data)
{

	HalLcd_HW_Control(Data);
}



void Write_Data(unsigned char Data)
{

	HalLcd_HW_Write(Data);
}
#endif


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Instruction Setting
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Set_Start_Column(unsigned char d)
{
	Write_Command(0x00+d%16);		// Set Lower Column Start Address for Page Addressing Mode
						//   Default => 0x00
	Write_Command(0x10+d/16);		// Set Higher Column Start Address for Page Addressing Mode
						//   Default => 0x10
}


void Set_Addressing_Mode(unsigned char d)
{
	Write_Command(0x20);			// Set Memory Addressing Mode
	Write_Command(d);			//   Default => 0x02
						//     0x00 => Horizontal Addressing Mode
						//     0x01 => Vertical Addressing Mode
						//     0x02 => Page Addressing Mode
}


void Set_Column_Address(unsigned char a, unsigned char b)
{
	Write_Command(0x21);			// Set Column Address
	Write_Command(a);			//   Default => 0x00 (Column Start Address)
	Write_Command(b);			//   Default => 0x7F (Column End Address)
}


void Set_Page_Address(unsigned char a, unsigned char b)
{
	Write_Command(0x22);			// Set Page Address
	Write_Command(a);			//   Default => 0x00 (Page Start Address)
	Write_Command(b);			//   Default => 0x07 (Page End Address)
}


void Set_Start_Line(unsigned char d)
{
	Write_Command(0x40|d);			// Set Display Start Line
						//   Default => 0x40 (0x00)
}


void Set_Contrast_Control(unsigned char d)
{
	Write_Command(0x81);			// Set Contrast Control
	Write_Command(d);			//   Default => 0x7F
}


void Set_Charge_Pump(unsigned char d)
{
	Write_Command(0x8D);			// Set Charge Pump
	Write_Command(0x10|d);			//   Default => 0x10
						//     0x10 (0x00) => Disable Charge Pump
						//     0x14 (0x04) => Enable Charge Pump
}


void Set_Segment_Remap(unsigned char d)
{
	Write_Command(0xA0|d);			// Set Segment Re-Map
						//   Default => 0xA0
						//     0xA0 (0x00) => Column Address 0 Mapped to SEG0
						//     0xA1 (0x01) => Column Address 0 Mapped to SEG127
}


void Set_Entire_Display(unsigned char d)
{
	Write_Command(0xA4|d);			// Set Entire Display On / Off
						//   Default => 0xA4
						//     0xA4 (0x00) => Normal Display
						//     0xA5 (0x01) => Entire Display On
}


void Set_Inverse_Display(unsigned char d)
{
	Write_Command(0xA6|d);			// Set Inverse Display On/Off
						//   Default => 0xA6
						//     0xA6 (0x00) => Normal Display
						//     0xA7 (0x01) => Inverse Display On
}


void Set_Multiplex_Ratio(unsigned char d)
{
	Write_Command(0xA8);			// Set Multiplex Ratio
	Write_Command(d);			//   Default => 0x3F (1/64 Duty)
}


void Set_Display_On_Off(unsigned char d)	
{
	Write_Command(0xAE|d);			// Set Display On/Off
						//   Default => 0xAE
						//     0xAE (0x00) => Display Off
						//     0xAF (0x01) => Display On
}


void Set_Start_Page(unsigned char d)
{
	Write_Command(0xB0|d);			// Set Page Start Address for Page Addressing Mode
						//   Default => 0xB0 (0x00)
}


void Set_Common_Remap(unsigned char d)
{
	Write_Command(0xC0|d);			// Set COM Output Scan Direction
						//   Default => 0xC0
						//     0xC0 (0x00) => Scan from COM0 to 63
						//     0xC8 (0x08) => Scan from COM63 to 0
}


void Set_Display_Offset(unsigned char d)
{
	Write_Command(0xD3);			// Set Display Offset
	Write_Command(d);			//   Default => 0x00
}


void Set_Display_Clock(unsigned char d)
{
	Write_Command(0xD5);			// Set Display Clock Divide Ratio / Oscillator Frequency
	Write_Command(d);			//   Default => 0x80
						//     D[3:0] => Display Clock Divider
						//     D[7:4] => Oscillator Frequency
}


void Set_Precharge_Period(unsigned char d)
{
	Write_Command(0xD9);			// Set Pre-Charge Period
	Write_Command(d);			//   Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks [Phase 1])
						//     D[3:0] => Phase 1 Period in 1~15 Display Clocks
						//     D[7:4] => Phase 2 Period in 1~15 Display Clocks
}


void Set_Common_Config(unsigned char d)
{
	Write_Command(0xDA);			// Set COM Pins Hardware Configuration
	Write_Command(0x02|d);			//   Default => 0x12 (0x10)
						//     Alternative COM Pin Configuration
						//     Disable COM Left/Right Re-Map
}


void Set_VCOMH(unsigned char d)
{
	Write_Command(0xDB);			// Set VCOMH Deselect Level
	Write_Command(d);			//   Default => 0x20 (0.77*VCC)
}


void Set_NOP()
{
	Write_Command(0xE3);			// Command for No Operation
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Global Variables
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define XLevelL		0x00
#define XLevelH		0x12
#define XLevel		((XLevelH&0x0F)*16+XLevelL)
#define Max_Column	96
#define Max_Row		40
#define	Brightness	0xAF


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Patterns
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
unsigned char UniV[][48]={			// UniV OLED
	0x00,0x00,0xFC,0xFC,0xFC,0xFC,0x00,0x00,0x00,0xFC,0xFC,0xFC,0xFC,0x00,0x00,0x00,0xE0,0xE0,0xE0,0xE0,0x60,0xE0,0xE0,0xE0,0xC0,0x00,0x00,0x00,0xEC,0xEC,0xEC,0xEC,0x00,0x00,0x00,0x04,0x3C,0xFC,0xFC,0xF0,0x80,0x80,0xF0,0xFC,0xFC,0x3C,0x04,0x00,
	0x00,0x00,0x07,0x0F,0x1F,0x1F,0x1C,0x18,0x1C,0x1F,0x1F,0x0F,0x07,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x00,0x1F,0x1F,0x1F,0x1F,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x00,0x00,0x00,0x00,0x00,0x01,0x0F,0x1F,0x1F,0x1F,0x1F,0x0F,0x01,0x00,0x00,0x00,
	0x00,0x80,0xE0,0xE0,0xF0,0x70,0x30,0x70,0xF0,0xE0,0xE0,0x80,0x00,0x00,0x00,0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xF0,0xF0,0xF0,0x30,0x30,0x30,0x30,0x30,0x00,0x00,0x00,0xF0,0xF0,0xF0,0xF0,0x30,0x70,0xF0,0xF0,0xE0,0x80,
	0x00,0x0F,0x3F,0x3F,0x7F,0x70,0x60,0x70,0x7F,0x3F,0x3F,0x0F,0x00,0x00,0x00,0x7F,0x7F,0x7F,0x7F,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x7F,0x7F,0x7F,0x7F,0x63,0x63,0x63,0x63,0x60,0x00,0x00,0x00,0x7F,0x7F,0x7F,0x7F,0x60,0x70,0x7F,0x7F,0x3F,0x0F,
};




//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Regular Pattern (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fill_RAM(unsigned char Data)
{
unsigned char i,j;

	for(i=0;i<8;i++)
	{
		Set_Start_Page(i);
		Set_Start_Column(0x00);

		for(j=0;j<128;j++)
		{
			Write_Data(Data);
		}
	}
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Regular Pattern (Partial or Full Screen)
//
//    a: Start Page
//    b: End Page
//    c: Start Column
//    d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fill_Block(unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
unsigned char i,j;
	
	for(i=a;i<(b+1);i++)
	{
		Set_Start_Page(i);
		Set_Start_Column(c);

		for(j=0;j<d;j++)
		{
			Write_Data(Data);
		}
	}
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Checkboard (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Checkerboard()
{
unsigned char i,j;
	
	for(i=0;i<8;i++)
	{
		Set_Start_Page(i);
		Set_Start_Column(0x00);

		//for(j=0;j<64;j++)
                for(j=0;j<64;j++)
		{
			Write_Data(0x55);
			Write_Data(0xaa);
		}
	}
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Frame (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Frame()
{
unsigned char i,j;
	
	Set_Start_Page(0x00);
	Set_Start_Column(XLevel);

	for(i=0;i<Max_Column;i++)
	{
		Write_Data(0x02);
	}

	Set_Start_Page(0x04);
	Set_Start_Column(XLevel);

	for(i=0;i<Max_Column;i++)
	{
		Write_Data(0x80);
	}

	Set_Start_Page(0x00);

	for(i=0;i<Max_Column;i+=(Max_Column-1))
	{
		Set_Start_Column(XLevel+i);

		Write_Data(0xFE);
	}

	for(i=1;i<5;i++)
	{
		Set_Start_Page(i);

		for(j=0;j<Max_Column;j+=(Max_Column-1))
		{
			Set_Start_Column(XLevel+j);

			Write_Data(0xFF);
		}
	}
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Character (5x7)
//
//    a: Database
//    b: Ascii
//    c: Start Page
//    d: Start Column
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Show_Font57(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{/*
unsigned char *Src_Pointer;
unsigned char i;

	switch(a)
	{
		case 1:
			Src_Pointer=&Ascii_1[(b-1)][0];
			break;
		case 2:
			Src_Pointer=&Ascii_2[(b-1)][0];
			break;
	}
	Set_Start_Page(c);
	Set_Start_Column(d);

	for(i=0;i<5;i++)
	{
		Write_Data(*Src_Pointer);
		Src_Pointer++;
	}
	Write_Data(0x00);*/
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show String
//
//    a: Database
//    b: Start Page
//    c: Start Column
//    * Must write "0" in the end...
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Show_String(unsigned char a, unsigned char *Data_Pointer, unsigned char b, unsigned char c)
{
unsigned char *Src_Pointer;

	Src_Pointer=Data_Pointer;
	Show_Font57(1,96,b,c);			// No-Break Space
						//   Must be written first before the string start...

	while(1)
	{
		Show_Font57(a,*Src_Pointer,b,c);
		Src_Pointer++;
		c+=6;
		if(*Src_Pointer == 0) break;
	}
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Pattern (Partial or Full Screen)
//
//    a: Start Page
//    b: End Page
//    c: Start Column
//    d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Vertical / Fade Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Upward)
//       "0x01" (Downward)
//    b: Set Top Fixed Area
//    c: Set Vertical Scroll Area
//    d: Set Numbers of Row Scroll per Step
//    e: Set Time Interval between Each Scroll Step
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Vertical_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e)
{
unsigned int i,j;	

	Write_Command(0xA3);			// Set Vertical Scroll Area
	Write_Command(b);			//   Default => 0x00 (Top Fixed Area)
	Write_Command(c);			//   Default => 0x40 (Vertical Scroll Area)

	switch(a)
	{
		case 0:
			for(i=0;i<c;i+=d)
			{
				Set_Start_Line(i);
				for(j=0;j<e;j++)
				{
					uDelay(200);
				}
			}
			break;
		case 1:
			for(i=0;i<c;i+=d)
			{
				Set_Start_Line(c-i);
				for(j=0;j<e;j++)
				{
					uDelay(200);
				}
			}
			break;
	}
	Set_Start_Line(0x00);
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Continuous Horizontal Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Rightward)
//       "0x01" (Leftward)
//    b: Define Start Page Address
//    c: Define End Page Address
//    d: Set Time Interval between Each Scroll Step in Terms of Frame Frequency
//    e: Delay Time
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Horizontal_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e)
{
	Write_Command(0x26|a);			// Horizontal Scroll Setup
	Write_Command(0x00);			// => (Dummy Write for First Parameter)
	Write_Command(b);
	Write_Command(d);
	Write_Command(c);
	Write_Command(0x2F);			// Activate Scrolling
	//Delay(e);
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Continuous Vertical / Horizontal / Diagonal Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Vertical & Rightward)
//       "0x01" (Vertical & Leftward)
//    b: Define Start Row Address (Horizontal / Diagonal Scrolling)
//    c: Define End Page Address (Horizontal / Diagonal Scrolling)
//    d: Set Top Fixed Area (Vertical Scrolling)
//    e: Set Vertical Scroll Area (Vertical Scrolling)
//    f: Set Numbers of Row Scroll per Step (Vertical / Diagonal Scrolling)
//    g: Set Time Interval between Each Scroll Step in Terms of Frame Frequency
//    h: Delay Time
//    * d+e must be less than or equal to the Multiplex Ratio...
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Continuous_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f, unsigned char g, unsigned char h)
{
	Write_Command(0xA3);			// Set Vertical Scroll Area
	Write_Command(d);			//   Default => 0x00 (Top Fixed Area)
	Write_Command(e);			//   Default => 0x40 (Vertical Scroll Area)

	Write_Command(0x29+a);			// Continuous Vertical & Horizontal Scroll Setup
	Write_Command(0x00);			//           => (Dummy Write for First Parameter)
	Write_Command(b);
	Write_Command(g);
	Write_Command(c);
	Write_Command(f);
	Write_Command(0x2F);			// Activate Scrolling
	//Delay(h);
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Deactivate Scrolling (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Deactivate_Scroll()
{
	Write_Command(0x2E);			// Deactivate Scrolling
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Fade In (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fade_In()
{
unsigned int i;	

	Set_Display_On_Off(0xAF);
	for(i=0;i<(Brightness+1);i++)
	{
		Set_Contrast_Control(i);
		uDelay(200);
		uDelay(200);
		uDelay(200);
	}
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Fade Out (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fade_Out()
{
unsigned int i;	

	for(i=(Brightness+1);i>0;i--)
	{
		Set_Contrast_Control(i-1);
		uDelay(200);
		uDelay(200);
		uDelay(200);
	}
	Set_Display_On_Off(0xAE);
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Sleep Mode
//
//    "0x00" Enter Sleep Mode
//    "0x01" Exit Sleep Mode
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Sleep(unsigned char a)
{
	switch(a)
	{
		case 0:
			Set_Display_On_Off(0xAE);
			Set_Entire_Display(0xA5);
			break;
		case 1:
			Set_Entire_Display(0xA4);
			Set_Display_On_Off(0xAF);
			break;
	}
}




//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Initialization
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void OLED_Init_E()				// VCC Supplied Externally
{
unsigned char i;

	RES=0;
	for(i=0;i<200;i++)
	{
		uDelay(200);
	}
	RES=1;
	uDelay(20);
	Set_Display_On_Off(0xAE);		// Display Off (0xAE/0xAF)
	Set_Display_Clock(0x80);		// Set Clock as 160 Frames/Sec
	Set_Multiplex_Ratio(0x27);		// 1/39 Duty (0x0F~0x3F)
	Set_Display_Offset(0x00);		// Shift Mapping RAM Counter (0x00~0x3F)
	Set_Start_Line(0x00);			// Set Mapping RAM Display Start Line (0x00~0x3F)
	Set_Charge_Pump(0x10);			// Disable Built-in DC/DC Converter (0x10/0x14)
	Set_Addressing_Mode(0x02);		// Set Page Addressing Mode (0x00/0x01/0x02)
	Set_Segment_Remap(0xA1);		// Set SEG/Column Mapping (0xA0/0xA1)
	Set_Common_Remap(0xC8);			// Set COM/Row Scan Direction (0xC0/0xC8)
	Set_Common_Config(0x12);		// Set Alternative Configuration (0x02/0x12)
	Set_Contrast_Control(Brightness);	// Set SEG Output Current
	Set_Precharge_Period(0x25);		// Set Pre-Charge as 2 Clocks & Discharge as 5 Clock
	Set_VCOMH(0x20);			// Set VCOM Deselect Level
	Set_Entire_Display(0xA4);		// Disable Entire Display On (0xA4/0xA5)
	Set_Inverse_Display(0xA6);		// Disable Inverse Display On (0xA6/0xA7)

	Fill_RAM(0x00);				// Clear Screen

	Set_Display_On_Off(0xAF);		// Display On (0xAE/0xAF)
}


void OLED_Init_I()				// VCC Generated by Internal DC/DC Circuit
{
unsigned char i;

	RES=0;
	for(i=0;i<20;i++)
	{
		uDelay(100);
	}
	uDelay(200);
	RES=1;
	uDelay(200);
//		RES=0;
	Set_Display_On_Off(0xAE);		// Display Off (0xAE/0xAF)
	Set_Display_Clock(0x80);		// Set Clock as 160 Frames/Sec
	Set_Multiplex_Ratio(0x27);		// 1/39 Duty (0x0F~0x3F)
	Set_Display_Offset(0x00);		// Shift Mapping RAM Counter (0x00~0x3F)
	Set_Start_Line(0x00);			// Set Mapping RAM Display Start Line (0x00~0x3F)
	Set_Charge_Pump(0x14);			// Enable Built-in DC/DC Converter (0x10/0x14)
	Set_Addressing_Mode(0x02);		// Set Page Addressing Mode (0x00/0x01/0x02)
	Set_Segment_Remap(0xA1);		// Set SEG/Column Mapping (0xA0/0xA1)
	Set_Common_Remap(0xC8);			// Set COM/Row Scan Direction (0xC0/0xC8)
	Set_Common_Config(0x12);		// Set Alternative Configuration (0x02/0x12)
	Set_Contrast_Control(Brightness);	// Set SEG Output Current
	Set_Precharge_Period(0x25);		// Set Pre-Charge as 2 Clocks & Discharge as 5 Clock
	Set_VCOMH(0x20);			// Set VCOM Deselect Level
	Set_Entire_Display(0xA4);		// Disable Entire Display On (0xA4/0xA5)
	Set_Inverse_Display(0xA6);		// Disable Inverse Display On (0xA6/0xA7)
	

	Fill_RAM(0x00);				// Clear Screen

	Set_Display_On_Off(0xAF);		// Display On (0xAE/0xAF)
	
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Main Program
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
/*
void test_oled()
{
    unsigned char Name[]={55,73,83,69,35,72,73,80,0};
                                                    // WiseChip
    unsigned char Tel[]={11,24,24,22,13,19,23,13,21,24,23,17,22,24,0};
 //   unsigned char Tel1[]={0x00,0x10,0x10,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00};
                         
//Set_Addressing_Mode(0x10);	
    while(1)
    {
    // Show Pattern - UniV OLED

      Show_Pattern(&UniV[0][0],0x01,0x05,XLevel+0x18,0x30);

            Delay(1);

    // Fade In/Out (Full Screen)
    //        Fade_Out();
     //       Fade_In();
     //       Fade_Out();
      //      Fade_In();


    // Scrolling (Partial or Full Screen)
            Vertical_Scroll(0x00,0x00,Max_Row,0x01,0x20);
                                            // Upward
            Delay(1);
            Vertical_Scroll(0x01,0x00,Max_Row,0x01,0x20);
                                            // Downward
            Delay(1);
            Deactivate_Scroll();
            Continuous_Scroll(0x00,0x00,0x00,0x00,0x17,0x01,0x00,0x01);
                                            // Upward - Top Area  上一半部分向上移动
            Continuous_Scroll(0x00,0x00,0x00,0x00,0x17,0x16,0x00,0x01);
                                            // Downward - Top Area 上一半部分向下移动
            Continuous_Scroll(0x00,0x00,0x02,0x00,0x17,0x01,0x00,0x02);
                                            // Up & Rightward - Top Area 上一半部分向右上移动
            Continuous_Scroll(0x01,0x00,0x02,0x00,0x17,0x16,0x00,0x02);
                                            // Down & Leftward - Top Area  上一半部分向左下移动
            Continuous_Scroll(0x01,0x03,0x04,0x00,0x17,0x01,0x00,0x02);
                                            // Upward - Top Area   上一半部分靠右边沿向上移动
                                            // Leftward - Bottom Area   下一半部分向左移动
            Continuous_Scroll(0x00,0x03,0x04,0x00,0x17,0x16,0x00,0x02);
                                            // Downward - Top Area  上一半部分靠右边沿向下移动
                                            // Rightward - Bottom Area     下一半部分向右移动
            Deactivate_Scroll(); //停止移动

    // All Pixels On (Test Pattern)
            Fill_RAM(0xFF);       //全亮
            uDelay(1);
Fill_RAM(0x00);	
    // Checkerboard (Test Pattern)
            Checkerboard();    //网格点
            Delay(1);
            

    // Frame (Test Pattern)
            Frame();     //边框
            Delay(1);

    // Show String - WiseChip +886-37-587168
            Show_String(1,Name,0x01,XLevel+0x18);
            Show_String(1,Tel,0x03,XLevel+0x06);
            Delay(1);
            Fill_RAM(0x00);			// Clear Screen
    }
}
*/