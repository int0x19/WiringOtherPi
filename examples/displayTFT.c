/* 
 * File:   displayTFT.c
 * Author: mariuszb /flexiti
 *
 * Created on 3 czerwca 2016, 08:51
 */


#include <stdio.h>
#include <wiringPi.h>
#include <stdint.h>
#include <time.h>
#include<math.h>


#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "fonts.h"



//#define FAST_DATA   // my own dedicated fast port write instead of the typical WiringOP, 5 time faster
                      // Warning: port are fixed, if you have other change carefully

//#define DS_1820      // if DS1820 is connected (data on pin 37)
                    

#define BLINK_LED 29


#define CSX  7
#define DCX  1
#define WRX 0
#define RES 11

/* LCD color */
#define White          0xFFFF
#define Black          0x0000
#define Blue           0x001F
#define Blue2          0x051F
#define Red            0xF800
#define Magenta        0xF81F
#define Green          0x07E0
#define Cyan           0x7FFF
#define Yellow         0xFFE0

#define SUNXI_GPIO_BASE (0x01C20800)
#define MAP_SIZE	(4096*2)
#define MAP_MASK	(MAP_SIZE - 1)
#define	BLOCK_SIZE		(4*1024)
#define GPIO_BASE_BP		(0x01C20000)


int pins[16] = {3,4,5,12,6,13,14,10,7,1,0,11};  //all used

int pinsD[8] = {3,4,5,12,6,13,14,10};  //data only pins


static volatile uint32_t *gpio1 ;
int   fd ;

 char devPath[128]; // Path to DS1820 device

 uint32_t readl1(uint32_t addr)
{
	  uint32_t val = 0;
	  uint32_t mmap_base = (addr & ~MAP_MASK);
	  uint32_t mmap_seek = ((addr - mmap_base) >> 2);
	  val = *(gpio1 + mmap_seek);
	  return val;
	
}
void writel1(uint32_t val, uint32_t addr)
{
	  uint32_t mmap_base = (addr & ~MAP_MASK);
	  uint32_t mmap_seek = ((addr - mmap_base) >> 2);
	  *(gpio1 + mmap_seek) = val;
}


void send_data_int(unsigned int Fcolor)   //write 2 bytes of color
 {
 #ifdef FAST_DATA 
    uint32_t regval = 0;
// ----------------------------------------------------------------------------- 
// color high  
// -----------------------------------------------------------------------------    
    uint32_t phyaddr = SUNXI_GPIO_BASE + (2 * 36) + 0x10; // +0x10 -> data reg C
    regval = readl1(phyaddr);
    regval &= 0xFFFFFF60;
    regval += (Fcolor >> 11) & 1; //bit 0
    regval += (Fcolor >> 12) & 2;  //1
    regval += (Fcolor >> 12) & 4;  //2
    regval += (Fcolor >> 12) & 8;  //3
    regval += (Fcolor >> 5) & 0x10;  //4
    regval += (Fcolor >> 3) & 0x80;  //7
    writel1(regval, phyaddr);
    
    phyaddr = SUNXI_GPIO_BASE  + 0x10; // +0x10 -> data reg A
    regval = readl1(phyaddr);
    regval &= 0xFFFFFFF3;
    regval += (Fcolor >> 10) & 4; //bit 2
    regval += (Fcolor >> 5) & 8;  //3
    regval &= 0xFFFFFFFD;   //strobe
    writel1(regval, phyaddr); 
    regval |= 2;
    writel1(regval, phyaddr);  
    
// color low    
    
    phyaddr = SUNXI_GPIO_BASE + (2 * 36) + 0x10; // +0x10 -> data reg C
    regval = readl1(phyaddr);
    regval &= 0xFFFFFF60;
    regval += (Fcolor >> 3) & 1; //bit 0
    regval += (Fcolor >> 4) & 2;  //1
    regval += (Fcolor >> 4) & 4;  //2
    regval += (Fcolor >> 4) & 8;  //3
    regval += (Fcolor << 3) & 0x10;  //4
    regval += (Fcolor << 5) & 0x80;  //7
    writel1(regval, phyaddr);
     
    phyaddr = SUNXI_GPIO_BASE  + 0x10; // +0x10 -> data reg A
    regval = readl1(phyaddr);
    regval &= 0xFFFFFFF3;
    regval += (Fcolor >> 2) & 4; //bit 2
    regval += (Fcolor << 3) & 8;  //3
    regval &= 0xFFFFFFFD;   //strobe
    writel1(regval, phyaddr); 
    regval |= 2;
    writel1(regval, phyaddr);     

    
#else
    digitalWrite(3, (Fcolor >> 8) & 1); 
     digitalWrite(4, (Fcolor >> 9) & 1); 
     digitalWrite(5, (Fcolor >> 10) & 1);   
     digitalWrite(12, (Fcolor >> 11) & 1);   
     digitalWrite(6, (Fcolor >> 12) & 1); 
     digitalWrite(13, (Fcolor >> 13) & 1); 
     digitalWrite(14, (Fcolor >> 14) & 1);   
     digitalWrite(10, (Fcolor >> 15) & 1);  
    digitalWrite(WRX, LOW);   
    digitalWrite(WRX, HIGH);       
     digitalWrite(3, (Fcolor >> 0) & 1); 
     digitalWrite(4, (Fcolor >> 1) & 1); 
     digitalWrite(5, (Fcolor >> 2) & 1);   
     digitalWrite(12, (Fcolor >> 3) & 1);   
     digitalWrite(6, (Fcolor >> 4) & 1); 
     digitalWrite(13, (Fcolor >> 5) & 1); 
     digitalWrite(14, (Fcolor >> 6) & 1);   
     digitalWrite(10, (Fcolor >> 7) & 1);  
    digitalWrite(WRX, LOW);   
    digitalWrite(WRX, HIGH);   
     
#endif    
  
 } 

