
/**
  OC1 Generated Driver API Source File

  @Company
    Microchip Technology Inc.

  @File Name
    oc1.c

  @Summary
    This is the generated source file for the OC1 driver using PIC32MX MCUs

  @Description
    This source file provides APIs for driver for OC1.
    Generation Information :
        Product Revision  :  PIC32MX MCUs - pic32mx : v1.35
        Device            :  PIC32MX150F128B
        Driver Version    :  0.5
    The generated drivers are tested against the following:
        Compiler          :  XC32 1.42
        MPLAB 	          :  MPLAB X 3.55
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

/**
  Section: Included Files
*/
#include <xc.h>
#include "oc1.h"

/**
  Section: Driver Interface
*/

void OC1_Initialize (void)
{
    // OC1RS 0;     
    OC1RS = 0x0;
    
    // OC1R 0;     
    OC1R = 0x0;
    
    // OC32 16-bit Mode; OCM PWM mode fault disabled; SIDL disabled; OCTSEL TMR2; ON enabled;     
    OC1CON = 0x8006;
	
}

void OC1_Tasks( void )
{
    if(IFS0bits.OC1IF)
    {
        IFS0CLR= 1 << _IFS0_OC1IF_POSITION;
    }
}

void OC1_Start( void )
{
    OC1CONbits.ON = 1;
}

void OC1_Stop( void )
{
    OC1CONbits.ON = 0;
}

void OC1_SingleCompareValueSet( uint16_t value )
{
    OC1R = value;
}

void OC1_DualCompareValueSet( uint16_t priVal, uint16_t secVal )
{
    OC1R = priVal;
	
    OC1RS = secVal;
}

void OC1_PWMPulseWidthSet( uint16_t value )
{
    OC1RS = value;
}

bool OC1_PWMFaultStatusGet()
{ 
    /* Return the status of the fault condition */
    return(OC1CONbits.OCFLT);
}

/**
 End of File
*/
