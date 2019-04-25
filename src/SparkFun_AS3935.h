#ifndef _SPARKFUN_AS3935_H_
#define _SPARKFUN_AS3935_H_

#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>

enum SF_AS3935_REGISTER_NAMES {

	AFE_GAIN          = 0x00, 
  THRESHOLD,
  LIGHTNING_REG,
  INT_MASK_ANT,
  ENERGY_LIGHT_LSB,
  ENERGY_LIGHT_MSB,
  ENERGY_LIGHT_MMSB,
  DISTANCE,
  FREQ_DISP_IRQ,
  CALIB_TRCO        = 0x3A, 
  CALIB_SRCO        = 0x3B,
  DEFAULT_RESET     = 0x3C,
  CALIB_RCO         = 0x3D 

};

// Masks for various registers, there are some redundant values that I kept 
// for the sake of clarity.
enum SF_AS3935_REGSTER_MASKS { 

  GAIN_MASK         = 0xF,
  SPIKE_MASK        = 0xF,
  DISTANCE_MASK     = 0xC0,
  INT_MASK          = 0xF0, 
  ENERGY_MASK       = 0xF0, 
  FLOOR_MASK        = 0x07,
  OSC_MASK          = 0xE0,
  CAP_MASK          = 0xF, 
  SPI_READ_M        = 0x40,
  CALIB_MASK        = 0x7F,
  DIV_MASK          = 0x3F

};

typedef enum SF_AS3935_I2C_ADDRESS {

 AS3935_DEFAULT_ADDRESS = 0x03, // Default ADD0 and ADD1 are HIGH
 AS3935_ADDRESS_ADD1_H  = 0x02, // ADD1 HIGH, ADD0 LOW
 AS3935_ADDRESS_ADD0_H  = 0x01, // ADD1 LOW, ADD0 HIGH
 AS3935_ADDRESS_LOW     = 0x00  // BOTH LOW 

} i2cAddress;

typedef enum INTERRUPT_STATUS {

  NOISE_TO_HIGH     = 0x01,
  DISTURBER_DETECT  = 0x04,
  LIGHTNING         = 0x08

} lightningStatus;  

#define INDOOR  0x12
#define OUTDOOR 0xE
#define DIRECT_COMMAND 0x96