void set_CS(int status)
{
#ifdef FAST_DATA 
    uint32_t regval=0;
    uint32_t phyaddr = 0;
// ----------------------------------------------------------------------------- 
// CS set/reset
// -----------------------------------------------------------------------------    
    phyaddr = SUNXI_GPIO_BASE + 0x10; // +0x10 -> data reg A
    regval = readl1(phyaddr);
    if (status == LOW)
      regval &= 0xFFFFFFBF;
    else
      regval |= 0x40;
    writel1(regval, phyaddr);
 
#else 
   digitalWrite(CSX, status); 
#endif    
}



void send_data_byte (unsigned char byte)
{
    
#ifdef FAST_DATA
    uint32_t regval = 0;
    uint32_t phyaddr = SUNXI_GPIO_BASE + (2 * 36) + 0x10; // +0x10 -> data reg C
    regval = readl1(phyaddr);
    regval &= 0xFFFFFF60;
    regval += (byte >> 3) & 1; //bit 0
    regval += (byte >> 4) & 2;  //1
    regval += (byte >> 4) & 4;  //2
    regval += (byte >> 4) & 8;  //3
    regval += (byte << 3) & 0x10;  //4
    regval += (byte << 5) & 0x80;  //7
    writel1(regval, phyaddr);
    
    phyaddr = SUNXI_GPIO_BASE  + 0x10; // +0x10 -> data reg A
    regval = readl1(phyaddr);
    regval &= 0xFFFFFFF3;
    regval += (byte >> 2) & 4; //bit 2
    regval += (byte << 3) & 8;  //3
    regval &= 0xFFFFFFFD;   //strobe
    writel1(regval, phyaddr);
    regval |= 2;
    writel1(regval, phyaddr);   
      
#else    
     
     digitalWrite(3, (byte >> 0) & 1); 
     digitalWrite(4, (byte >> 1) & 1); 
     digitalWrite(5, (byte >> 2) & 1);   
     digitalWrite(12, (byte >> 3) & 1);   
     digitalWrite(6, (byte >> 4) & 1); 
     digitalWrite(13, (byte >> 5) & 1); 
     digitalWrite(14, (byte >> 6) & 1);   
     digitalWrite(10, (byte >> 7) & 1);  
 
    digitalWrite(WRX, LOW);   
    digitalWrite(WRX, HIGH);   
#endif     
  

}

