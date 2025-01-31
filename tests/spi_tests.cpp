#define ENABLE_TESTS
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "CppUTest/CommandLineTestRunner.h" 
// Include the header instead of the source file
extern "C" {
    #include "../Inc/spi.h"
}

// Mock functions
static uint8_t mock_pin_states[256] = {0};
static uint8_t mock_pin_read_sequence[8] = {0};
static int current_read_index = 0;

// Implementation of our mock functions that will override the weak symbols in spi.c
extern "C" {
    void set_pin_high(uint8_t pin) {
        mock_pin_states[pin] = 1;
    }

    void set_pin_low(uint8_t pin) {
        mock_pin_states[pin] = 0;
    }

    uint8_t read_pin(uint8_t pin) {
        if (pin == 11) { // MISO pin
            uint8_t value = mock_pin_read_sequence[current_read_index];
            current_read_index = (current_read_index + 1) % 8;
            return value;
        }
        return mock_pin_states[pin];
    }
}

TEST_GROUP(SPITests) {
    SPIContext ctx;
    SPIConfig config;

    void setup() {
        // Initialize configuration with test pins
        config.mosi_pin = 10;
        config.miso_pin = 11;
        config.sck_pin = 12;
        config.ss_pin = 13;

        // Reset mock states
        memset(mock_pin_states, 0, sizeof(mock_pin_states));
        memset(mock_pin_read_sequence, 0, sizeof(mock_pin_read_sequence));
        current_read_index = 0;
        
        // Initialize SPI
        spi_init(&ctx, config);
    }

    void teardown() {
        // Reset all mock states
        memset(mock_pin_states, 0, sizeof(mock_pin_states));
        current_read_index = 0;
    }
};

TEST(SPITests, InitializationSetsCorrectPinStates) {
    // Verify initial pin states after initialization
    CHECK_EQUAL(0, mock_pin_states[config.mosi_pin]); // MOSI should be low
    CHECK_EQUAL(0, mock_pin_states[config.sck_pin]);  // SCK should be low
    CHECK_EQUAL(1, mock_pin_states[config.ss_pin]);   // SS should be high
}

TEST(SPITests, TransmitBitSetsMOSIAndTogglesClockCorrectly) {
    // Test transmitting a high bit
    spi_transmit_bit(&ctx, true);
    CHECK_EQUAL(1, mock_pin_states[config.mosi_pin]); // MOSI should be high
    CHECK_EQUAL(1, mock_pin_states[config.sck_pin]);  // SCK should end high
    
    // Test transmitting a low bit
    spi_transmit_bit(&ctx, false);
    CHECK_EQUAL(0, mock_pin_states[config.mosi_pin]); // MOSI should be low
    CHECK_EQUAL(1, mock_pin_states[config.sck_pin]);  // SCK should end high
}

TEST(SPITests, ReceiveBitReadsMISOAndTogglesClockCorrectly) {
    // Setup MISO sequence for two reads
    mock_pin_read_sequence[0] = 1;
    mock_pin_read_sequence[1] = 0;
    
    bool bit1 = spi_receive_bit(&ctx);
    bool bit2 = spi_receive_bit(&ctx);
    
    CHECK_TRUE(bit1);    // First bit should be high
    CHECK_FALSE(bit2);   // Second bit should be low
    CHECK_EQUAL(1, mock_pin_states[config.sck_pin]); // Clock should end high
}

TEST(SPITests, TransferByteHandlesFullByteCorrectly) {
    // Setup MISO sequence to receive 0b10101010
    for(int i = 0; i < 8; i++) {
        mock_pin_read_sequence[i] = (i % 2);  // Alternating 0s and 1s
    }
    
    uint8_t tx_byte = 0xA5; // 0b10100101
    uint8_t rx_byte = spi_transfer_byte(&ctx, tx_byte);
    
    CHECK_EQUAL(0xAA, rx_byte); // Should receive 0b10101010
    CHECK_EQUAL(1, mock_pin_states[config.ss_pin]); // SS should end high
}

TEST(SPITests, TransactionHandlesMultipleBytes) {
    // Setup MISO to return incrementing values
    for(int i = 0; i < 8; i++) {
        mock_pin_read_sequence[i] = 1;  // All 1s for simple test
    }
    
    uint8_t tx_data[] = {0x55, 0xAA, 0x33};
    uint8_t rx_data[3];
    
    bool result = spi_transaction(&ctx, tx_data, rx_data, 3);
    
    CHECK_TRUE(result);
    CHECK_EQUAL(0xFF, rx_data[0]); // Should receive all 1s
    CHECK_EQUAL(0xFF, rx_data[1]);
    CHECK_EQUAL(0xFF, rx_data[2]);
    CHECK_EQUAL(1, mock_pin_states[config.ss_pin]); // SS should end high
}

TEST(SPITests, TransactionHandlesMaxBufferSize) {
    uint8_t tx_data[SPI_MAX_BUFFER_SIZE + 1];
    uint8_t rx_data[SPI_MAX_BUFFER_SIZE + 1];
    
    // Attempt to transfer more than max buffer size
    bool result = spi_transaction(&ctx, tx_data, rx_data, SPI_MAX_BUFFER_SIZE + 1);
    
    CHECK_FALSE(result); // Should fail due to buffer overflow protection
}

TEST(SPITests, ClockPolarityAndPhase) {
    // Test SPI MODE 0 (CPOL=0, CPHA=0) timing
    spi_transmit_bit(&ctx, true);
    
    // Verify clock starts low
    CHECK_EQUAL(0, mock_pin_states[config.sck_pin]);
    
    // Data should be set before rising edge
    CHECK_EQUAL(1, mock_pin_states[config.mosi_pin]);
}

int main(int argc, char** argv) {
    return CommandLineTestRunner::RunAllTests(argc, argv); // Run all tests defined above
    }