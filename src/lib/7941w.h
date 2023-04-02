#include <stdbool.h>
#include "hardware/uart.h"

void rfid_7941w_init(uart_inst_t * uart);

void rfid_7941w_reset(uart_inst_t * uart);

void rfid_7941w_init_generic(uart_inst_t * uart, uint tx_pin, uint rx_pin);

// data is optional, can be NULL if length is 0
void rfid_7941w_send(uart_inst_t *uart, uint8_t address, uint8_t command, uint8_t length, uint8_t *data);

// data must have enough space for 255 bytes, returns true on success
bool rfid_7941w_recv(uart_inst_t *uart, uint8_t *address, uint8_t *command, uint8_t *length, uint8_t *data);

// read 125 kHz, eg T5577/EM4305
bool rfid_7941w_read_id_LF(uart_inst_t *uart, uint8_t *length, uint8_t *data);

// read 13.56 MHz, eg S50
bool rfid_7941w_read_id_HF(uart_inst_t *uart, uint8_t *length, uint8_t *data);

// try to read HF first then LF
bool rfid_7941w_read_id(uart_inst_t *uart, uint8_t *length, uint8_t *data);

// write 125 kHz, eg EM4305
bool rfid_7941w_write_id_LF(uart_inst_t *uart, uint8_t length, uint8_t *data);

// write 13.56 MHz, eg S50
bool rfid_7941w_write_id_HF(uart_inst_t *uart, uint8_t length, uint8_t *data);

typedef enum rfid_7941w_type {
  LF = 0b01,
  HF = 0b10,
  ERROR = -1,
} rfid_7941w_type_t;


// try to read HF first then LF, returns uint64
uint64_t rfid_7941w_alt_read_id(uart_inst_t *uart);

// try to read HF first then LF, returns uint64
uint64_t rfid_7941w_alt_read_id_with_info(uart_inst_t *uart, rfid_7941w_type_t *info);

// write 125 kHz, EM4305, 5 bytes
bool rfid_7941w_alt_write_id_EM4305(uart_inst_t *uart, uint8_t vendor, uint32_t id);

// write 13.56 MHz, S50, 4 bytes
bool rfid_7941w_alt_write_id_S50(uart_inst_t *uart, uint32_t id);