void send_cmd(unsigned char byte)
{
    #ifdef FAST_DATA 
    uint32_t regval=0;
    uint32_t phyaddr = 0;
// ----------------------------------------------------------------------------- 
// DC set/reset
// -----------------------------------------------------------------------------    
    phyaddr = SUNXI_GPIO_BASE + (3 * 36)+ 0x10; // +0x10 -> data reg D
    regval = readl1(phyaddr);
    regval &= 0xFFFFBFFF;             //DC low
    writel1(regval, phyaddr);
    
    send_data_byte (byte);
    
    regval = readl1(phyaddr);    
    regval |= 0x4000;            //DC high
    writel1(regval, phyaddr);    
  
    
#else
  digitalWrite(DCX, LOW);
  send_data_byte(byte);
  digitalWrite(DCX, HIGH);
#endif
}


void Init() {
int x;
    wiringPiSetup () ;

    pinMode (BLINK_LED, OUTPUT);  //led  
  
    //# prepare all LCD pins for output 
    for (x=0;x<12;x++)
    { 
        pinMode (pins[x], OUTPUT);   
        digitalWrite(pins[x], HIGH);
    }
  
    //# hard reset sequence
    digitalWrite(RES, HIGH);
    delay(500); //  ## 5 ms
    digitalWrite(RES, LOW);  
    delay(1000); //  ## 15 ms
    digitalWrite(RES, HIGH);    
    delay(150); //  ## 15 ms

                 
    digitalWrite(CSX, HIGH);      
    digitalWrite(WRX, HIGH);     
    digitalWrite(CSX, LOW);       
#ifdef FAST_DATA  
  fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC); 
  gpio1 = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_BASE_BP);
#endif  
  
}


