#include "mbed.h"
#include "mRotaryEncoder.h"


mRotaryEncoder::mRotaryEncoder(PinName pinA, PinName pinB, PinName pinSW, PinMode pullMode, int debounceTime_us, int detectRise, int detectFall) {
    m_pinA = new PinDetect(pinA);                      // interrrupts on pinA
    m_pinB = new DigitalIn(pinB);                      // only digitalIn for pinB

    //set pins with internal PullUP-default
    m_pinA->mode(pullMode);
    m_pinB->mode(pullMode);

    // attach interrrupts on pinA
    if (detectRise != 0){
        m_pinA->attach_asserted(callback(this, &mRotaryEncoder::rise));
    }
    if (detectFall != 0){
        m_pinA->attach_deasserted(callback(this, &mRotaryEncoder::fall));
    }
    
    //start sampling pinA
    m_pinA->setSampleFrequency(debounceTime_us);                  // Start timers an Defaults debounce time.

    // Switch on pinSW
    m_pinSW = new PinDetect(pinSW);                 // interrupt on press switch
    m_pinSW->mode(pullMode);
    
    m_pinSW->setSampleFrequency(debounceTime_us);                  // Start timers an Defaults debounce time.


    m_position = 0;

    m_debounceTime_us = debounceTime_us;
}

mRotaryEncoder::~mRotaryEncoder() {
    delete m_pinA;
    delete m_pinB;
    delete m_pinSW;
}

int mRotaryEncoder::Get(void) {
    return m_position;
}



void mRotaryEncoder::Set(int value) {
    m_position = value;
}


void mRotaryEncoder::fall(void) {
    // debouncing does PinDetect for us
    //pinA still low?
    if (*m_pinA == 0) {
        if (*m_pinB == 1) {
            m_position++;
            if (rotCWIsr) {
                rotCWIsr();
            }
        } else {
            m_position--;
            if (rotCWIsr){
                rotCCWIsr();
            }
        }
        if (rotIsr){
            rotIsr();                        // call the isr for rotation
        }
    }
}

void mRotaryEncoder::rise(void) {
    //PinDetect does debouncing
    //pinA still high?
    if (*m_pinA == 1) {
        if (*m_pinB == 1) {
            m_position--;
            if (rotCCWIsr){
                rotCCWIsr();
            }
        } else {
            m_position++;
            if (rotCWIsr){
                rotCWIsr();
            }
        }
        if (rotIsr){
            rotIsr();                        // call the isr for rotation
        }
    }
}

