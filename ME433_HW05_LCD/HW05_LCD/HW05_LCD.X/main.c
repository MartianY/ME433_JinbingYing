#include<xc.h>                      // processor SFR definitions
#include<sys/attribs.h>             // __ISR macro
#include<math.h>
#include "i2c_master_noint.h"       // I2C functions
#include "ILI9163C.h"               // TFTLCD functions

// DEVCFG0
#pragma config DEBUG = OFF          // no debugging
#pragma config JTAGEN = OFF         // no jtag
#pragma config ICESEL = ICS_PGx1    // use PGED1 and PGEC1
#pragma config PWP = OFF            // no write protect
#pragma config BWP = OFF            // no boot write protect
#pragma config CP = OFF             // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL       // use primary oscillator with pll
#pragma config FSOSCEN = OFF        // turn off secondary oscillator
#pragma config IESO = OFF           // no switching clocks
#pragma config POSCMOD = HS         // high speed crystal mode
#pragma config OSCIOFNC = OFF       // free up secondary osc pins
#pragma config FPBDIV = DIV_1       // divide CPU freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD       // do not enable clock switch
#pragma config WDTPS = PS1048576    // slowest wdt
#pragma config WINDIS = OFF         // no wdt window
#pragma config FWDTEN = OFF         // wdt off by default
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the CPU clock to 48MHz
#pragma config FPLLIDIV = DIV_2     // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24     // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2     // divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = DIV_2     // divider for the 8MHz input clock, then multiply by 12 to get 48MHz for USB
#pragma config UPLLEN = ON          // USB clock on

// DEVCFG3
#pragma config USERID = 0x1234      // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF       // allow multiple reconfigurations
#pragma config IOL1WAY = OFF        // allow multiple reconfigurations
#pragma config FUSBIDIO = ON        // USB pins controlled by USB module
#pragma config FVBUSONIO = ON       // USB BUSON controlled by USB module

#define SLAVE_ADDR 0x6B             // slave address 01101011
#define maxLength 14                // slave address 01101011

unsigned char whoAmI  = 0x00;       // check who am I register 
char stuff[maxLength];              // save 14 8 bit data
short gx = 0x0000;                  // gyroscope x
short gy = 0x0000;                  // gyroscope y
short gz = 0x0000;                  // gyroscope z
short ax = 0x0000;                  // accelerometer x
short ay = 0x0000;                  // accelerometer y
short az = 0x0000;                  // acceletometer z
short temperature = 0x0000;         // temperature 
char message[100];

//function prototype
void initIMU();
unsigned char getWhoAmI();
void I2C_read_multiple(char add, char reg, char * data, char length);
void printChar(unsigned char letter, int offsetX, int offsetY);
void printString(char * message, int startX, int startY);

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
    
    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;         //RA4 (PIN#12) for Green LED
    TRISBbits.TRISB4 = 1;         //RB4 (PIN#11) for pushbutton

    // init I2C
    i2c_master_setup(); 
    initIMU();
    // init SPI
    SPI1_init();
    LCD_init();
    LCD_clearScreen(WHITE);
    
    __builtin_enable_interrupts();
    //printChar('h',10,20);  
    // TFTLCD
    sprintf(message,"Hello world 1337!");
    printString(message,20,32);
    
    // check I2C communication
    if(getWhoAmI() == 0x69){
        LATAbits.LATA4 = 1;
    }
    
    while(1){
        _CP0_SET_COUNT(0);
        I2C_read_multiple(SLAVE_ADDR, 0x20, stuff, 14);
        temperature = ((stuff[1]<<8) | stuff[0]);
        gx = ((stuff[3]<<8) | stuff[2]);
        gy = ((stuff[5]<<8) | stuff[4]);
        gz = ((stuff[7]<<8) | stuff[6]);
        ax = ((stuff[9]<<8) | stuff[8]);
        ay = ((stuff[11]<<8) | stuff[10]);
        az = ((stuff[13]<<8) | stuff[12]);
        
        sprintf(message,"gx: %2.4f dps ",245*gx/32768.0);
        printString(message,12,42);
        sprintf(message,"gy: %2.4f dps ",245*gy/32768.0);
        printString(message,12,52);
        sprintf(message,"gz: %2.4f dps ",245*gz/32768.0);
        printString(message,12,62);
        sprintf(message,"ax: %2.4f g  ",2*ax/32768.0);
        printString(message,12,72);
        sprintf(message,"ay: %2.4f g  ",2*ay/32768.0);
        printString(message,12,82);
        sprintf(message,"az: %2.4f g  ",2*az/32768.0);
        printString(message,12,92);
        sprintf(message,"temp: %2.4f deg. C  ",25+(temperature/16.0));
        printString(message,12,102);
        while(_CP0_GET_COUNT() < 480000) { // 50 Hz
            ;
        }
    }
}

void printChar(unsigned char letter, int offsetX, int offsetY){
    int x=0;
    int y=0;
    
    for (x=0; x<5; x++) {
        for (y=7; y>-1; y--) {
            if (((ASCII[letter-0x20][x] >> (7-y)) & 1) == 1)
                LCD_drawPixel(offsetX+x, offsetY+(7-y), RED);
            else
                LCD_drawPixel(offsetX+x, offsetY+(7-y), WHITE);
        }
    }
}

void printString(char * message, int startX, int startY){
    int i = 0; 
    int xCoord = startX;
    int yCoord = startY;
    while(message[i]){ 
        printChar(message[i],xCoord,yCoord);
        xCoord = xCoord+5;
        i++;
    }
}

void initIMU(){  
    // init accelerometer
    i2c_master_start();
    i2c_master_send(0xD6);    
    i2c_master_send(0x10);
    i2c_master_send(0x80);
    i2c_master_stop();
    
    // init gyroscope
    i2c_master_start();
    i2c_master_send(0xD6);    
    i2c_master_send(0x11);
    i2c_master_send(0x80);
    i2c_master_stop();
    
    // init CTRL3_C IF_CON bit
    i2c_master_start();
    i2c_master_send(0xD6);    
    i2c_master_send(0x12);
    i2c_master_send(0b00000100);
    i2c_master_stop();
}

unsigned char getWhoAmI(){
    i2c_master_start();
    i2c_master_send(0xD6);    
    i2c_master_send(0x0F);
    i2c_master_restart();
    i2c_master_send(0xD7);
    whoAmI = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    
    return whoAmI;
}

void I2C_read_multiple(char add, char reg, char * data, char length){
    i2c_master_start();
    i2c_master_send(add << 1);    
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send((add << 1)|1);
    
    int i;
    for(i =0 ; i < (length-1); i++){
        data[i] = i2c_master_recv();
        i2c_master_ack(0);
    }
    
    data[length-1] = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
}