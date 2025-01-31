#include "spi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define GPIO functions as weak so they can be overridden by tests
__attribute__((weak)) void set_pin_high(uint8_t pin) {
    // Default implementation
    printf("Pin %d set HIGH\n", pin);
}

__attribute__((weak)) void set_pin_low(uint8_t pin) {
    // Default implementation
    printf("Pin %d set LOW\n", pin);
}

__attribute__((weak)) uint8_t read_pin(uint8_t pin) {
    // Default implementation
    return 0;
}

// Bit Delay Simulation
static void spi_delay() {
    volatile uint32_t delay_count = SPI_CLOCK_FREQUENCY / 2;
    while(delay_count--) {
        __asm__ volatile("nop");
    }
}

void spi_init(SPIContext* ctx, SPIConfig config) {
    ctx->config = config;
    ctx->tx_length = 0;
    ctx->rx_length = 0;
    ctx->current_bit = 0;
    ctx->transmission_complete = false;

    set_pin_low(config.sck_pin);
    set_pin_high(config.ss_pin);
    set_pin_low(config.mosi_pin);
}

void spi_transmit_bit(SPIContext* ctx, bool bit) {
    set_pin_low(ctx->config.sck_pin);
    spi_delay();

    if (bit) {
        set_pin_high(ctx->config.mosi_pin);
    } else {
        set_pin_low(ctx->config.mosi_pin);
    }
    spi_delay();

    set_pin_high(ctx->config.sck_pin);
    spi_delay();
}

bool spi_receive_bit(SPIContext* ctx) {
    set_pin_low(ctx->config.sck_pin);
    spi_delay();

    set_pin_high(ctx->config.sck_pin);
    spi_delay();

    return read_pin(ctx->config.miso_pin) ? true : false;
}

uint8_t spi_transfer_byte(SPIContext* ctx, uint8_t tx_byte) {
    uint8_t rx_byte = 0;

    set_pin_low(ctx->config.ss_pin);

    for (int8_t bit = 7; bit >= 0; bit--) {
        spi_transmit_bit(ctx, (tx_byte & (1 << bit)) ? 1 : 0);
        if (spi_receive_bit(ctx)) {
            rx_byte |= (1 << bit);
        }
    }

    set_pin_high(ctx->config.ss_pin);

    return rx_byte;
}

bool spi_transaction(SPIContext* ctx, uint8_t* tx_data, uint8_t* rx_data, uint16_t length) {
    if (length > SPI_MAX_BUFFER_SIZE) return false;

    for (uint16_t i = 0; i < length; i++) {
        rx_data[i] = spi_transfer_byte(ctx, tx_data[i]);
    }

    return true;
}

// Function that creates a memory leak
uint8_t* create_test_data(size_t size) {
    uint8_t* data = (uint8_t*)malloc(size);
    // Memory leak: This data is never freed
    return data;
}

// Another function with memory leak
SPIContext* create_context() {
    SPIContext* ctx = (SPIContext*)malloc(sizeof(SPIContext));
    // Memory leak: Context is never freed
    return ctx;
}
#ifndef ENABLE_TESTS
 int main() {
    printf("SPI Program Starting...\n");

    // Create some intentional memory leaks
    uint8_t* test_buffer1 = create_test_data(1024);
    uint8_t* test_buffer2 = create_test_data(2048);

    // Configure SPI
    SPIConfig config = {
        .sck_pin = 1,
        .mosi_pin = 2,
        .miso_pin = 3,
        .ss_pin = 4
    };

    // Create context with memory leak
    SPIContext* ctx = create_context();
    spi_init(ctx, config);

    // Prepare test data
    uint8_t tx_data[] = {0x55, 0xAA, 0x12, 0x34};
    uint8_t* rx_data = (uint8_t*)malloc(sizeof(tx_data));  // Another memory leak

    printf("Performing SPI transaction...\n");
    
    // Perform SPI transaction
    if (spi_transaction(ctx, tx_data, rx_data, sizeof(tx_data))) {
        printf("Transaction successful!\n");
        printf("Transmitted data: ");
        for (size_t i = 0; i < sizeof(tx_data); i++) {
            printf("0x%02X ", tx_data[i]);
        }
        printf("\n");
        
        printf("Received data: ");
        for (size_t i = 0; i < sizeof(tx_data); i++) {
            printf("0x%02X ", rx_data[i]);
        }
        printf("\n");
    } else {
        printf("Transaction failed!\n");
    }

    // Allocate more memory that won't be freed
    char* temp_buffer = (char*)malloc(512);
    strcpy(temp_buffer, "This memory will never be freed");

    printf("Test complete!\n");
    
    // Note: Intentionally not freeing any allocated memory to create memory leaks
    return 0;
 }
#endif