void InitLCD ()
{


set_CS(LOW);   
send_cmd( 0x11 );   //#Exit Sleep
delay(200); 
send_cmd(0xE0); 
send_data_byte(0x00); 
send_data_byte(0x04); 
send_data_byte(0x0E); 
send_data_byte(0x08); 
send_data_byte(0x17); 
send_data_byte(0x0A); 
send_data_byte(0x40); 
send_data_byte(0x79); 
send_data_byte(0x4D); 
send_data_byte(0x07); 
send_data_byte(0x0E); 
send_data_byte(0x0A); 
send_data_byte(0x1A); 
send_data_byte(0x1D); 
send_data_byte(0x0F);  

send_cmd(0xE1); 
send_data_byte(0x00); 
send_data_byte(0x1B); 
send_data_byte(0x1F); 
send_data_byte(0x02); 
send_data_byte(0x10); 
send_data_byte(0x05); 
send_data_byte(0x32); 
send_data_byte(0x34); 
send_data_byte(0x43); 
send_data_byte(0x02); 
send_data_byte(0x0A); 
send_data_byte(0x09); 
send_data_byte(0x33); 
send_data_byte(0x37); 
send_data_byte(0x0F); 
  
  
  
send_cmd( 0xD1 );   //  # VCOM Control                                                                     
send_data_byte( 0x00 );   // #    SEL/VCM                                                                      
send_data_byte( 0x0C );  //  #    VCM    0c                                                                  
send_data_byte( 0x0F );  //  #    VDV    0f                                                              

send_cmd( 0xD0 );   //  # Power_Setting                                                                     
send_data_byte( 0x07 );  //  #    VC                                                                      
send_data_byte( 0x04 );  //  #    BT     04                                                             
send_data_byte( 0x00 );  //  #    VRH    00                                                                  

send_cmd( 0x36 );  //   # Set_address_mode
send_data_byte( 0x48 );  //
//# 0x48 = 0b01001000
///# 0 - top to bottom
///# 1 - left to right
//# 0 - v/h reverse
//# 0 - refresh top to bottom
//# 1 - RGB or BGR  (here BGR order)
//# 0 - <skip>
//# 0 - no hflip
//# 0 - no flip

send_cmd( 0x3A );     //# Set pixel format
send_data_byte( 0x05 );    //DBI:16bit/pixel (65,536 colors)
  

send_cmd( 0x51 );    // #brignets 
send_data_byte( 0 );   
  
send_cmd( 0x53 );    // #brignets 
send_data_byte( 0 );     
  
send_cmd( 0x55 );    // # Addaptive brignets 
send_data_byte( 0); 
  
send_cmd( 0x5E );    // # Addaptive brignets 
send_data_byte( 0 );   
  
  
send_cmd(0xC0); 
send_data_byte(0x18); 
send_data_byte(0x16); 

send_cmd(0xC1); 
send_data_byte(0x41); 

send_cmd(0xC5); 
send_data_byte(0x00); 
send_data_byte(0x1E); //VCOM
send_data_byte(0x80);   
 
     
send_cmd(0xC5); 
send_data_byte(0x00); 
send_data_byte(0x1E); //VCOM
send_data_byte(0x80); 
 
  
send_cmd( 0xD2 );     //# Power setting
send_data_byte( 0x01 );   //Gamma Driver Amplifier:1.00, Source Driver Amplifier: 1.00
send_data_byte( 0x44 );
 
  
send_cmd(0xC0); 
send_data_byte(0x18); 
send_data_byte(0x16); 

send_cmd(0xC1); 
send_data_byte(0x41); 

send_cmd(0xC5); 
send_data_byte(0x00); 
send_data_byte(0x1E); //VCOM
send_data_byte(0x80); 
  
  
  //---
send_cmd(0xB1);   //Frame rate 70HZ  
send_data_byte(0xB0); 

send_cmd(0xB4); 
send_data_byte(0x02);   

send_cmd(0xB9); //PWM Settings for Brightness Control
send_data_byte(0x01);// Disabled by default.
send_data_byte(0xFF); //0xFF = Max brightness
send_data_byte(0xFF);
send_data_byte(0x18);
  
  
  
send_cmd(0xE9); 
send_data_byte(0x00);
 
send_cmd(0XF7);    
send_data_byte(0xA9); 
send_data_byte(0x51); 
send_data_byte(0x2C); 
send_data_byte(0x82);
//---  
  
  
  
    send_cmd( 0xC8 );     //# Set Gamma
    send_data_byte( 0x04 );
    send_data_byte( 0x67 );
    send_data_byte( 0x35 );
    send_data_byte( 0x04 );
    send_data_byte( 0x08 );
    send_data_byte( 0x06 );
    send_data_byte( 0x24 );
    send_data_byte( 0x01 );
    send_data_byte( 0x37 );
    send_data_byte( 0x40 );
    send_data_byte( 0x03 );
    send_data_byte( 0x10 );
    send_data_byte( 0x08 );
    send_data_byte( 0x80 );
    send_data_byte( 0x00 );

    send_cmd( 0x2A );  //column address
    send_data_byte( 0x00 );
    send_data_byte( 0x00 );
    send_data_byte( 0x00 );
    send_data_byte( 0xeF );
  
    send_cmd( 0x2B );   //page address
    send_data_byte( 0x00 );
    send_data_byte( 0x00 );
    send_data_byte( 0x01 );
    send_data_byte( 0x8F );
  
  
    send_cmd( 0x29 ); //#display on
    send_cmd( 0x2C ); //#display on

    set_CS(HIGH);    
    
 
}  

//
// set region to write data
//
void TFT_Set_Address(unsigned int x1,unsigned int y1,unsigned int x2, unsigned int y2)
{
 
     set_CS(LOW); 
     
     send_cmd(0x2b); //# Set_page_address
     send_data_int(y1);
     send_data_int(y2);

     send_cmd(0x2a);  // # Set_column_address
     send_data_int(x1);
     send_data_int(x2);

     
     set_CS(HIGH);        
}


//
// fill box with color
//
void TFT_Box(unsigned int x,unsigned int y,unsigned int x1,unsigned int y1,unsigned int color)
{  
    unsigned int i,j;
  TFT_Set_Address(y,x,y1,x1);

  set_CS(LOW); 
  send_cmd(0x2c);                      // # Write_memory_start
 
  for(i = y; i <= y1; i++)
    for(j = x; j <= x1; j++)
           send_data_int(color);

  set_CS(HIGH); 
}

//
// fill all display
//
void TFT_Clear(unsigned int color)
{
 TFT_Box(0,0,479,319,color);   
}




