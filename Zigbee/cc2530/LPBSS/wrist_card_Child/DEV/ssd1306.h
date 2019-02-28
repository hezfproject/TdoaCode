//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Delay Time
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void uDelay(unsigned char  );
void Delay(unsigned char  n);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Read/Write Sequence
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Write_Command(unsigned char);
void Write_Data(unsigned char);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Instruction Setting
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Set_Start_Column(unsigned char);                            // Set Column Start Address for Page 
void Set_Addressing_Mode(unsigned char );                        // Set Memory Addressing Mode
void Set_Column_Address(unsigned char , unsigned char );         // Set Column Address
void Set_Page_Address(unsigned char, unsigned char);   // Set Page Address
void Set_Start_Line(unsigned char);                    // Set Display Start Line
void Set_Contrast_Control(unsigned char);              // Set Contrast Control
void Set_Charge_Pump(unsigned char);                   // Set Charge Pump
void Set_Segment_Remap(unsigned char);                 // Set Segment Re-Map
void Set_Entire_Display(unsigned char);                // Set Entire Display On / Off
void Set_Inverse_Display(unsigned char);               // Set Inverse Display On/Off
void Set_Multiplex_Ratio(unsigned char);               // Set Multiplex Ratio
void Set_Display_On_Off(unsigned char);                // Set Display On/Off
void Set_Start_Page(unsigned char);                    // Set Page Start Address for Page Addressing Mode
void Set_Common_Remap(unsigned char);                  // Set COM Output Scan Direction
void Set_Display_Offset(unsigned char);                // Set Display Offset
void Set_Display_Clock(unsigned char);                 // Set Display Clock Divide Ratio / Oscillator Frequency
void Set_Precharge_Period(unsigned char);              // Set Pre-Charge Period
void Set_Common_Config(unsigned char);                 // Set COM Pins Hardware Configuration 
void Set_VCOMH(unsigned char);                         // Set VCOMH Deselect Level
void Set_NOP();                                        // Command for No Operation

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Regular Pattern (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fill_RAM(unsigned char);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Regular Pattern (Partial or Full Screen)
//
//    a: Start Page
//    b: End Page
//    c: Start Column
//    d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fill_Block(unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Checkboard (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Checkerboard();

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Frame (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Frame();

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Character (5x7)
//
//    a: Database
//    b: Ascii
//    c: Start Page
//    d: Start Column
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Show_Font57(unsigned char a, unsigned char b, unsigned char c, unsigned char d);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show String
//
//    a: Database
//    b: Start Page
//    c: Start Column
//    * Must write "0" in the end...
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Show_String(unsigned char a, unsigned char *Data_Pointer, unsigned char b, unsigned char c);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Pattern (Partial or Full Screen)
//
//    a: Start Page
//    b: End Page
//    c: Start Column
//    d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//void Show_Pattern(const UINT8 CODE *Data_Pointer, unsigned char a, unsigned char b, unsigned char c, unsigned char d);

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
void Vertical_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e);

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
void Horizontal_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e);

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
void Continuous_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f, unsigned char g, unsigned char h);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Deactivate Scrolling (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Deactivate_Scroll();

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Fade In (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fade_In();

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Fade Out (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fade_Out();

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Sleep Mode
//
//    "0x00" Enter Sleep Mode
//    "0x01" Exit Sleep Mode
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Sleep(unsigned char);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Connection Test
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Test();

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Initialization
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void OLED_Init_E();				// VCC Supplied Externally
void OLED_Init_I();				// VCC Generated by Internal DC/DC Circuit
void test_oled();














































