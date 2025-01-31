#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// SPI Protocol Configuration
#define SPI_MAX_BUFFER_SIZE 256
#define SPI_CLOCK_FREQUENCY 1000000 // 1 MHz
#define SPI_MODE 0 // CPOL = 0, CPHA = 0

// Configuration structure
typedef struct {
    uint8_t mosi_pin;
    uint8_t miso_pin;
    uint8_t sck_pin;
    uint8_t ss_pin;
} SPIConfig;

// Software SPI State Machine
typedef struct {
    SPIConfig config;
    uint8_t tx_buffer[SPI_MAX_BUFFER_SIZE];
    uint8_t rx_buffer[SPI_MAX_BUFFER_SIZE];
    uint16_t tx_length;
    uint16_t rx_length;
    uint16_t current_bit;
    bool transmission_complete;
} SPIContext;

// Function declarations
void set_pin_high(uint8_t pin);
void set_pin_low(uint8_t pin);
uint8_t read_pin(uint8_t pin);
void spi_init(SPIContext* ctx, SPIConfig config);
void spi_transmit_bit(SPIContext* ctx, bool bit);
bool spi_receive_bit(SPIContext* ctx);
uint8_t spi_transfer_byte(SPIContext* ctx, uint8_t tx_byte);
bool spi_transaction(SPIContext* ctx, uint8_t* tx_data, uint8_t* rx_data, uint16_t length);

#endif // SPI_H