//********************************************************************
void TFT_Char(char C,unsigned int x,unsigned int y,char DimFont,unsigned int Fcolor,unsigned int Bcolor)
{
const char *PtrFont;
unsigned int Cptrfont,rows,size,a;	
unsigned int font16x16[16];
unsigned char k,i,j;


if (DimFont == 24)
{
        TFT_Set_Address(x,y,x+23,y+23);
	Cptrfont = (C-'0')*72;
        set_CS(LOW); 
        send_cmd(0x2c);                    // # Write_memory_start

           for(k=0;k<2;k++) 
           {
             for(j = 0; j < 8; j++)
             {        
                for(i=0;i<24;i++)
                { 
                 if ((Font24[Cptrfont+((23-i)*3) + (k)] >> (7-j))  & 1)                     
                    send_data_int(Fcolor);
                  else
                    send_data_int(Bcolor);
                 
                }  
             }
           }
	
    set_CS(HIGH); 
}
else
if (((DimFont == 32) || (DimFont == 64)) && (C != ' '))  //32x48
{
	
    if (DimFont == 32)
    {  
        rows=4;
        size=1;
    }    
    else
    {   
        rows=8;
        size=2;
    }            
   
         
	if ((C == ':') || (C == '.'))
        {
		
              TFT_Set_Address(x,y,(x+(12*rows)-1),y+(4*rows)-1);  
              rows=2;
        }
        else
        {  
          TFT_Set_Address(x,y,(x+(12*rows)-1),y+(8*rows)-1);  
          rows=4;
        }

	   Cptrfont = (C-'0')*192;
           if (C == '.')
              Cptrfont=1920+192+192+96; 
           if (C == '*')
              Cptrfont=1920;   
           if (C == 'C')
              Cptrfont=1920+192;   
            if (C == ':')
              Cptrfont=1920+192+192;  

      set_CS(LOW); 
      send_cmd(0x2c);                     // # Write_memory_start

	     for(i = 0; i < rows; i++)
                {
		
       		for(j=0; j < 8; j++)
                    {
                    for(a=0;a<size;a++)
                      {  
                             for(k = 0; k < 48; k++)
       				{        
                                          if (SegmentFont[Cptrfont+(47-k)*rows+i] >> (7-j) & 1)
                                        {  
             				  send_data_int(Fcolor);
                                          if (DimFont == 64)
                                            send_data_int(Fcolor);   
                                        }  
         				 else
                                         {     
             				  send_data_int(Bcolor);
                                          if (DimFont == 64)
                                           send_data_int(Bcolor); 
                                         } 
       				}
                             
                          
                       }     
                    }
                }
  
          set_CS(HIGH); 
}	
else	
if(DimFont == 8)
{
     Cptrfont = (C-32)*8;
 
     TFT_Set_Address(x,y,x+7,y+7);

     set_CS(LOW); 
     send_cmd(0x2c);                     // # Write_memory_start

     for(i = 0; i <= 7; i++)
     {
       for(k = 0; k <= 7; k++)
       {
           if ((FONT_8x8[Cptrfont+7-k] >> (7-i) ) & 1)
             send_data_int(Fcolor);
         else
            send_data_int(Bcolor);
       }  
     }  
     set_CS(HIGH); 
}

else if(DimFont == 16)
{
     PtrFont = &FONT_16x16[(C-32)*32];
 
     for(k = 0; k <= 15; k++)
     {
      font16x16[15-k] = (*PtrFont++) << 8;
      font16x16[15-k] = font16x16[15-k] + *PtrFont++;
     }
		
     TFT_Set_Address(x,y,x+15,y+15);
    // digitalWrite(CSX, LOW); 
     set_CS(LOW); 
     send_cmd(0x2c);                   // # Write_memory_start

     for(i = 0; i <= 15; i++)						// left/right
      {		
       for(k = 0; k <= 15; k++)
       {

        if((font16x16[k] >> (15-i)) & 1)   //góra/dól
           send_data_int(Fcolor);
        else
           send_data_int(Bcolor);



       }
     }
 
    set_CS(HIGH); 
}

 
  set_CS(HIGH); 
  
}

