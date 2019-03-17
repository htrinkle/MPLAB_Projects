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

volatile bool sec_flag = false;

const uint16_t max_i2c_retry = 3;
const uint16_t mpu_id = 0x68;

I2C1_MESSAGE_STATUS i2c_status;
I2C1_TRANSACTION_REQUEST_BLOCK trb_W, trb_W1;
I2C1_TRANSACTION_REQUEST_BLOCK trb_R;
uint8_t regAddr;

#define buf_len 200
uint8_t buf[200];

void MPU_Init()
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
    // Baud Rate 0x18B = 395; 40,000,000MHz => ~100KHz clock
    I2C1BRG = 0x18B;
    
    // ACKEN disabled; STRICT disabled; STREN disabled; GCEN disabled; SMEN disabled; 
    // DISSLW disabled; I2CSIDL disabled; ACKDT Sends ACK; SCLREL Holds; 
    // RSEN disabled; A10M 7 Bit; PEN disabled; RCEN disabled; SEN disabled; ON enabled; 
    //I2C1CON = 0x8200;
    // BCL disabled; P disabled; S disabled; I2COV disabled; IWCOL No collision; 
    I2C1STAT = 0x0;
}

// TX REG => I2C1TRN
// RX RED => I2C1RSR -> I2C1RCV; RBF high if data ready in I2C1RSV

#define _MPU_Start I2C1CONbits.SEN = 1
#define _MPU_WaitStart while (I2C1CONbits.SEN)

#define _MPU_Stop I2C1CONbits.PEN = 1
#define _MPU_WaitStop while (I2C1CONbits.PEN)

// TRSTAT is transmit status; 1 if transmit in progress
// IWCOL 1 => write to I2CxTRN failed due to busy
// TBF 1 => Transmit Buffer Full (i.e. TX in progress)
#define _MPU_TX(x) I2C1TRN = x
#define _MPU_WaitTx while(I2C1STATbits.TRSTAT)

// ACKSTAT contains ack received from slave (1 = nack, 0 = ack)  HW clear after slave acknowledge
#define _MPU_WaitAck while(I2C1STATbits.ACKSTAT)

// BCL 1 => bus collision detected during master operation
// RBF 1=> receive buffer full

inline void DelayUs(uint16_t us)
{
    // Max delay = ~200 seconds
    uint32_t wait = us * 20; // FOSC = 40 MHz, Core Timer clock = FOSC/2
    uint32_t start = _CP0_GET_COUNT();
    while((_CP0_GET_COUNT() - start) < wait);
    //T4CONbits.ON = false;
    //TMR4 = 0;
    //PR4 = 5 * us;
    //IFS0bits.T4IF = false;
    //T4CONbits.ON = true; 
    //while(!IFS0bits.T4IF);
}

bool MPU_Read(uint8_t adr, uint8_t *buf, uint8_t len)
{
    i2c_status = I2C1_MESSAGE_PENDING;
    regAddr = adr;

    I2C1_MasterWriteTRBBuild(&trb_W, &regAddr, 1, mpu_id);
    I2C1_MasterReadTRBBuild(&trb_R, buf, len, mpu_id);
   
    I2C1_MasterTRBInsert(1, &trb_W, &i2c_status);
    if (i2c_status == I2C1_MESSAGE_FAIL) {
        printf("Fail Insert W\n");
        return false;
    }
    
    I2C1_MasterTRBInsert(1, &trb_R, &i2c_status);
    if (i2c_status == I2C1_MESSAGE_FAIL) {
        printf("Fail Insert R\n");
        return false;
    }
    
    while(i2c_status == I2C1_MESSAGE_PENDING) {
        DelayUs(100);        
        printf("pending\n");
    }
    
    if (i2c_status == I2C1_MESSAGE_COMPLETE) {  
        printf("Done\n");
        return true;
    } 

    printf("Failed %d", i2c_status);
    return false;
}

