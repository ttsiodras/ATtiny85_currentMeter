/**
 * Attempting to measure current with a DIY shunt resistor,
 * and voltage amplification via an LM358 opamp.
 *
 *                                     +--------+
 *        (PCINT5/~RESET/ADC0/dW) PB5  | 1    8 | VCC
 * (PCINT3/XTAL1/CLKI/~OC1B/ADC3) PB3  | 2    7 | PB2 (SCK/USCK/SCL/ADC1/T0/INT0/PCINT2)
 *  (PCINT4/XTAL2/CLKO/OC1B/ADC2) PB4  | 3    6 | PB1 (MISO/DO/AIN1/OC0B/OC1A/PCINT1)
 *                                GND  | 4    5 | PB0 (MOSI/DI/SDA/AIN0/OC0A/~OC1A/AREF/PCINT0)
 *                                     +--------+
 * Connections:
 *
 * 1. TP4056 will be in charge of charging and protecting the LiPo battery.
 *
 * 2. The battery output goes to VCC, and we will be using VCC as
 *    the reference voltage. We use 1.1V internal reference trickery
 *    (see getVcc below) to make proper ADC measurements regardless
 *    of whatever drop in battery voltage.
 *
 * 3. ADC will be using 10bit resolution, using ADC2 for input (PB4).
 *    For more information, see table 17.5 "ADC Prescaler Selections" in
 *    chapter 17.13.2 "ADCSRA – ADC Control and Status Register A"
 *    (pages 140 and 141 on the complete ATtiny25/45/85 datasheet,
 *     Rev. 2586M–AVR–07/10).
 *    The 10-bit resolution is set via ADLAR set to 0 to disable
 *    left-shifting the result (bits ADC9 + ADC8 are in ADC[H/L] and
 *    bits ADC7..ADC0 are in ADC[H/L]).
 *    We therefore need a uint16_t variable to read ADC (raw_adc)
 *
 * 4. PB4 will be connected to the output of the LM358 opamp (pin 7),
 *    with the actual analog input from the shunt resistor connected to
 *    the opamp's non-inverting input (pin 5), and the two resistors
 *    Rf (10K) and R1 (1K) connected as follows:
 *    - R1 is between inverting input (pin 6) and GND.
 *    - Rf is between output (pin 7) and inverting input (pin 6).
 *
 * 5. A SW send-only serial is placed on PB3 (setup in globals.h)
 *    and set to 115200 baud (see 'setup' below)
 *
 * 6. Finally, the I2C OLED display is connected to the SCL/PB2 and
 *    SDA/PB1 pins (i.e. pins 7 and 6).
 */

//////////////////////////////
// Average the noise out, Luke
//////////////////////////////
#define NO_SAMPLES 10

///////////////
// OLED related
///////////////

#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Initially I was using SDA and SCL pins; at some point I moved
// the SDA to PB1 since I realized that (a) HW I2C doesn't work
// in the ATtiny85, and (b) I wanted to use the EXTERNAL reference
// for the voltage measurements. Turned out I didn't have to,
// since I used the 1.1V internal reference trick (see getVcc).
// ...but the choice of PB2 and PB1 (for SCL and SDA) remained.
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(PB2, PB1, PB0);

////////////////////////////////
// We will sleep in the end
// ...and be woken up via reset.
////////////////////////////////
#include <avr/sleep.h>

#include "globals.h"

////////////////////////////////////////////
// Emiter class automates information output
////////////////////////////////////////////
#include "Emiter.h"

////////////////////////////////////////////////
// The trickery with the 1.1V internal reference
////////////////////////////////////////////////
int readVcc()
{
    // Read 1.1V reference against AVcc.
    // Set the reference to Vcc and the measurement to internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || \
    defined(__AVR_ATmega1280__) || \
    defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || \
      defined(__AVR_ATtiny44__)  || \
      defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || \
      defined(__AVR_ATtiny45__)  || \
      defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
#else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

    delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Start conversion
    while (bit_is_set(ADCSRA,ADSC)); // measuring

    uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
    uint8_t high = ADCH; // unlocks both

    long result = (high<<8) | low;  // Can this return 0? I hope not.
    // result = 1125300L / result; // Calculate Vcc (in mV);
    //                             // 1125300 = 1.1*1023*1000
    result = 981496 / result; // Calculate Vcc (in mV); actual values
                              // for my ATtiny85 after calibrating
                              // measurements.
    return int(result); // Vcc in millivolts
}

