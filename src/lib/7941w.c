#include "7941w.h"

#include <string.h>
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

// 7941W communicates with these
#define rfid_7941W_UART_BAUD 115200
#define rfid_7941W_UART_DATA_BITS 8
#define rfid_7941W_UART_STOP_BITS 1
#define rfid_7941W_UART_PARITY UART_PARITY_NONE

// response time ms
#define rfid_7941W_RESPONSE_TIME_MS 20

void rfid_7941w_init(uart_inst_t *uart)
{
    if (uart==uart0)
    {
        rfid_7941w_init_generic(uart0, 0, 1);
    }
    else if (uart==uart1)
    {
        rfid_7941w_init_generic(uart1, 4, 5);
    }
    else //invalid hw uart
    {
        assert(false);
    }
}

void rfid_7941w_reset(uart_inst_t *uart)
{
    rfid_7941w_init(uart);
    uint8_t dummy[1];
    while (uart_is_readable(uart))
    {
        uart_read_blocking(uart, dummy, 1);
    }
}

void rfid_7941w_init_generic(uart_inst_t *uart, uint tx_pin, uint rx_pin)
{
    uart_init(uart, 2400);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    int __unused actual = uart_set_baudrate(uart, rfid_7941W_UART_BAUD);
    uart_set_hw_flow(uart, false, false);
    uart_set_format(uart, rfid_7941W_UART_DATA_BITS, rfid_7941W_UART_STOP_BITS, rfid_7941W_UART_PARITY);
}

uint8_t calc_xor(uint8_t length, uint8_t *data) 
{
    uint8_t ret = 0x0;
    uint8_t * tail = data + length;
    while (data<tail)
    {
        ret = ret ^ (*data);
        data++;
    }
    return ret;
}

bool uart_read_blocking_timout(uart_inst_t *uart, uint8_t *dst, size_t len)
{
    const uint32_t bytes_per_sec = rfid_7941W_UART_BAUD / (rfid_7941W_UART_DATA_BITS + rfid_7941W_UART_STOP_BITS + 1);
    const uint32_t wait_per_byte_us = ( 1000000 + bytes_per_sec ) / bytes_per_sec + 10; // ceiling + little extra

    size_t read = 0;
    do
    {
        if (! uart_is_readable_within_us(uart, wait_per_byte_us))
        {
            break;
        }
        uart_read_blocking(uart, (dst+read), 1);
        read++;
    } while (read < len);

    return read == len;
}

void rfid_7941w_send(uart_inst_t *uart, uint8_t address, uint8_t command, uint8_t length, uint8_t *data)
{
    //Protocol Header | Address | Command | Data Length | Data        | XOR Check
    //AB BA           | 1 Byte  | 1 Byte  | 1 Byte      | 0-255 Bytes | 1 Byte
    //XOR check: result of other bytes check except protocol header.

    uint8_t buffer[261]; 

    buffer[0] = 0xAB;
    buffer[1] = 0xBA;  
    buffer[2] = address; 
    buffer[3] = command;  
    buffer[4] = length;

    if (length!=0 && data!=NULL) 
    {
        memcpy((void *)(buffer+5), (void *)data, length);
    }
    buffer[5+length] = calc_xor(length+3, (buffer+2) );

    if (uart_is_writable(uart))
    {
        uart_write_blocking(uart, buffer, (6+length));
    }
    
}

bool rfid_7941w_recv(uart_inst_t *uart, uint8_t *address, uint8_t *command, uint8_t *length, uint8_t *data)
{
    //Protocol Header | Address | Command | Data Length | Data        | XOR Check
    //CD DC           | 1 Byte  | 1 Byte  | 1 Byte      | 0-255 Bytes | 1 Byte
    //XOR check: result of other bytes check except protocol header.
    
    uint8_t buffer[261]; 

    if (! uart_read_blocking_timout(uart, buffer, 5) )
    {
        return false;
    }

    if (buffer[0]==0xCD && buffer[1]==0xDC)
    {
        uint8_t length_local = buffer[4];

        if (address)
        {
            *address = buffer[2];
        }
        if (command)
        {
            *command = buffer[3];
        }
        if (length)
        {
            *length = length_local;
        }

        if (! uart_read_blocking_timout(uart, (buffer+5), length_local+1) )
        {
            return false;
        }

        uint8_t xor = calc_xor(length_local+3, (buffer+2) );
        
        if (buffer[5+length_local] == xor)
        {
            if (length_local!=0 && data!=NULL) 
            {
                memcpy((void *)data, (void *)(buffer+5), length_local);
            }
            return true;
        }
    }

    return false;
}