class SparkFun_AS3935
{
  public: 
    // Constructor to be used with SPI
    SparkFun_AS3935();
    // Constructor to be used with I-squared-C. 
    SparkFun_AS3935(i2cAddress address);
    // I-squared-C Begin
    bool begin(TwoWire &wirePort = Wire);
    // SPI begin 
    bool beginSPI(uint8_t user_CSPin, uint32_t spiPortSpeed, SPIClass &spiPort = SPI); 
    // REG0x00, bit[0], manufacturer default: 0. 
    // The product consumes 1-2uA while powered down. If the board is powered down 
    // the the TRCO will need to be recalibrated: REG0x08[5] = 1, wait 2 ms, REG0x08[5] = 0.
    // SPI and I-squared-C remain active when the chip is powered down. 
    void powerDown();
    // REG0x3A bit[7].
    // This register holds the state of the timer RC oscillator (TRCO),
    // after it has been calibrated. The TRCO will need to be recalibrated
    // after power down. The following function wakes the IC, sends the "Direct Command" to 
    // CALIB_RCO register REG0x3D, waits 2ms and then checks that it has been successfully
    // calibrated. Note that I-squared-C and SPI are active during power down. 
    bool wakeUp();
    // REG0x00, bits [5:1], manufacturer default: 10010 (INDOOR). 
    // This funciton changes toggles the chip's settings for Indoors and Outdoors. 
    void setIndoorOutdoor(uint8_t _setting);
    // REG0x01, bits[3:0], manufacturer default: 0010 (2). 
    // This setting determines the threshold for events that trigger the 
    // IRQ Pin.  
    void watchdogThreshold(uint8_t _sensitivity);
    // REG0x01, bits [6:4], manufacturer default: 010 (2).
    // The noise floor level is compared to a known reference voltage. If this
    // level is exceeded the chip will issue an interrupt to the IRQ pin,
    // broadcasting that it can not operate properly due to noise (INT_NH).
    // Check datasheet for specific noise level tolerances when setting this register. 
    void setNoiseLevel(uint8_t _floor);
    // REG0x02, bits [3:0], manufacturer default: 0010 (2).
    // This setting, like the watchdog threshold, can help determine between false
    // events and actual lightning. The shape of the spike is analyzed during the
    // chip's signal validation routine. Increasing this value increases robustness
    // at the cost of sensitivity to distant events. 
    void spikeRejection(uint8_t _spSensitivity);
    // REG0x02, bits [5:4], manufacturer default: 0 (single lightning strike).
    // The number of lightning events before IRQ is set high. 15 minutes is The 
    // window of time before the number of detected lightning events is reset. 
    // The number of lightning strikes can be set to 1,5,9, or 16. 
    void lightningThreshold(uint8_t _strikes);
    // REG0x02, bit [6], manufacturer default: 1. 
    // This register clears the number of lightning strikes that has been read in
    // the last 15 minute block. 
    void clearStatistics(bool _clearStat);
    // REG0x03, bits [3:0], manufacturer default: 0. 
    // When there is an event that exceeds the watchdog threshold, the register is written
    // with the type of event. This consists of two messages: INT_D (disturber detected) and 
    // INT_L (Lightning detected). A third interrupt INT_NH (noise level too HIGH) 
    // indicates that the noise level has been exceeded and will persist until the
    // noise has ended. Events are active HIGH. There is a one second window of time to
    // read the interrupt register after lightning is detected, and 1.5 after
    // disturber.  
    uint8_t readInterruptReg();
    // REG0x03, bit [5], manufacturere default: 0.
    // This setting will change whether or not disturbers trigger the IRQ Pin. 
    void maskDisturber(bool _state);
    // REG0x03, bit [7:6], manufacturer default: 0 (16 division ratio). 
    // The antenna is designed to resonate at 500kHz and so can be tuned with the
    // following setting. The accuracy of the antenna must be within 3.5 percent of
    // that value for proper signal validation and distance estimation.
    void changeDivRatio(uint8_t _divisionRatio);
    // REG0x03, bit [7:6], manufacturer default: 0 (16 division ratio). 
    // This function returns the current division ratio of the resonance frequency.
    // The antenna resonance frequency should be within 3.5 percent of 500kHz, and
    // so when modifying the resonance frequency with the internal capacitors
    // (tuneCap()) it's important to keep in mind that the displayed frequency on
    // the IRQ pin is divided by this number. 
    uint8_t readDivisionRatio();
    // REG0x07, bit [5:0], manufacturer default: 0. 
    // This register holds the distance to the front of the storm and not the
    // distance to a lightning strike.  
    uint8_t distanceToStorm();
    // REG0x08, bits [5,6,7], manufacturer default: 0. 
    // This will send the frequency of the oscillators to the IRQ pin. 
    //  _osc 1, bit[5] = TRCO - Timer RCO Oscillators 1.1MHz
    //  _osc 2, bit[6] = SRCO - System RCO at 32.768kHz
    //  _osc 3, bit[7] = LCO - Frequency of the Antenna
    void displayOscillator(bool _state, int _osc);
    // REG0x08, bits [3:0], manufacturer default: 0. 
    // This setting will add capacitance to the series RLC antenna on the product.
    // It's possible to add 0-120pF in steps of 8pF to the antenna. 
    void tuneCap(uint8_t _farad);
    // LSB =  REG0x04, bits[7:0]
    // MSB =  REG0x05, bits[7:0]
    // MMSB = REG0x06, bits[4:0]
    // This returns a 20 bit value that is the 'energy' of the lightning strike.
    // According to the datasheet this is only a pure value that doesn't have any
    // physical meaning. 
    uint32_t lightningEnergy();
  
  private:

    uint32_t _pureLight = 0; // Variable for lightning energy which is just a pure number.  
    uint32_t _tempPE = 0; // Temp variable for lightning energy. 
    uint32_t _spiPortSpeed; // Given sport speed. 
    uint8_t _cs; // Chip select pin
    uint8_t _regValue; // Variable for returned register data. 
    uint8_t _spiWrite = 0; // Variable used for SPI write commands. 
    uint8_t _i2cWrite = 0; // Variable used for SPI write commands. 
    // Address variable. 
    i2cAddress _address; 
    // This function handles all I2C write commands. It takes the register to write
    // to, then will mask the part of the register that coincides with the
    // setting, and then write the given bits to the register at the given
    // start position. 
    void writeRegister(uint8_t _reg, uint8_t _mask, uint8_t _bits, uint8_t _startPosition);
    // This function reads the given register. 
    uint8_t readRegister(uint8_t _reg, int _len);
    
    // I-squared-C and SPI Classes
    TwoWire *_i2cPort; 
    SPIClass *_spiPort; 

};
#endif