int printVcc()
{
    int vcc = readVcc();
    int voltage1 = vcc / 1000;
    int voltage2 = vcc % 1000; // (((float)vcc / 1000) - voltage1) * 1000;

    Emiter line;
    line.printString("VCC      ");
    line.printInt(voltage1);
    line.printChar('.');
    line.printInt(voltage2, false /* unsigned */, true /* padded */, 3 /* padwidth */);
    line.printChar('V');
    return vcc;
}

void setup()
{
    mySerial.begin(115200);

    // Setup our screen and font (Go Alan Sugar, go!)
    u8x8.begin();
    u8x8.setFont(u8x8_font_amstrad_cpc_extended_u);

    // Wake up screen...
    u8x8.setPowerSave(false);
    // ...but prepare the CPU for a deep sleep (when sleep_cpu is called)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void prepareNormalAdc()
{
    // ...and indeed we do, setting it up like so:
    ADCSRA =
        (0 << ADEN)  |     // Disable ADC, to set the AREF as input
        (1 << ADPS2) |     // set prescaler to 64, bit 2
        (1 << ADPS1) |     // set prescaler to 64, bit 1
        (0 << ADPS0);      // set prescaler to 64, bit 0

    ADMUX =
        (0 << ADLAR) |     // do not left shift result (for 10-bit values)
        (0 << REFS2) |     // Sets ref. voltage to Vcc, bit 2
        (0 << REFS1) |     // Sets ref. voltage to Vcc, bit 1
        (0 << REFS0) |     // Sets ref. voltage to Vcc, bit 0
        (0 << MUX3)  |     // use ADC2 for input (PB4), MUX bit 3
        (0 << MUX2)  |     // use ADC2 for input (PB4), MUX bit 2
        (1 << MUX1)  |     // use ADC2 for input (PB4), MUX bit 1
        (0 << MUX0);       // use ADC2 for input (PB4), MUX bit 0

    ADCSRA =
        (1 << ADEN)  |     // Enable ADC
        (1 << ADPS2) |     // set prescaler to 64, bit 2
        (1 << ADPS1) |     // set prescaler to 64, bit 1
        (0 << ADPS0);      // set prescaler to 64, bit 0
}

void loop()
{
    // ADC variables
    uint8_t adc_lobyte; // low byte of the ADC register (ADCL)
    uint16_t raw_adc;

    // Collect the average value over NO_SAMPLES
    uint8_t samples;
    float v_ADC_total = 0.0, lower_bound, upper_bound;
    int i, currentAvg;
    static int cnt = 0;

    prepareNormalAdc();
    for(samples=0; samples<NO_SAMPLES; samples++) {
        ADCSRA |= (1 << ADSC);         // start ADC measurement
        while (ADCSRA & (1 << ADSC) ); // wait till conversion complete

        // 10-bit resolution:
        adc_lobyte = ADCL;               // get the low 8-bits of
                                         // the sample value from ADCL
        raw_adc = ADCH<<8 | adc_lobyte;  // ...and then get the rest.

        // Average over NO_SAMPLES
        float current = ((float)raw_adc)/1024.0;
        v_ADC_total += current;
    }

    Emiter::lineNo = 0; // Reset screen placement.
    int vcc = printVcc();

    // vcc is in millivolts, ditto for v_ADC_average then:
    float v_ADC_average_f = float(vcc)*v_ADC_total/float(samples);
    int v_ADC_average = int(v_ADC_average_f + 0.5); // in millivolt
    int v1 = v_ADC_average / 1000;
    int v2 = v_ADC_average % 1000;

    {
        Emiter line;
        line.printString("V_SHUNT  ");
        line.printInt(v1);
        line.printChar('.');
        line.printInt(v2, false /* unsigned */, true /* padded */, 3 /* padwidth */);
        line.printChar('V');
    }

    // Henceforth, in Volt:
    v_ADC_average_f /= 1000.0;

    // I used the faulty INA's shunt resistor, and got the following sample
    // measurements with an input of 5V, after the Op-amp's (in theory)
    // linear 11x boost ( R_feedback was 10K, R_1 was 1K -  with the feedback
    // resistor connected between the output of the Op-amp and the negative
    // input, and R_1 connected between negative input and ground):
    //
    //    +---------------+------------------+-----------------+
    //    | Load resistor | Measured Voltage | Load Current    |
    //    | (in Ohm)      | (in Volt)        | Amp (in theory) |
    //    |---------------+------------------+-----------------|
    //    |           250 |            0.011 |            0.02 |
    //    |           100 |            0.056 |            0.05 |
    //    |            10 |            0.570 |             0.5 |
    //    |             5 |            1.201 |               1 |
    //    |         3.333 |            1.942 |             1.5 |
    //    +---------------+------------------+-----------------+
    //
    // Assuming linear behavior from the Op-amp, the unknown shunt
    // resistor should impact the expected measured voltage as per this:
    //
    //                          R_shunt
    //        Vout = Vin * -----------------
    //                      R_shunt + R_load
    //
    //                Vout * R_load
    // =>  R_shunt =  -------------
    //                  Vin - Vout
    //
    // This gives the following sequence of values for R_shunt,
    // multiplied by the Op-amp's scale factor:
    //
    //    0.551213
    //    1.13269
    //    1.28668
    //    1.58068
    //    2.11683
    //
    // What does this tell us?
    //
    // Simple: That reality is NEVER simple :-)
    // Our LM358 Op-amp is non-linear, or our ADC is non-linear. Or both :-)
    //
    // We should therefore abandon theoretical models, like all good
    // engineers do - and adapt to reality...
    const static struct {
        float volt;
        float amp;
    } scale[] = {
        { 0.0,    0.0  },
        { 0.011,  0.02 },
        { 0.056,  0.05 },
        { 0.57,   0.5  },
        { 1.201,  1,   },
        { 1.942,  1.5  }
    };
    // This should be a Bezier curve - but I'll keep it nice and simple
    // (i.e. linear). For now at least :-)
    const int no = sizeof(scale)/sizeof(scale[0]) - 1;
    for(i=0; i<no; i++) {
        lower_bound = scale[i].volt;
        upper_bound = scale[i+1].volt;
        // If we get a measurement above the largest sample,
        // then use the slant of the last step. Otherwise
        // pick the matching measurement "window" from our samples above.
        if ((v_ADC_average_f >= lower_bound &&
             v_ADC_average_f <= upper_bound) || (i == no-1))
        {
            currentAvg = 1000.0*(
                scale[i].amp +
                (scale[i+1].amp - scale[i].amp) *
                     (v_ADC_average_f - lower_bound) /
                        (upper_bound - lower_bound));
            break;
        }
    }
    v1 = currentAvg / 1000;
    v2 = currentAvg % 1000;

    {
        Emiter line;
        line.printString("CURRENT  ");
        line.printInt(v1);
        line.printChar('.');
        line.printInt(v2, false /* unsigned */, true /* padded */, 3 /* padwidth */);
        line.printChar('A');
    }

    {
        Emiter line;
        line.printString("SAMPLES     ");
        line.printInt(cnt);
        line.printString("/5");
    }

    delay(250);
    if (cnt++ == 5) {
        int n = 9;
        int sleepTenTimes = 10;

        cnt = 0; // Reset for next time

        // Go to sleep - and wake up after 8x10 ~= 80 seconds.
        u8x8.setPowerSave(true); // Lights off, OLED
        while(sleepTenTimes--) {
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            // Enable the watchdog timer, set at 8 seconds
            WDTCR = 1<<WDIE | (n & 0x8)<<2 | 1<<WDE | (n & 0x7);
            sleep_enable();
            sleep_cpu();
            // After waking up, repeat this sleeping loop 10 times;
            // in total, sleep around 80 seconds
        }
        // Re-enable the screen, go do normal operatios
        u8x8.setPowerSave(false);
    }
}

ISR(WDT_vect) {
    // When we wake up, disable watchdog timer
    WDTCR = 1<<WDCE | 1<<WDE;
    WDTCR = 0;
}