bool rfid_7941w_read_id_LF(uart_inst_t *uart, uint8_t *length, uint8_t *data)
{
    rfid_7941w_send(uart, 0x0, 0x15, 0, NULL);
    sleep_ms(rfid_7941W_RESPONSE_TIME_MS);
    uint8_t status = 0;
    bool success;
    success = rfid_7941w_recv(uart, NULL, &status, length, data);
    return success && status == 0x81;
}

bool rfid_7941w_read_id_HF(uart_inst_t *uart, uint8_t *length, uint8_t *data)
{
    rfid_7941w_send(uart, 0x0, 0x10, 0, NULL);
    sleep_ms(rfid_7941W_RESPONSE_TIME_MS);
    uint8_t status = 0;
    bool success;
    success = rfid_7941w_recv(uart, NULL, &status, length, data);
    return success && status == 0x81;
}

bool rfid_7941w_read_id(uart_inst_t *uart, uint8_t *length, uint8_t *data)
{
    bool success;
    success = rfid_7941w_read_id_HF(uart, length, data);
    if (!success)
    {
        success = rfid_7941w_read_id_LF(uart, length, data);
    }
    return success;
}

bool rfid_7941w_write_id_LF(uart_inst_t *uart, uint8_t length, uint8_t *data)
{
    //if (length!=5) return false;
    rfid_7941w_send(uart, 0x0, 0x16, length, data);
    sleep_ms(rfid_7941W_RESPONSE_TIME_MS);
    uint8_t status = 0;
    bool success;
    uint8_t b[255];
    uint8_t l = 0;
    success = rfid_7941w_recv(uart, NULL, &status, &l, b);
    return success && status == 0x81;
}


bool rfid_7941w_write_id_HF(uart_inst_t *uart, uint8_t length, uint8_t *data)
{
    //if (length!=4) return false;
    rfid_7941w_send(uart, 0x0, 0x11, length, data);
    sleep_ms(rfid_7941W_RESPONSE_TIME_MS);
    uint8_t status = 0;
    bool success;
    uint8_t b[255];
    uint8_t l = 0;
    success = rfid_7941w_recv(uart, NULL, &status, &l, b);
    return success && status == 0x81;
}

uint64_t rfid_7941w_alt_read_id(uart_inst_t *uart)
{
    return rfid_7941w_alt_read_id_with_info(uart, NULL);
}

uint64_t rfid_7941w_alt_read_id_with_info(uart_inst_t *uart, rfid_7941w_type_t *info)
{
    uint8_t len;
    uint8_t buff[255];
    uint64_t ret = 0;
    rfid_7941w_type_t type = ERROR;

    if (rfid_7941w_read_id_LF(uart, &len, buff))
    {
        type = LF;
    }
    else if (rfid_7941w_read_id_HF(uart, &len, buff))
    {
        type = HF;
    }

    if (type != ERROR)
    {
        const size_t size = sizeof(ret);
        uint8_t *ret_array = (uint8_t *)&ret;
        size_t i;
        for ( i=0; i<size && i<len; i++ )
        {
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            ret_array[i] = buff[len-1-i];
            #else
            ret_array[size-1-i] = buff[len-1-i];
            #endif
        }
    }

    if (info)
    {
        *info = type;
    }

    return ret;
}

bool rfid_7941w_alt_write_id_EM4305(uart_inst_t *uart, uint8_t vendor, uint32_t id)
{
    size_t i = 0;
    uint8_t len = 5;
    uint8_t buff[len];
    uint8_t *convert = (uint8_t *)&id;
    buff[i++] = vendor;
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    buff[i++] = convert[3];
    buff[i++] = convert[2];
    buff[i++] = convert[1];
    buff[i++] = convert[0];
    #else
    buff[i++] = convert[0];
    buff[i++] = convert[1];
    buff[i++] = convert[2];
    buff[i++] = convert[3];
    #endif
    return rfid_7941w_write_id_LF(uart, len, buff);
}

bool rfid_7941w_alt_write_id_S50(uart_inst_t *uart, uint32_t id)
{
    uint8_t len = 4;
    uint8_t buff[len];
    uint8_t *convert = (uint8_t *)&id;
    size_t i = 0;
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    buff[i++] = convert[3];
    buff[i++] = convert[2];
    buff[i++] = convert[1];
    buff[i++] = convert[0];
    #else
    buff[i++] = convert[0];
    buff[i++] = convert[1];
    buff[i++] = convert[2];
    buff[i++] = convert[3];
    #endif
    return rfid_7941w_write_id_HF(uart, len, buff);
}