void TFT_Text(char* S,unsigned int y,unsigned int x,unsigned int DimFont,unsigned int Fcolor,unsigned int Bcolor)
{

		 while (*S)
		 {
			
                       TFT_Char(*S,x,y,DimFont,Fcolor,Bcolor);
	
                                    y = y + DimFont;
                                           if ((DimFont == 64) && ((*S == ':') || (*S == '.')))	
						  y-=32;
                                           else
			 		   if (DimFont == 16)
							y-=2;
					   else
					   if ((DimFont == 32) && ((*S == ':') || (*S == '.')))	
						  y-=16;	
                                           else 
					   if (DimFont == 24)
							y-=8;
			 S++;
		 }
		
}

void Led29(int status)
{
#ifdef FAST_DATA 

    uint32_t regval=0;
    uint32_t phyaddr = 0;
// ----------------------------------------------------------------------------- 
// Led 29 set/reset
// -----------------------------------------------------------------------------    
    phyaddr = SUNXI_GPIO_BASE + (6*36)+0x10; // +0x10 -> data reg A
    regval = readl1(phyaddr);
    if (status == LOW)
      regval &= 0xFFFFFF7F;
    else
      regval |= 0x80;
    writel1(regval, phyaddr);
      
#else
        digitalWrite(BLINK_LED, status);     //blink, port 29   
#endif
}

void TFT_Dot(unsigned int x,unsigned int y,unsigned int color)
{

  TFT_Set_Address(y,x,y,x);
  set_CS(LOW); 
  send_cmd(0x2c);                   // # Write_memory_start
  send_data_int(color);
  set_CS(HIGH);  
 
}
void TFT_Line(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2,unsigned int color)
{
int x,y,addx,addy,dx,dy;
long P;
unsigned int i;

  dx = x2-x1;  //fabs(x2-x1);
  dy = y2-y1; //fabs(y2-y1);
  x = x1;
  y = y1;

   if(x1 > x2)
   {
       addx = -1;
   }
   else
   {
       addx = 1;
   }

   if(y1 > y2)
   {
      addy = -1;
   }
   else
   {
      addy = 1;
   }


 if(dx >= dy)
 {

  P = (2*dy) - dx;

   for(i = 1; i <= (dx +1); i++)
   {

     TFT_Dot(x,y,color);

     if(P < 0)
     {
         P = P + (2*dy);
         x = (x + addx);
     }
     else
     {
        P = P+(2*dy) - (2*dx);
        x = x + addx;
        y = y + addy;
     }
    }
  }
  else
  {
    P = (2*dx) - dy;

    for(i = 1; i <= (dy +1); i++)
    {

     TFT_Dot(x,y,color);

     if(P<0)
     {
       P = P + (2*dx);
       y = y + addy;
     }
     else
     {
        P = P + (2*dx) - (2*dy);
        x = x + addx;
        y = y + addy;
     }
    }
   }
}

void TFT_Circle(unsigned int x,unsigned int y,char radius,char fill,unsigned int color)
{
int a_,b_,P;
 a_ = 0;
 b_ = radius;
 P = 1 - radius;
 while (a_ <= b_)
 {
    if(fill == 1)
    {
         TFT_Box(x-a_,y-b_,x+a_,y+b_,color);
         TFT_Box(x-b_,y-a_,x+b_,y+a_,color);
    }
    else
    {
         TFT_Dot(a_+x, b_+y, color);
         TFT_Dot(b_+x, a_+y, color);
         TFT_Dot(x-a_, b_+y, color);
         TFT_Dot(x-b_, a_+y, color);
         TFT_Dot(b_+x, y-a_, color);
         TFT_Dot(a_+x, y-b_, color);
         TFT_Dot(x-a_, y-b_, color);
         TFT_Dot(x-b_, y-a_, color);
    }
    if (P < 0 )
    {
        P = (P + 3) + (2* a_);
        a_ ++;
    }
    else
    {
        P = (P + 5) + (2* (a_ - b_));
        a_ ++;
        b_ --;
    }
  }
}

