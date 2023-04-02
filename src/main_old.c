#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "lib/7941w.h"


int main() {
    stdio_init_all();

    rfid_7941w_init(uart1);

    #if 0
    //rfid_7941w_alt_write_id_EM4305(uart1, 0x68, 5275669);
    rfid_7941w_alt_write_id_EM4305(uart1, 0xff, 0x11223344);
    //rfid_7941w_alt_write_id_S50(uart1, 2207165843);
    //rfid_7941w_alt_write_id_S50(uart1, 0xaabbccdd);
    sleep_ms(3000);
    #endif

    int counter = 0;
    while (true) {
        printf("Loop %d\n", counter++);
        
        uint64_t id = rfid_7941w_alt_read_id(uart1);

        if (id != 0)
        {
            printf("  Received id: 0x%010" PRIx64 " (%010" PRIu32 ")\n", id, (uint32_t)id);
            sleep_ms(500);
        }

        sleep_ms(100);
        sleep_ms(800);
    }
    return 0;
}
