/* 
 * File:   ADC.h
 * Author: saim
 *
 * Created on May 21, 2021, 6:47 PM
 */

#ifndef ADC_H
#define	ADC_H

#ifdef	__cplusplus
extern "C" {
#endif

    void readADCChannel(unsigned char channel)
    {
        // 0b 0101 -> 5th chanel 
        ADCON0bits.CHS0 =  channel & 0x1; // Select channel..
        ADCON0bits.CHS1 = (channel >> 1) & 0x1;
        ADCON0bits.CHS2 = (channel >> 2) & 0x1;
        ADCON0bits.CHS3 = (channel >> 3) & 0x1;
        
        
        ADCON0bits.ADON = 1;
        ADCON0bits.GODONE = 1; //Start convertion
        
        
    }

    
    void initADC()
    {
        ADCON1bits.PCFG3=1;  // AN0, AN2 are analog. AN2 = thermometer, AN0 = potentiometer.
        ADCON1bits.PCFG2=1;
        ADCON1bits.PCFG1=0;
        ADCON1bits.PCFG0=0;
        
        ADCON1bits.VCFG0 =0; //Vref+=5.0, Vref=0
        ADCON1bits.VCFG1=0;
        
        // RECIT COMMENTS ; For 8MHZ -> Tosc = 1/8 us
        // Tad options (2xTosc, 4xTosc, 8xTosc, 16xTosc, 32xTosc, 64xTosc)
        // min Tad 0.7 us - max Tad 25 us (Keep as short as possible)
        // The closest one to min Tad (8xTosc) hence Tad = 8xTosc = 1 us (ADCS2:ADCS0=001)
        // Acquisition time options (0xTad, 2xTad, 4xTad, 6xTad, 8xTad, 12xTad, 16xTad, 20xTad)
        // Min acquisition time = 2.4 us (the closest acqusition time we can set 4xTad = 4us) (ACQT2:ACQT0=010)
        
        ADCON2bits.ADCS2 = 0; // conversion clock : TAD = 1/40 us * 32 = 8/10 = 0.8 us >= 0.7 us . OK.
        ADCON2bits.ADCS1 = 1; // Fosc = 40, 40/32 = 1,25 frequency at which I convert each bit by a time TAD
        ADCON2bits.ADCS0 = 0;
        
        ADCON2bits.ACQT2 = 0; // acquisiton time : 0.8 us * 4 = 3.2 us >= 2.4 us. since programmed no need to employ a timer.
        ADCON2bits.ACQT1 = 1;
        ADCON2bits.ACQT0 = 0;
        
        ADCON2bits.ADFM = 1; // Right justified...
           
    }

    
#ifdef	__cplusplus
}
#endif

#endif	/* ADC_H */

