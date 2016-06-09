#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<math.h>
#include "i2c_master_noint.h"
// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // no boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // free up secondary osc pins
#pragma config FPBDIV = DIV_1 // divide CPU freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1048576  // slowest wdt
#pragma config WINDIS = OFF // no wdt window
#pragma config FWDTEN = OFF // wdt off by default
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the CPU clock to 48MHz
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_20 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = DIV_2 // divider for the 8MHz input clock, then multiply by 12 to get 48MHz for USB
#pragma config UPLLEN = ON // USB clock on

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module

// define constants and variables

#define Pi 3.1415926
#define SineCount 100
#define TriangleCount 200
#define CS LATBbits.LATB7 

static volatile float SineWave[100]; 
static volatile float TriangleWave[TriangleCount]; 

char read  = 0x00;
unsigned char checkGP7 = 0x00;
short A[14];
short tem, gx, gy, gz, ax, ay, az;
float ax2, ay2;

void init_OC();
void init_Timer2();
void MakeSineWave();
void maketrianglewave();
void InitSPI1();
void initI2C();
void initI2C2();

char SPI1_IO(char write);

void setVoltage(char channel, int voltage);
void setExpander(int pin, int level);
char getExpander();
unsigned char setLowBitOperation(int pin);
void I2C_Read_Multiple();




void __ISR(_TIMER_2_VECTOR, IPL5SOFT) PWMcontroller(void){
    
    ax2=1500.0*ax/32768.0+1500.0;
    ay2=1500.0*ay/32768.0+1500.0;
    OC1RS=(short)ax2;
    OC2RS=(short)ay2;
    IFS0bits.T2IF=0;

}

int main() {

   __builtin_disable_interrupts();
    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);
    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;
    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;
    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;  
    i2c_master_setup();
    initI2C2(); 
    initI2C();
    init_OC();
   
    // do your TRIS and LAT commands here    
    __builtin_enable_interrupts();  
     // set up USER pin as inputsd
    TRISBbits.TRISB4 = 1; // 0 for output, 1 for input
    // set up LED1 pin as a digital output   
    TRISAbits.TRISA4 = 0; // 0 for output, 1 for input

    
    
    while(1) {
        _CP0_SET_COUNT(0);
      
      /*  if(PORTBbits.RB4==0)
        {
            if(read == 0x69)
            LATAbits.LATA4=1;
            else
            LATAbits.LATA4=0;
        }
        else
        LATAbits.LATA4=0;  */
        while(_CP0_GET_COUNT() < 480000){
        
        ;
        }
        I2C_Read_Multiple();
       				 
     }            
    
}

//Library

void initI2C2(){
    i2c_master_setup();   
}

void initI2C(){
    i2c_master_start();
    i2c_master_send(0b11010110);    
    i2c_master_send(0x10);
    i2c_master_send(0x80);
    i2c_master_stop();
	
	i2c_master_start();
    i2c_master_send(0b11010110);    
    i2c_master_send(0x11);
    i2c_master_send(0x80);
    i2c_master_stop();
	
	i2c_master_start();
    i2c_master_send(0b11010110);    
    i2c_master_send(0x12);
    i2c_master_send(0x04);
    i2c_master_stop();
	
}

void init_OC(){
    
    RPB15Rbits.RPB15R=0b0101;
    RPB8Rbits.RPB8R=0b0101;
    T2CONbits.TCKPS=0b0100;
    PR2 = 2999;
    TMR2 = 0;
    OC1CONbits.OC32=0;
    OC1CONbits.OCTSEL=0;
    OC1CONbits.OCM=0b110;
    OC1RS = 1500;
    OC1R = 1500;
    OC2CONbits.OC32=0;
    OC2CONbits.OCTSEL=0;
    OC2CONbits.OCM=0b110;
    OC2RS = 1500; // initial PWM
    OC2R = 1500;  // change PWM later 
    //T2CONbits.TCKPS=0b100;
    T2CONbits.ON = 1;
    OC1CONbits.ON=1;
    OC2CONbits.ON=1;
    IPC2bits.T2IP = 5;
    IPC2bits.T2IS = 0;
    IFS0bits.T2IF = 0;
    IEC0bits.T2IE = 1;
}



void I2C_Read_Multiple(){
    i2c_master_start();
    i2c_master_send(0b11010110);
    i2c_master_send(0x20);
    i2c_master_restart();
    i2c_master_send(0b11010111);
    int i;
    for(i=0; i < 13; i++)
    {
        A[i] = i2c_master_recv();
        i2c_master_ack(0);   
    }
    A[13] = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    tem=(A[1]<<8)|(A[0]);
    gx=(A[3]<<8)|(A[2]);
    gy=(A[5]<<8)|(A[4]);
    gz=(A[7]<<8)|(A[6]);
    ax=(A[9]<<8)|(A[8]);
    ay=(A[11]<<8)|(A[10]);
    az=(A[13]<<8)|(A[12]);
} 







