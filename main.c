#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "rc5.h"

#define RED 0
#define GREEN 1
#define BLUE 2

#define RC5_ADDR 0

#define CMD_STDBY 12
#define CMD_CHUP 32
#define CMD_CHDN 33
#define CMD_MUTE 13
#define CMD_VOLUP 16
#define CMD_VOLDN 17


static bool on = true;
static uint8_t cci = 0;
static uint8_t hue[] = { 15, 0, 7 };
static uint8_t pins[] = { RED_PIN, GREEN_PIN, BLUE_PIN };


void change_color(bool next) {
    if (!on) return;

    if (next) {
        cci++;
        cci %= 3;
    } else {
        if (cci) cci--;
        else cci = 2;
    }

    DDRB &= ~_BV(RED_PIN) & ~_BV(GREEN_PIN) & ~_BV(BLUE_PIN);
    DDRB |= _BV(pins[cci]);
    _delay_ms(100);
}

void change_intensity(bool up) {
    if (!on) return;

    if (up) {
        if (hue[cci] < 15) hue[cci]++;
    } else if (hue[cci] > 0) {
        hue[cci]--;
    }
}

void main(void) __attribute__ ((noreturn));
void main() {
    static uint8_t pwm_ctr = 0;
    static uint16_t prevcmd, cmd;

    RC5_Init();

    sei();

    for (;;) {
        cli();
        if (!on) {
            DDRB &= ~_BV(RED_PIN) & ~_BV(GREEN_PIN) & ~_BV(BLUE_PIN);
            GIMSK |= _BV(INT0);
            set_sleep_mode(SLEEP_MODE_IDLE);
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        } else {
            sei();

            for (uint8_t i=0; i<3; i++) {
                if (hue[i] > pwm_ctr) DDRB |= _BV(pins[i]);
                else DDRB &= ~_BV(pins[i]);
            }

            pwm_ctr++;
            pwm_ctr %= 16;
        }

        if (RC5_NewCommandReceived(&cmd)) {
            RC5_Reset();
            if (prevcmd == cmd) continue; // pretty much a toggle bit handler
            prevcmd = cmd;
            if (RC5_GetStartBits(cmd) != 3) continue;
            if (RC5_GetAddressBits(cmd) == RC5_ADDR) {
                switch (RC5_GetCommandBits(cmd)) {
                    case CMD_STDBY:
                        on ^= true;
                        break;
                    case CMD_CHDN:
                        change_color(false);
                        break;
                    case CMD_CHUP:
                        change_color(true);
                        break;
                    case CMD_MUTE:
                        DDRB |= _BV(RED_PIN) | _BV(GREEN_PIN) | _BV(BLUE_PIN);
                        _delay_ms(100);
                        DDRB &= ~_BV(RED_PIN) & ~_BV(GREEN_PIN) & ~_BV(BLUE_PIN);
                        _delay_ms(100);
                        break;
                    case CMD_VOLDN:
                        change_intensity(false);
                        break;
                    case CMD_VOLUP:
                        change_intensity(true);
                        break;
                }
            }
        }
    }
}