bool MPU_Write(uint8_t adr, uint8_t *buf, uint8_t len)
{
    i2c_status = I2C1_MESSAGE_PENDING;
    regAddr = adr;

    I2C1_MasterWriteTRBBuild(&trb_W, &regAddr, 1, mpu_id);
    I2C1_MasterWriteTRBBuild(&trb_W1, buf, len, mpu_id);
    
    I2C1_MasterTRBInsert(1, &trb_W, &i2c_status);
    if (i2c_status == I2C1_MESSAGE_FAIL) {
        printf("Fail Insert W\n");
        return false;
    }
    
    I2C1_MasterTRBInsert(1, &trb_W1, &i2c_status);
    if (i2c_status == I2C1_MESSAGE_FAIL) {
        printf("Fail Insert W1\n");
        return false;
    }
    
    while(i2c_status == I2C1_MESSAGE_PENDING) {
        DelayUs(100);        
        printf("pending\n");
    }
    
    if (i2c_status == I2C1_MESSAGE_COMPLETE) {  
        printf("Done\n");
        return true;
    } 

    printf("Failed %d", i2c_status);
    return false;

}

bool MPU_Flush(uint8_t *buf, uint8_t len)
{
    uint8_t i;

    i2c_status = I2C1_MESSAGE_PENDING;

    for(i=0; i<len; i++) buf[i] = 0x00;
    I2C1_MasterWriteTRBBuild(&trb_W, buf, len, mpu_id);
   
    I2C1_MasterTRBInsert(1, &trb_W, &i2c_status);
    if (i2c_status == I2C1_MESSAGE_FAIL) {
        printf("Fail Insert\n");
        return false;
    }

    while(i2c_status == I2C1_MESSAGE_PENDING) {
        DelayUs(1000);  
        printf("pending\n");
    }
    
    if (i2c_status == I2C1_MESSAGE_COMPLETE) {
        printf("Done\n");
        return true;
    } 

    printf("Failed %d", i2c_status);
    return false;
}

bool MPU_Wake(void)
{
    buf[0] = 0x01;
    MPU_Write(0x6B, buf, 1);  
    return true;
}

uint8_t MPU_SetAccel(uint8_t i)
{
    // Range can be +/- 2,4,8,16 g -> correspond to 0x00, 0x08, 0x10, 0x18
    if (i > 3) return 0xFF;
    const uint8_t ar[] = {0x00, 0x08, 0x10, 0x18};
    buf[0] = ar[i]; 
    MPU_Write(0x1C, buf, 1); 
    MPU_Read(0x1C, buf, 1);
    return (buf[0]>>3) & 0x03;
}

uint8_t MPU_SetGyro(uint8_t i)
{
    // Range can be +/- 250,500,1000,2000 deg/sec -> correspond to 0x00, 0x08, 0x10, 0x18
    if (i > 3) return 0xFF;
    const uint8_t gr[] = {0x00, 0x08, 0x10, 0x18};
    buf[0] = gr[i];  
    MPU_Write(0x1C, buf, 1); 
    MPU_Read(0x1C, buf, 1);
    return (buf[0]>>3) & 0x03;
}

bool MPU_Init_old(void)
{
    printf("Flush\n");
    MPU_Flush(buf, 107);
    printf("Read ID\n");
    MPU_Read(0x75, buf, 1);
    printf("ID = 0x%02x\n", buf[0]);
    MPU_Wake();
    printf("Acceleration = 0x%02x\n", MPU_SetAccel(1));
    printf("Gyro = 0x%02x\n", MPU_SetGyro(0));
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
    
    printf("hi\n");
    
    MPU_Init_old();
    
    while (1)
    {
        // Add your application code  
        if (sec_flag) {
            sec_flag = false;
            IO_RB4_Toggle();
            printf("Hello Hans\n");
        }
        
    }

    return -1;
}
/**
 End of File
*/