float get_temp_cpu()
{
 
 char bufT[256];     // Data from device
 char tmpDataT[6];   // Temp C * 1000 reported by device 
 ssize_t numRead;
 float tempP = 0;
 fd = open("/sys/class/thermal/thermal_zone1/temp", O_RDONLY);
  if(fd != -1)
  {    
     while((numRead = read(fd, bufT, 256)) > 0) 
      {
       tmpDataT[2]=0;   
       strncpy(tmpDataT, &bufT[0], 2); 
       tempP = strtof(tmpDataT, NULL);
      }    
  } 

   close(fd);
   return tempP;
 
}

#ifdef DS_1820
void get_DS1820_dir()
{
 DIR *dir;
 char path[] = "/sys/bus/w1/devices"; 
 struct dirent *dirent;
 char dev[16];      // Dev ID
 dir = opendir (path);
 if (dir != NULL)
 {
  while ((dirent = readdir (dir)))
   // 1-wire devices are links beginning with 28-
   if (dirent->d_type == DT_LNK && 
     strstr(dirent->d_name, "28-") != NULL) { 
    strcpy(dev, dirent->d_name);
    printf("\nDevice: %s\n", dev);
   }
        (void) closedir (dir);
        }
 else
 {
  perror ("Couldn't open the w1 devices directory");

 }

 // Assemble path to OneWire device
 sprintf(devPath, "%s/%s/w1_slave", path, dev);  

}



float get_temp_ds()
{
  ssize_t numRead;  
  float tempX=0;  
  char bufR[256];     // Data from device
  char tmpDataR[6];   // Temp C * 1000 reported by device 
  fd = open(devPath, O_RDONLY);
  if(fd != -1)
  {
  
    while((numRead = read(fd, bufR, 256)) > 0) 
    {
        strncpy(tmpDataR, strstr(bufR, "t=") + 2, 5); 
        tempX = strtof(tmpDataR, NULL);
    }
  }
  close(fd);   
  return tempX/1000;
}

#endif

int main (void)
{
    time_t t;
    struct tm tm;
    char buf[256];     // buffer
    int step =0;
    float tempC = 0;
  
    Init();
    InitLCD();

    
    TFT_Clear(Black);
    

    TFT_Line(20,300,460,300,Cyan);
    TFT_Circle(420,220,40,1,Yellow);   //with fill
 
#ifdef DS_1820
    get_DS1820_dir();   //if connected DS1820 (pin 37 on 40 pin connector)
#endif    

    for(;;)
    {
      
        
        delay(500);      
        Led29(HIGH);

          sprintf(buf,"TFT ILI9488 Test");
          TFT_Text(buf,165,25,8,Yellow,Black);
          sprintf(buf,"%04d",step);
          TFT_Text(buf,360,10,24,Cyan,Black);
       
          TFT_Text("Temp ext:",20,240,16,Green,Black);  
          TFT_Text("Temp cpu:",220,240,16,Red,Black);  
          t = time(NULL);
          tm = *localtime(&t); 
          sprintf(buf,"%02d:%02d.%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
      
#ifdef DS_1820
           if ((step & 3) == 1)    //read DS temp every 4 steps (1s *4 = 4s)
            {    
             tempC= get_temp_ds();
             if (tempC != 0)        //temperature is valid
              {   
               sprintf(buf,"%.1f*C", tempC);
               TFT_Text(buf,20,180,32,Green,Black);  
              }    
            }  
#endif 
          
          
            if ((step & 3) == 0)    //read CPU temp every 4 steps (1s *4 = 4s)
            {    
            tempC= get_temp_cpu();
             if (tempC != 0)        //temperature is valid
              {   
               sprintf(buf,"%.0f*C", tempC);
               TFT_Text(buf,220,180,32,Red,Black);        //print temperature on LCD
              }    
            }  
          
                 
      delay(500);
      Led29(LOW);               //blink 
      
          t = time(NULL);
          tm = *localtime(&t);
          sprintf(buf,"%d-%d-%d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
          TFT_Text(buf,140,320-16,16,White,Black);
          sprintf(buf,"%02d:%02d.%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
          TFT_Text(buf,10,60,64,Blue,Black);  
          
          
       step++;   

    }
  
  
    return 0;      
}          

