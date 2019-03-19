/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC32MX MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC32MX MCUs - pic32mx : v1.35
        Device            :  PIC32MX150F128B
        Driver Version    :  2.00
    The generated drivers are tested against the following:
        Compiler          :  XC32 1.42
        MPLAB             :  MPLAB X 3.55
*/

/*
    (c) 2016 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

#include "mcc_generated_files/mcc.h"
#include <stdio.h>
#include <math.h>

volatile bool sec_flag = false;

const uint16_t max_i2c_retry = 3;
const uint16_t mpu_id = 0x68;

I2C1_MESSAGE_STATUS i2c_status;
I2C1_TRANSACTION_REQUEST_BLOCK trb[2];
uint8_t regAddr;

#define buf_len 16
uint8_t buf[buf_len];

uint8_t MPU_ar = 0;

void I2C_Init()
{
    
    I2C1CONbits.SIDL = 0;               // Continue in Idle
    I2C1CONbits.A10M = 0;               // 7 bit addr
    I2C1CONbits.DISSLW = 1;             // Disable slew rate limiting
    I2C1CONbits.SMEN = 0;               // Disable SMBus input levels
    I2C1CONbits.ACKDT = 0;              // Send Ack (applies to master receive)
    I2C1CONbits.ACKEN = 0;              // Disable ACKEN
    
    I2C1CONbits.SEN = 0;                // Start condition enable = 0
    I2C1CONbits.RSEN = 0;               // Repeated start condition enable = 0
    I2C1CONbits.PEN = 0;                // Stop condition enable = 0
    I2C1CONbits.RCEN = 0;               // Receive enable, HW clr after 8th bit
    
    
    I2C1CONbits.ON = 1;                 // Turn on I2C1
    // Baud Rate 0x018B = 395; 40,000,000MHz => ~100KHz clock
    //           0x00C6 ~200KHz
    //           0x0064 ~400KHz
    I2C1BRG = 0x0064;
    
    // ACKEN disabled; STRICT disabled; STREN disabled; GCEN disabled; SMEN disabled; 
    // DISSLW disabled; I2CSIDL disabled; ACKDT Sends ACK; SCLREL Holds; 
    // RSEN disabled; A10M 7 Bit; PEN disabled; RCEN disabled; SEN disabled; ON enabled; 
    //I2C1CON = 0x8200;
    // BCL disabled; P disabled; S disabled; I2COV disabled; IWCOL No collision; 
    I2C1STAT = 0x0;
}

void MPU_Init()
{
    I2C_Init();
}

// TX REG => I2C1TRN
// RX RED => I2C1RSR -> I2C1RCV; RBF high if data ready in I2C1RSV

void inline I2C_Start(void)
{
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN) continue;
}

void inline I2C_Stop(void)
{
    I2C1CONbits.PEN = 1;
    while (I2C1CONbits.PEN) continue;
}

void inline I2C_ReStart(void)
{
    I2C1CONbits.RSEN = 1;
    while (I2C1CONbits.RSEN) continue;
}

bool inline I2C_Tx(uint8_t x)
{
    const uint32_t timeout = 20 * 1000; // 1000us or 1ms
    I2C1TRN = x;
    uint32_t start = _CP0_GET_COUNT();
    while (I2C1STATbits.TRSTAT) {
        if ((_CP0_GET_COUNT() - start) > timeout) return false;
    };
    return I2C1STATbits.ACKSTAT;
}

uint8_t inline I2C_Rx(bool lastRead) // a is true if last byte
{
    I2C1CONbits.RCEN;  // Enable Read
    I2C1CONbits.ACKDT = lastRead;   // true (1) is NACK, false (0) is ACK)
    while(I2C1CONbits.RCEN); // Wait till cleared (8 bits received)
    I2C1CONbits.ACKEN = 1;   // Send (N)ACK
    return I2C1RCV;
}

bool inline I2C_IsIdle(void)
{
    if (I2C1STATbits.TRSTAT) return false;
    return  ((I2C1CON & 0x001F) == 0x0000);
}

bool I2C_WaitIdle(void)
{
    const uint32_t timeout = 20 * 1000; // 1000us or 1ms
    uint32_t start = _CP0_GET_COUNT();
    while((_CP0_GET_COUNT() - start) < timeout) {
        if (I2C_IsIdle()) break;
    }
}

inline void DelayUs(uint16_t us)
{
    // Max delay = ~200 seconds
    uint32_t wait = us * 20; // FOSC = 40 MHz, Core Timer clock = FOSC/2
    uint32_t start = _CP0_GET_COUNT();
    while((_CP0_GET_COUNT() - start) < wait);
}

bool MPU_Read_x(uint8_t adr, uint8_t *buf, uint8_t len)
{
    if (~I2C_WaitIdle()) return false;
    I2C_Start();
    if (!I2C_Tx((mpu_id<<1) & 0xFE)) {
        I2C_Stop();
        return false;  
    }
    if (!I2C_Tx(adr)) {
        I2C_Stop();
        return false;
    }
    
    I2C_ReStart();
    if (!I2C_Tx((mpu_id<<1) | 0x01 )) {
        I2C_Stop();
        return false;  
    }
    int i;
    for(i=0;i<len;i++) {
        buf[i] = I2C_Rx(i == (len-1));
    }
    I2C_Stop();
    return true;
}

bool MPU_Write_x(uint8_t *buf, uint8_t len)
{
    if (~I2C_WaitIdle()) return false;
    I2C_Start();
    if (!I2C_Tx((mpu_id<<1) & 0xFE)) {
        I2C_Stop();
        return false;  
    }
    int i;
    for(i=0;i<len;i++) {
        if (!I2C_Tx(buf[i])) {
            I2C_Stop();
            return false;  
        }
    }
    I2C_Stop();
    return true;
}

    //uint32_t wait = us * 20; // FOSC = 40 MHz, Core Timer clock = FOSC/2
    //uint32_t start = _CP0_GET_COUNT();
    //while((_CP0_GET_COUNT() - start) < wait);

bool MPU_Read(uint8_t adr, uint8_t *buf, uint8_t len)
{
    i2c_status = I2C1_MESSAGE_PENDING;
    regAddr = adr;

    I2C1_MasterWriteTRBBuild(&trb[0], &regAddr, 1, mpu_id);
    I2C1_MasterReadTRBBuild(&trb[1], buf, len, mpu_id);
   
    I2C1_MasterTRBInsert(2, trb, &i2c_status);
    if (i2c_status == I2C1_MESSAGE_FAIL) {
        printf("Fail Insert W\n");
        return false;
    }
    
    while(i2c_status == I2C1_MESSAGE_PENDING) {
        DelayUs(100);        
    }
    
    if (i2c_status == I2C1_MESSAGE_COMPLETE) {  
        //printf("Read %02x = %02x\n", regAddr, buf[0]);
        return true;
    } 

    printf("Failed %d", i2c_status);
    return false;
}

bool MPU_Write(uint8_t *buf, uint8_t len)
{
    printf("writing %02x to %02x len %d\n", buf[1], buf[0], len);
    i2c_status = I2C1_MESSAGE_PENDING;

    I2C1_MasterWriteTRBBuild(&trb[0], buf, len, mpu_id);
    
    I2C1_MasterTRBInsert(1, &trb[0], &i2c_status);
    if (i2c_status == I2C1_MESSAGE_FAIL) {
        printf("Fail Insert W\n");
        return false;
    }
    
    while(i2c_status == I2C1_MESSAGE_PENDING) {
        DelayUs(100);        
        printf("pending\n");
    }
    
    if (i2c_status == I2C1_MESSAGE_COMPLETE) {  
        printf("OK\n");
        return true;
    } 

    printf("Failed %d", i2c_status);
    return false;

}

bool MPU_Wake(uint8_t c)
{
    buf[0] = 0x6B;
    buf[1] = c;  
    MPU_Write(buf, 2);  
    return true;
}

uint8_t MPU_SetAccel(uint8_t i)
{
    // Range can be +/- 2,4,8,16 g -> correspond to 0x00, 0x08, 0x10, 0x18
    if (i > 3) return 0xFF;
    const uint8_t ar[] = {0x00, 0x08, 0x10, 0x18};
    buf[0] = 0x1C;
    buf[1] = ar[i];   
    MPU_Write(buf, 2); 
    MPU_Read(0x1C, buf, 1);
    MPU_ar = (buf[0]>>3) & 0x03;
    return MPU_ar;
}

uint8_t MPU_SetGyro(uint8_t i)
{
    // Range can be +/- 250,500,1000,2000 deg/sec -> correspond to 0x00, 0x08, 0x10, 0x18
    if (i > 3) return 0xFF;
    const uint8_t gr[] = {0x00, 0x08, 0x10, 0x18};
    buf[0] = 0x1B;
    buf[1] = gr[i];   
    MPU_Write(buf, 2); 
    MPU_Read(0x1B, buf, 1);
    return (buf[0]>>3) & 0x03;
}

float MPU_Angle(void) //Based on x and z accel
{
    MPU_Read(0x3B, buf, 6);
    uint16_t ax_r = (buf[0]*256+buf[1]);
    //uint16_t ay_r = (buf[2]*256+buf[3]);
    uint16_t az_r = (buf[4]*256+buf[5]);
    int16_t ax = (int16_t)ax_r;
    //int16_t ay = (int16_t)ay_r;
    int16_t az = (int16_t)az_r;
    //printf("x,y,z = %d,%d,%d\n", ax, ay, az);
    const float pi = 3.1415;
    const float rad2deg = 180.0 / pi;
    float result = atan2f((ax + 0.0), (az + 0.0)) * rad2deg;
    return result;
}



bool MPU_Init_old(void)
{
    // Wake with internal osc
    printf("Wake\n");
    MPU_Wake(0x00);
    
    printf("Read ID\n");
    // Check Chip ID
    MPU_Read(0x75, buf, 1);
    printf("ID = 0x%02x\n", buf[0]);
    
    // Wake with Gyro 
    MPU_Wake(0x01);
    
    uint8_t a = MPU_SetAccel(0x01);
    printf("Acceleration = 0x%02x\n", a);
    
    uint8_t g = MPU_SetGyro(0x00);
    printf("Gyro = 0x%02x\n", g);
    
    MPU_Read(0x1B, buf, 1);
    printf("1B = 0x%02x\n", buf[0]);
    
    MPU_Read(0x1C, buf, 1);
    printf("1C = 0x%02x\n", buf[0]);

    return true;
}


/*
                         Main application
 */
int main(void)
{
    __XC_UART = 1;
    // initialize the device
    SYSTEM_Initialize();
    TMR1_Start();
    
    printf("Startup Delay ... \n");
    sec_flag = false;
    while (!sec_flag) continue;
    sec_flag = false;
    while (!sec_flag) continue;
    sec_flag = false;
    
    printf("Initialize MPU \n");
    MPU_Init_old();
    
    printf("Calibrate MPU \n");
    int i;
    float angleCal = 0.0;
    for(i=0; i<100; i++) {
        DelayUs(1000);
        angleCal += MPU_Angle();
    }
    angleCal = angleCal/100.0;
    printf("Angle Offset: %f\n", angleCal);
    
    while (1)
    {
        if (sec_flag) {
            sec_flag = false;
            IO_RB4_Toggle();
            float angle = MPU_Angle() - angleCal;
            printf("Angle: %f\n", angle);
        }
        
    }

    return -1;
}
/**
 End of File
*/