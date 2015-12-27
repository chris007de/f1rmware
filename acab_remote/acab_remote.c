/* acab_remote
 *
 * Copyright muCCC 2015
 * andz, chris007
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <r0ketlib/config.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/pins.h>

#include <libopencm3/lpc43xx/adc.h>
#include <libopencm3/lpc43xx/uart.h>

#include "../rflib/rflib_m0.h"

#include "acab_remote.h"

#include <string.h>

// default to 2396 MHz
#define FREQSTART 2396000000

 #define UID_TIMEOUT 10000000

/**************************************************************************/

// Wrapper for Tx Method, switching back to Rx after Tx automatically
void transmit(uint8_t *data, uint16_t len) {
    rflib_bfsk_transmit(data, len, true);
}

//# MENU acab_remote
void acab_remote() {

    // Only needed when started from menu (and not as app)
    //getInputWaitRelease();

    // Get some randomness
    SETUPadc(LED4);
    srand(adc_get_single(ADC0,ADC_CR_CH6)*2*330/1023);

    cpu_clock_set(204);

    rflib_init();
    rflib_bfsk_init();
    rflib_set_freq(FREQSTART);
    rflib_bfsk_receive();

    uart_init(UART0_NUM, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE, 111, 0, 1);

    uint8_t txBuf[2];
    // Generate UID for this session and store in txBuf[1]
    // 0 is reserved for unset UID
    txBuf[0] = (rand() % 255) + 1; // 1-255
    lcdPrint("UID: ");
    lcdPrintln(IntToStr(txBuf[0],2,F_HEX));
    rflib_lcdDisplay();

    bool button_action = false;

    bool running = true;
    while(running) {

        uint8_t btn = getInputRaw();
        if (BTN_NONE != btn)
        {
            if(!button_action)
            {
                txBuf[1] = btn;
                transmit(txBuf, 2);
                button_action = true;
            }
        }
        else
        {
            button_action = false;
        }

        // Poll for BFSK packets
        static uint8_t myUid = 0;
        static int64_t uidTimer = 0;
        uint8_t rx_pkg[256];
        int rx_pkg_len = rflib_bfsk_get_packet(rx_pkg, 255);
        if(rx_pkg_len > 0) {
            uint8_t uid           = rx_pkg[0];
            uint8_t direction     = rx_pkg[1];

            // Learn UID if no other is set
            if(0 == myUid)
            {
                myUid = uid;

                lcdPrint("Learned new UID ");
                lcdPrintln(IntToStr(myUid,2,F_HEX));
                rflib_lcdDisplay();
            }
            // Check for currently active UID
            if(uid == myUid)
            {
                // Write to UART
                uart_write(UART0_NUM, direction);
                uart_write(UART0_NUM, '\n');
                uart_write(UART0_NUM, '\r');

                // Reset UID Timeout
                uidTimer = 0;

                // Debug Output
                lcdPrint(IntToStr(uid,2,F_HEX));
                lcdPrint(" ");
                lcdPrintln(IntToStr(direction,2,F_HEX));
                rflib_lcdDisplay(); 
            }           
        }

        // Increment Uid Timer and check for Timeout
        uidTimer++;
        if((uidTimer > UID_TIMEOUT)
            && (0 != myUid))
        {            
            myUid = 0;

            lcdPrintln("UID Timed Out");
            rflib_lcdDisplay();
        }
        //if(uart_rx_data_ready(UART0_NUM) == UART_RX_DATA_READY) {
        //    uint8_t data = uart_read(UART0_NUM);
        //    serial_handler(data);
        //}
    }

    rflib_shutdown();
    // \todo Clear LEDs
    OFF(EN_1V8);
    OFF(EN_VDD);
    return;
}
