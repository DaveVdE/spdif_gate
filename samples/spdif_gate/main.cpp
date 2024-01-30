/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "spdif_rx.h"

static constexpr uint8_t PIN_DCDC_PSM_CTRL = 23;
static constexpr uint8_t PIN_PICO_SPDIF_RX_DATA = 2;
static constexpr uint8_t PIN_PICO_SPDIF_ENABLE = 3;
static constexpr uint32_t TIMEOUT = 180;

volatile static bool stable_flg = false;
volatile static bool lost_stable_flg = false;

void measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_adc  = %dkHz\n", f_clk_adc);
    printf("clk_rtc  = %dkHz\n", f_clk_rtc);

    // Can't measure clk_ref / xosc as it is the ref
}

void on_stable_func(spdif_rx_samp_freq_t samp_freq)
{
    // callback function should be returned as quick as possible
    stable_flg = true;
}

void on_lost_stable_func()
{
    // callback function should be returned as quick as possible
    lost_stable_flg = true;
}

void enable_output() {
    gpio_put(PIN_PICO_SPDIF_ENABLE, 1);
}

void disable_output() {
    gpio_put(PIN_PICO_SPDIF_ENABLE, 0);
}

void print_binary(uint32_t frame) {
    printf("0b");
    printf("%d", (frame >> 31) & 1);
    printf("%d", (frame >> 30) & 1);
    printf("%d", (frame >> 29) & 1);
    printf("%d", (frame >> 28) & 1);
    printf("%d", (frame >> 27) & 1);
    printf("%d", (frame >> 26) & 1);
    printf("%d", (frame >> 25) & 1);
    printf("%d", (frame >> 24) & 1);
    printf("%d", (frame >> 23) & 1);
    printf("%d", (frame >> 22) & 1);
    printf("%d", (frame >> 21) & 1);
    printf("%d", (frame >> 20) & 1);
    printf("%d", (frame >> 19) & 1);
    printf("%d", (frame >> 18) & 1);
    printf("%d", (frame >> 17) & 1);
    printf("%d", (frame >> 16) & 1);
    printf("%d", (frame >> 15) & 1);
    printf("%d", (frame >> 14) & 1);
    printf("%d", (frame >> 13) & 1);
    printf("%d", (frame >> 12) & 1);
    printf("%d", (frame >> 11) & 1);
    printf("%d", (frame >> 10) & 1);
    printf("%d", (frame >> 9) & 1);
    printf("%d", (frame >> 8) & 1);
    printf("%d", (frame >> 7) & 1);
    printf("%d", (frame >> 6) & 1);
    printf("%d", (frame >> 5) & 1);
    printf("%d", (frame >> 4) & 1);
    printf("%d", (frame >> 3) & 1);
    printf("%d", (frame >> 2) & 1);
    printf("%d", (frame >> 1) & 1);
    printf("%d", frame & 1);
    printf("\n");
}

int main()
{
    stdio_init_all();

    // DCDC PSM control
    // 0: PFM mode (best efficiency)
    // 1: PWM mode (improved ripple)
    gpio_init(PIN_DCDC_PSM_CTRL);
    gpio_set_dir(PIN_DCDC_PSM_CTRL, GPIO_OUT);
    gpio_put(PIN_DCDC_PSM_CTRL, 1); // PWM mode for less Audio noise

    // ENABLE control
    // 0: Output is not enabled
    // 1: Output is enabled
    gpio_init(PIN_PICO_SPDIF_ENABLE);
    gpio_set_dir(PIN_PICO_SPDIF_ENABLE, GPIO_OUT);
    disable_output();

    spdif_rx_config_t config = {
        .data_pin = PIN_PICO_SPDIF_RX_DATA,
        .pio_sm = 0,
        .dma_channel0 = 0,
        .dma_channel1 = 1,
        .alarm = 0,
        .flags = SPDIF_RX_FLAGS_ALL
    };

    spdif_rx_start(&config);
    spdif_rx_set_callback_on_stable(on_stable_func);
    spdif_rx_set_callback_on_lost_stable(on_lost_stable_func);

    printf("Starting...\n");

    int count = 0;
    int countdown = 0;

    while (true) {
        if (stable_flg) {
            stable_flg = false;
            printf("stable\n");
        }
        if (lost_stable_flg) {
            lost_stable_flg = false;
            printf("waiting\n");
        }
        if (count % 100 == 0) {            
            if (spdif_rx_get_state() == SPDIF_RX_STATE_STABLE) {
                //auto fifo_count = spdif_rx_get_fifo_count();
                //printf("Fifo count = %d\n", fifo_count);
                uint32_t* buffer;
                auto read = spdif_rx_read_fifo(&buffer, SPDIF_RX_FIFO_SIZE);
                //print_binary(buffer[0]);
                uint32_t first_frame = buffer[0] & 0x0ffffff0;
                if (first_frame & 0x0ffffff0) {
                    countdown = TIMEOUT;
                }
            }

            printf("Countdown: %d\n", countdown);

            if (countdown > 0) {
                printf("enabled\n");
                enable_output();
                --countdown;
            }
            else
            {
                printf("disabled\n");
                disable_output();
            }
        }
        
        tight_loop_contents();
        sleep_ms(10);
        count++;
    }

    return 0;
}
