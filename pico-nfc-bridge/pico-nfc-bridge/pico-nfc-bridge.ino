/**
 * Pico NFC Bridge - Manual PN5180 driver with tag data reading
 * I2C slave (0x55) bridging ESP32 to PN5180
 *
 * Supports:
 * - MIFARE Classic 1K (Bambu Lab tags) with HKDF key derivation
 * - NTAG (SpoolEase/OpenPrintTag with NDEF)
 */

#include <SPI.h>
#include <Wire.h>
#include <SHA256.h>  // From Crypto library by Rhys Weatherley

#define PN5180_NSS   17
#define PN5180_BUSY  20
#define PN5180_RST   21

#define I2C_SDA      4
#define I2C_SCL      5
#define I2C_ADDR     0x55

// I2C Commands
#define CMD_GET_STATUS          0x00
#define CMD_GET_PRODUCT_VERSION 0x01
#define CMD_SCAN_TAG            0x10
#define CMD_READ_TAG_DATA       0x20  // New: Read tag blocks/pages

// Tag types (from SAK byte)
#define TAG_TYPE_UNKNOWN        0
#define TAG_TYPE_NTAG           1
#define TAG_TYPE_MIFARE_1K      2
#define TAG_TYPE_MIFARE_4K      3

// Response buffer - needs to be larger for tag data
volatile uint8_t respBuffer[200];
volatile uint8_t respLength = 0;
volatile uint8_t cmdBuffer[64];
volatile uint8_t cmdLength = 0;
volatile bool cmdReady = false;

// Tag state
uint8_t tagUid[10];
uint8_t tagUidLen = 0;
uint8_t tagSak = 0;
uint8_t tagType = TAG_TYPE_UNKNOWN;
bool tagPresent = false;
uint8_t lastStatus = 0;
uint8_t cachedVersion[2] = {0xFF, 0xFF};

// Reset management
uint8_t consecutiveFailures = 0;
const uint8_t MAX_FAILURES_BEFORE_RESET = 3;
uint32_t lastResetTime = 0;
const uint32_t RESET_COOLDOWN_MS = 500;

// Bambu Lab HKDF master key
const uint8_t BAMBU_MASTER_KEY[16] = {
    0x9a, 0x75, 0x9c, 0xf2, 0xc4, 0xf7, 0xca, 0xff,
    0x22, 0x2c, 0xb9, 0x76, 0x9b, 0x41, 0xbc, 0x96
};
const char BAMBU_CONTEXT[] = "RFID-A";

// Derived keys storage (16 sectors * 6 bytes = 96 bytes)
uint8_t bambuKeys[96];
bool keysGenerated = false;

// Tag data storage
uint8_t tagBlocks[5][16];  // Blocks 1, 2, 4, 5, 16 for Bambu
bool tagDataValid = false;

// Command processing flag - prevents background scan interference
volatile bool processingCommand = false;

// Current command sequence number for log correlation
uint8_t cmdSeq = 0;

// Helper to print sequence prefix for correlated logging
void logSeq(const char* msg) {
    Serial.print("[#");
    Serial.print(cmdSeq);
    Serial.print("] ");
    Serial.println(msg);
}

void logSeqStart(const char* msg) {
    Serial.print("[#");
    Serial.print(cmdSeq);
    Serial.print("] ");
    Serial.print(msg);
}

// Scan protection - after CMD_SCAN finds a tag, skip background scans briefly
uint32_t scanProtectionUntil = 0;

// ============================================================================
// HKDF Key Derivation for Bambu Lab tags
// ============================================================================

// HMAC-SHA256 helper using Crypto library
void hmac_sha256(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output) {
    SHA256 sha256;
    uint8_t ipad[64], opad[64];

    // Prepare key (pad or hash if needed)
    uint8_t keyBlock[64];
    memset(keyBlock, 0, 64);
    if (keyLen > 64) {
        sha256.reset();
        sha256.update(key, keyLen);
        sha256.finalize(keyBlock, 32);
    } else {
        memcpy(keyBlock, key, keyLen);
    }

    // Create ipad and opad
    for (int i = 0; i < 64; i++) {
        ipad[i] = keyBlock[i] ^ 0x36;
        opad[i] = keyBlock[i] ^ 0x5c;
    }

    // Inner hash: SHA256(ipad || data)
    uint8_t innerHash[32];
    sha256.reset();
    sha256.update(ipad, 64);
    sha256.update(data, dataLen);
    sha256.finalize(innerHash, 32);

    // Outer hash: SHA256(opad || innerHash)
    sha256.reset();
    sha256.update(opad, 64);
    sha256.update(innerHash, 32);
    sha256.finalize(output, 32);
}

void hkdf_derive_keys(const uint8_t* uid, uint8_t uidLen) {
    // HKDF-Extract: PRK = HMAC-SHA256(salt=master, IKM=uid)
    uint8_t prk[32];
    hmac_sha256(BAMBU_MASTER_KEY, 16, uid, uidLen, prk);

    // HKDF-Expand: Generate 96 bytes (16 sectors * 6 bytes)
    uint8_t t[32] = {0};
    uint8_t tLen = 0;
    uint8_t counter = 1;
    int okmOffset = 0;

    while (okmOffset < 96) {
        // Build input: T(n-1) || context || counter
        uint8_t input[64];
        int inputLen = 0;

        if (tLen > 0) {
            memcpy(input, t, tLen);
            inputLen = tLen;
        }
        memcpy(input + inputLen, BAMBU_CONTEXT, 7);  // includes null terminator
        inputLen += 7;
        input[inputLen++] = counter;

        // T(n) = HMAC-SHA256(PRK, T(n-1) || context || counter)
        hmac_sha256(prk, 32, input, inputLen, t);
        tLen = 32;

        int copyLen = (96 - okmOffset < 32) ? (96 - okmOffset) : 32;
        memcpy(bambuKeys + okmOffset, t, copyLen);
        okmOffset += copyLen;
        counter++;
    }

    keysGenerated = true;
    Serial.println("HKDF keys generated");
}

uint8_t* getSectorKey(uint8_t sector) {
    return &bambuKeys[sector * 6];
}

uint8_t* getBlockKey(uint8_t block) {
    return getSectorKey(block / 4);
}

// ============================================================================
// PN5180 Low-level functions
// ============================================================================

void waitBusy() {
    uint32_t start = millis();
    while (digitalRead(PN5180_BUSY) == LOW) {
        if (millis() - start > 10) break;
    }
    while (digitalRead(PN5180_BUSY) == HIGH) {
        if (millis() - start > 100) return;
    }
}

void pn5180_sendData(const uint8_t* data, uint8_t len, uint8_t validBits) {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x09);  // SEND_DATA
    SPI.transfer(validBits);
    for (uint8_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
}

void pn5180_readData(uint8_t* buf, uint8_t len) {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(2);
    SPI.transfer(0x0A);  // READ_DATA
    SPI.transfer(0x00);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    waitBusy();

    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(2);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SPI.transfer(0xFF);
    }
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
}

void pn5180_readEeprom(uint8_t addr, uint8_t* buf, uint8_t len) {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x07);
    SPI.transfer(addr);
    SPI.transfer(len);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
    delayMicroseconds(100);

    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SPI.transfer(0xFF);
    }
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
}

void pn5180_writeRegister(uint8_t reg, uint32_t value) {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x00);
    SPI.transfer(reg);
    SPI.transfer(value & 0xFF);
    SPI.transfer((value >> 8) & 0xFF);
    SPI.transfer((value >> 16) & 0xFF);
    SPI.transfer((value >> 24) & 0xFF);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
}

void pn5180_writeRegisterAndMask(uint8_t reg, uint32_t mask) {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x02);
    SPI.transfer(reg);
    SPI.transfer(mask & 0xFF);
    SPI.transfer((mask >> 8) & 0xFF);
    SPI.transfer((mask >> 16) & 0xFF);
    SPI.transfer((mask >> 24) & 0xFF);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
}

void pn5180_writeRegisterOrMask(uint8_t reg, uint32_t mask) {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x01);  // WRITE_REGISTER_OR_MASK
    SPI.transfer(reg);
    SPI.transfer(mask & 0xFF);
    SPI.transfer((mask >> 8) & 0xFF);
    SPI.transfer((mask >> 16) & 0xFF);
    SPI.transfer((mask >> 24) & 0xFF);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
}

uint32_t pn5180_readRegister(uint8_t reg) {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x04);
    SPI.transfer(reg);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
    delayMicroseconds(100);

    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    uint32_t val = SPI.transfer(0xFF);
    val |= ((uint32_t)SPI.transfer(0xFF)) << 8;
    val |= ((uint32_t)SPI.transfer(0xFF)) << 16;
    val |= ((uint32_t)SPI.transfer(0xFF)) << 24;
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    return val;
}

void pn5180_loadRfConfig(uint8_t tx, uint8_t rx) {
    pn5180_writeRegister(0x03, 0xFFFFFFFF);
    delayMicroseconds(100);

    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x11);
    SPI.transfer(tx);
    SPI.transfer(rx);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
    delay(10);
}

void pn5180_rfOn() {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x16);
    SPI.transfer(0x00);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
    delay(10);
}

void pn5180_rfOff() {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(5);
    SPI.transfer(0x17);
    SPI.transfer(0x00);
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(100);
    waitBusy();
    delay(5);
}

void pn5180_setTransceiveMode() {
    uint32_t sysConfig = pn5180_readRegister(0x00);
    sysConfig = (sysConfig & 0xFFFFFFF8) | 0x03;
    pn5180_writeRegister(0x00, sysConfig);
}

void pn5180_hardReset() {
    Serial.println("*** HARD RESET ***");
    digitalWrite(PN5180_RST, LOW);
    delay(50);
    digitalWrite(PN5180_RST, HIGH);
    delay(100);
    waitBusy();
    delay(50);
    pn5180_loadRfConfig(0x00, 0x80);
    delay(20);
    pn5180_rfOn();
    delay(50);
    consecutiveFailures = 0;
    lastResetTime = millis();

    Serial.println("Reset complete");
}

// ============================================================================
// MIFARE Crypto1 Implementation
// ============================================================================

// Crypto1 LFSR state (48 bits)
static uint64_t crypto1_state;

// Crypto1 filter function - takes 20 bits, produces 1 bit
static uint8_t crypto1_filter(uint32_t x) {
    // Filter function f(a,b,c,d,e) uses 5 groups of 4 bits each
    static const uint32_t filter_table[2][16] = {
        {0x9e98, 0x9e98, 0x9ee8, 0x9ee8, 0x9e98, 0x9e98, 0x9ee8, 0x9ee8,
         0x9e98, 0x9e98, 0x9ee8, 0x9ee8, 0x9e98, 0x9e98, 0x9ee8, 0x9ee8},
        {0xb48e, 0xb48e, 0xb4ce, 0xb4ce, 0xb48e, 0xb48e, 0xb4ce, 0xb4ce,
         0xb48e, 0xb48e, 0xb4ce, 0xb4ce, 0xb48e, 0xb48e, 0xb4ce, 0xb4ce}
    };

    uint32_t i;
    // fa fb fc fd fe are each 4-bit slices
    uint32_t fa = (x >> 16) & 0xf;
    uint32_t fb = (x >> 12) & 0xf;
    uint32_t fc = (x >> 8) & 0xf;
    uint32_t fd = (x >> 4) & 0xf;
    uint32_t fe = x & 0xf;

    // Lookup table approach for the filter function
    i = ((filter_table[0][fa] >> fb) & 1) << 4 |
        ((filter_table[0][fc] >> fd) & 1) << 3 |
        ((filter_table[1][fa] >> fb) & 1) << 2 |
        ((filter_table[1][fc] >> fd) & 1) << 1 |
        ((0xEC57E80A >> (fa << 1 | ((fb >> 3) & 1))) & 1);

    return (0xFFD8 >> ((i << 1) | (fe >> 3 & 1))) & 1;
}

// Simpler filter function implementation
static uint8_t f20(uint64_t x) {
    // 20-bit filter using standard Crypto1 filter
    uint32_t i = ((x >> 16) & 0xf) << 16 |
                 ((x >> 12) & 0xf) << 12 |
                 ((x >> 8) & 0xf) << 8 |
                 ((x >> 4) & 0xf) << 4 |
                 (x & 0xf);

    // Standard MIFARE Crypto1 filter function
    // f(x0..x19) implemented as table lookups
    uint32_t f;
    f  = (0x0EC57E80 >> (((x >> 15) & 0x0F) << 1)) & 0x02;  // fa
    f |= (0x0EC57E80 >> (((x >> 11) & 0x0F) << 1)) & 0x01;  // fb
    f  = (0x6E57E0A0 >> (f << 2 | ((x >> 7) & 0x03)));       // fc, fd
    f  = (0xFFD8 >> (f << 2 | ((x >> 3) & 0x03)));           // fe
    return f & 1;
}

// Crypto1 LFSR feedback
static uint8_t crypto1_bit(uint8_t in, int is_encrypted) {
    uint64_t lfsr = crypto1_state;

    // Feedback polynomial: x^48 + x^43 + x^39 + x^38 + x^36 + x^34 + x^33 + x^31 +
    //                      x^29 + x^24 + x^23 + x^21 + x^19 + x^13 + x^9 + x^7 + x^6 + x^5 + 1
    uint8_t feedback = (lfsr >> 0) ^ (lfsr >> 5) ^ (lfsr >> 9) ^ (lfsr >> 10) ^
                       (lfsr >> 12) ^ (lfsr >> 14) ^ (lfsr >> 15) ^ (lfsr >> 17) ^
                       (lfsr >> 19) ^ (lfsr >> 24) ^ (lfsr >> 25) ^ (lfsr >> 27) ^
                       (lfsr >> 29) ^ (lfsr >> 35) ^ (lfsr >> 39) ^ (lfsr >> 41) ^
                       (lfsr >> 42) ^ (lfsr >> 43);
    feedback &= 1;

    // Output is filter function of bits 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38
    uint32_t filter_input =
        ((lfsr >> 9) & 0x1) << 19 | ((lfsr >> 11) & 0x1) << 18 |
        ((lfsr >> 13) & 0x1) << 17 | ((lfsr >> 15) & 0x1) << 16 |
        ((lfsr >> 17) & 0x1) << 15 | ((lfsr >> 19) & 0x1) << 14 |
        ((lfsr >> 21) & 0x1) << 13 | ((lfsr >> 23) & 0x1) << 12 |
        ((lfsr >> 25) & 0x1) << 11 | ((lfsr >> 27) & 0x1) << 10 |
        ((lfsr >> 29) & 0x1) << 9 | ((lfsr >> 31) & 0x1) << 8 |
        ((lfsr >> 33) & 0x1) << 7 | ((lfsr >> 35) & 0x1) << 6 |
        ((lfsr >> 37) & 0x1) << 5 | ((lfsr >> 39) & 0x1) << 4 |
        ((lfsr >> 41) & 0x1) << 3 | ((lfsr >> 43) & 0x1) << 2 |
        ((lfsr >> 45) & 0x1) << 1 | ((lfsr >> 47) & 0x1);

    uint8_t out = f20(filter_input);

    // Shift LFSR and insert new bit
    crypto1_state = (lfsr >> 1) | ((uint64_t)(feedback ^ in ^ (is_encrypted ? out : 0)) << 47);

    return out;
}

// Generate one byte of keystream
static uint8_t crypto1_byte(uint8_t in, int is_encrypted) {
    uint8_t out = 0;
    for (int i = 0; i < 8; i++) {
        out |= crypto1_bit((in >> i) & 1, is_encrypted) << i;
    }
    return out;
}

// Generate 32-bit word of keystream
static uint32_t crypto1_word(uint32_t in, int is_encrypted) {
    uint32_t out = 0;
    for (int i = 0; i < 32; i++) {
        out |= (uint32_t)crypto1_bit((in >> i) & 1, is_encrypted) << i;
    }
    return out;
}

// Initialize Crypto1 with key and UID
static void crypto1_init(const uint8_t* key, uint32_t uid) {
    // Load key into LFSR
    crypto1_state = 0;
    for (int i = 47; i >= 0; i--) {
        crypto1_state = (crypto1_state << 1) | ((key[i / 8] >> (i % 8)) & 1);
    }

    // XOR in UID
    for (int i = 0; i < 32; i++) {
        crypto1_bit((uid >> i) & 1, 0);
    }
}

// PRNG successor function (for nonce)
static uint32_t prng_successor(uint32_t x, uint32_t n) {
    // MIFARE PRNG: 16-bit LFSR with polynomial x^16 + x^14 + x^13 + x^11 + 1
    while (n--) {
        x = x >> 1 | (((x >> 0) ^ (x >> 2) ^ (x >> 3) ^ (x >> 5)) & 1) << 15;
    }
    return x;
}

// ============================================================================
// MIFARE Authentication and Block Reading
// ============================================================================

// PN5180 MFC_AUTHENTICATE host command (0x0C)
// This is a HOST command sent via SPI, NOT an RF command!
// The PN5180 handles all Crypto1 internally.
// Reference: https://github.com/tueddy/PN5180-Library
//
// NOTE: After MFC_AUTHENTICATE, SPI register reads return garbage.
// We can't verify auth via SYSTEM_CONFIG. Instead, we just proceed
// and check if the subsequent read works.
bool mifare_authenticate(uint8_t blockNum, const uint8_t* key) {
    logSeqStart("MFC_AUTH block ");
    Serial.print(blockNum);
    Serial.print(" key=");
    for (int i = 0; i < 6; i++) {
        if (key[i] < 0x10) Serial.print("0");
        Serial.print(key[i], HEX);
    }
    Serial.println();

    // Wait for BUSY to go LOW before starting (PN5180 ready)
    uint32_t waitStart = millis();
    while (digitalRead(PN5180_BUSY) == HIGH) {
        if (millis() - waitStart > 100) {
            logSeq("MFC_AUTH: Timeout waiting for BUSY LOW (pre)");
            return false;
        }
    }

    // Send MFC_AUTHENTICATE host command (0x0C)
    // Format: [cmd][key 6 bytes][keyType][blockNo][uid 4 bytes] = 13 bytes
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(2);

    SPI.transfer(0x0C);  // MFC_AUTHENTICATE host command
    for (int i = 0; i < 6; i++) {
        SPI.transfer(key[i]);  // 6-byte key
    }
    SPI.transfer(0x60);  // Key A (0x60) or Key B (0x61)
    SPI.transfer(blockNum);
    for (int i = 0; i < 4; i++) {
        SPI.transfer(tagUid[i]);  // 4-byte UID (first 4 bytes)
    }

    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();

    // Wait for BUSY to go HIGH (PN5180 processing)
    waitStart = millis();
    while (digitalRead(PN5180_BUSY) == LOW) {
        if (millis() - waitStart > 10) break;
    }

    // Wait for BUSY to go LOW again (PN5180 done)
    waitStart = millis();
    while (digitalRead(PN5180_BUSY) == HIGH) {
        if (millis() - waitStart > 1000) {  // Auth can take up to 500ms
            logSeq("MFC_AUTH: Timeout waiting for BUSY LOW (post)");
            return false;
        }
    }

    // Read the 1-byte response
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PN5180_NSS, LOW);
    delayMicroseconds(2);
    uint8_t response = SPI.transfer(0xFF);  // Clock in response byte
    digitalWrite(PN5180_NSS, HIGH);
    SPI.endTransaction();

    logSeqStart("MFC_AUTH response=0x");
    Serial.println(response, HEX);

    if (response != 0x00) {
        logSeqStart("MFC_AUTH FAILED: status ");
        Serial.println(response);
        return false;
    }

    logSeq("MFC_AUTH SUCCESS!");
    return true;
}

// Read a single MIFARE block (16 bytes)
// Assumes authentication has already been done (Crypto1 active)
bool mifare_readBlock(uint8_t blockNum, uint8_t* buf) {
    logSeqStart("READ block ");
    Serial.println(blockNum);

    // Clear IRQ
    pn5180_writeRegister(0x03, 0xFFFFFFFF);

    // After successful MFC_AUTHENTICATE, Crypto1 is already active
    // Just need to set transceive mode with CRC enabled
    // Note: CRYPTO1_ON bit (0x40) is set automatically by MFC_AUTHENTICATE
    pn5180_setTransceiveMode();
    delay(1);

    // Enable TX and RX CRC for MIFARE encrypted read
    pn5180_writeRegisterOrMask(0x19, 0x01);  // TX CRC on
    pn5180_writeRegisterOrMask(0x12, 0x01);  // RX CRC on

    // Send MIFARE READ command (0x30 + block number)
    uint8_t readCmd[2] = {0x30, blockNum};
    pn5180_sendData(readCmd, 2, 0x00);
    delay(10);

    // Check RX status
    uint32_t rxStatus = pn5180_readRegister(0x13);
    uint16_t rxLen = rxStatus & 0x1FF;

    logSeqStart("Read rxLen=");
    Serial.println(rxLen);

    if (rxLen != 16) {
        Serial.print("Read block ");
        Serial.print(blockNum);
        Serial.print(" failed, rxLen=");
        Serial.println(rxLen);
        return false;
    }

    // Read the 16 bytes
    pn5180_readData(buf, 16);
    return true;
}

// Re-select the card (needed before authentication after RF toggle)
bool reactivateCard() {
    // Brief RF cycle to reset card state
    pn5180_rfOff();
    delay(10);
    pn5180_writeRegister(0x03, 0xFFFFFFFF);  // Clear IRQ
    pn5180_loadRfConfig(0x00, 0x80);
    delay(5);
    pn5180_rfOn();
    delay(20);

    // Crypto off, CRC off for WUPA
    pn5180_writeRegisterAndMask(0x00, 0xFFFFFFBF);  // Crypto off
    pn5180_writeRegisterAndMask(0x12, 0xFFFFFFFE);  // RX CRC off
    pn5180_writeRegisterAndMask(0x19, 0xFFFFFFFE);  // TX CRC off
    pn5180_writeRegister(0x03, 0xFFFFFFFF);  // Clear IRQ
    pn5180_setTransceiveMode();
    delay(2);

    uint8_t wupa = 0x52;
    pn5180_sendData(&wupa, 1, 0x07);
    delay(5);

    uint32_t rxStatus = pn5180_readRegister(0x13);
    uint16_t rxLen = rxStatus & 0x1FF;
    if (rxLen < 2) {
        logSeq("Reactivate: no ATQA");
        return false;
    }

    // Read ATQA
    uint8_t atqa[2];
    pn5180_readData(atqa, 2);

    // Anticollision
    pn5180_writeRegister(0x03, 0xFFFFFFFF);
    pn5180_setTransceiveMode();
    delay(2);

    uint8_t anticol[2] = {0x93, 0x20};
    pn5180_sendData(anticol, 2, 0x00);
    delay(10);

    rxStatus = pn5180_readRegister(0x13);
    rxLen = rxStatus & 0x1FF;
    if (rxLen < 5) {
        logSeq("Reactivate: anticol failed");
        return false;
    }

    uint8_t uidBuf[5];
    pn5180_readData(uidBuf, 5);

    // SELECT to put card in ACTIVE state
    pn5180_writeRegister(0x03, 0xFFFFFFFF);
    pn5180_setTransceiveMode();
    delay(2);

    // Enable CRC for SELECT
    pn5180_writeRegisterOrMask(0x19, 0x01);
    pn5180_writeRegisterOrMask(0x12, 0x01);

    uint8_t bcc = uidBuf[0] ^ uidBuf[1] ^ uidBuf[2] ^ uidBuf[3];
    uint8_t selectCmd[7] = {0x93, 0x70, uidBuf[0], uidBuf[1], uidBuf[2], uidBuf[3], bcc};
    pn5180_sendData(selectCmd, 7, 0x00);
    delay(10);

    rxStatus = pn5180_readRegister(0x13);
    rxLen = rxStatus & 0x1FF;
    if (rxLen < 1) {
        logSeq("Reactivate: SELECT failed");
        return false;
    }

    // Read and discard SAK to clear RX buffer
    uint8_t sak[3];
    pn5180_readData(sak, rxLen > 3 ? 3 : rxLen);
    logSeqStart("Reactivate OK, SAK=0x");
    Serial.println(sak[0], HEX);

    return true;
}

// Read Bambu tag blocks (1, 2, 4, 5)
bool readBambuTagData() {
    if (!keysGenerated) {
        logSeq("Keys not generated!");
        return false;
    }

    logSeqStart("Reading Bambu tag UID=");
    for (int i = 0; i < tagUidLen; i++) {
        if (tagUid[i] < 0x10) Serial.print("0");
        Serial.print(tagUid[i], HEX);
    }
    Serial.println();

    // Card should already be in ACTIVE state from CMD_SCAN_TAG
    // Skip reactivation - WUPA fails because card is already active, not in IDLE/HALT
    // Just ensure crypto is off before starting authentication
    pn5180_writeRegisterAndMask(0x00, 0xFFFFFFBF);  // Clear MFC_CRYPTO1_ON
    pn5180_writeRegister(0x03, 0xFFFFFFFF);  // Clear IRQs

    const uint8_t blocksToRead[] = {1, 2, 4, 5};
    int currentSector = -1;

    tagDataValid = false;
    memset(tagBlocks, 0, sizeof(tagBlocks));

    // Print all derived keys for debugging

    Serial.println("Derived keys for all sectors:");
    for (int s = 0; s < 4; s++) {
        Serial.print("  Sector ");
        Serial.print(s);
        Serial.print(": ");
        uint8_t* k = getSectorKey(s);
        for (int j = 0; j < 6; j++) {
            if (k[j] < 0x10) Serial.print("0");
            Serial.print(k[j], HEX);
        }
        Serial.println();
    }

    // Card may have timed out since CMD_SCAN_TAG - reactivate it first
    logSeq("Reactivating card...");
    if (!reactivateCard()) {
        logSeq("Reactivate FAILED");
        return false;
    }

    for (int i = 0; i < 4; i++) {
        uint8_t block = blocksToRead[i];
        uint8_t sector = block / 4;

        // Authenticate if sector changed
        if (sector != currentSector) {
            uint8_t* key = getSectorKey(sector);

            // CRITICAL TIMING: Must authenticate IMMEDIATELY after reactivate!
            // MIFARE has ~5ms timeout after SELECT before it expects AUTH command.
            // Move all debug output AFTER authentication attempt.

            bool authOk = mifare_authenticate(block, key);

            // Now safe to print debug info
            Serial.print("Authenticated block ");
            Serial.print(block);
            Serial.print(" (sector ");
            Serial.print(sector);
            Serial.print(") with key: ");
            for (int j = 0; j < 6; j++) {
                if (key[j] < 0x10) Serial.print("0");
                Serial.print(key[j], HEX);
            }
            Serial.print(" -> ");
            Serial.println(authOk ? "OK" : "FAILED");

            if (!authOk) {
                // Reactivate card for retry with default key
                logSeq("Trying default key FFFFFFFFFFFF...");
                if (!reactivateCard()) {
                    logSeq("Reactivate for default key failed");
                    return false;
                }
                uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                if (mifare_authenticate(block, defaultKey)) {
                    logSeq("DEFAULT KEY WORKED - wrong derived key!");
                } else {
                    logSeq("Default key also failed - auth mechanism issue");
                }
                return false;
            }
            currentSector = sector;
        }

        // Read the block
        if (!mifare_readBlock(block, tagBlocks[i])) {
            logSeqStart("Read FAILED block ");
            Serial.println(block);
            return false;
        }

        logSeqStart("Block ");
        Serial.print(block);
        Serial.print(": ");
        for (int j = 0; j < 16; j++) {
            if (tagBlocks[i][j] < 0x10) Serial.print("0");
            Serial.print(tagBlocks[i][j], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    logSeq("Read complete!");
    tagDataValid = true;
    return true;
}

// ============================================================================
// NTAG Reading (for SpoolEase/OpenPrintTag)
// ============================================================================

// Read NTAG pages (4 bytes each, reads 4 pages = 16 bytes at a time)
bool ntag_readPages(uint8_t startPage, uint8_t* buf, uint8_t numPages) {
    pn5180_writeRegister(0x03, 0xFFFFFFFF);
    pn5180_setTransceiveMode();

    // Disable CRC for NTAG READ
    pn5180_writeRegisterAndMask(0x19, 0xFFFFFFFE);  // TX CRC off
    pn5180_writeRegisterAndMask(0x12, 0xFFFFFFFE);  // RX CRC off

    uint8_t pagesRead = 0;
    while (pagesRead < numPages) {
        // NTAG READ command: 0x30 + page number, returns 16 bytes (4 pages)
        uint8_t readCmd[2] = {0x30, (uint8_t)(startPage + pagesRead)};
        pn5180_sendData(readCmd, 2, 0x00);
        delay(5);

        uint32_t rxStatus = pn5180_readRegister(0x13);
        uint16_t rxLen = rxStatus & 0x1FF;

        if (rxLen < 16) {
            Serial.print("NTAG read failed at page ");
            Serial.println(startPage + pagesRead);
            return false;
        }

        uint8_t pagesToCopy = (numPages - pagesRead > 4) ? 4 : (numPages - pagesRead);
        uint8_t temp[16];
        pn5180_readData(temp, 16);
        memcpy(buf + (pagesRead * 4), temp, pagesToCopy * 4);
        pagesRead += 4;
    }

    return true;
}

// ============================================================================
// Tag Activation (with SAK detection)
// ============================================================================

// Returns: 0 = no tag, 4/7/10 = UID length, 0xFF = chip stuck
uint8_t activateTypeA(uint8_t *uid, uint8_t *sak) {
    // Turn OFF Crypto
    pn5180_writeRegisterAndMask(0x00, 0xFFFFFFBF);
    // Clear CRCs
    pn5180_writeRegisterAndMask(0x12, 0xFFFFFFFE);
    pn5180_writeRegisterAndMask(0x19, 0xFFFFFFFE);
    pn5180_writeRegister(0x03, 0xFFFFFFFF);

    // Reset transceive
    uint32_t sysConfig = pn5180_readRegister(0x00);
    pn5180_writeRegister(0x00, sysConfig & 0xFFFFFFF8);
    delay(1);
    pn5180_writeRegister(0x00, (sysConfig & 0xFFFFFFF8) | 0x03);
    delay(2);

    // Send WUPA
    uint8_t wupa = 0x52;
    pn5180_sendData(&wupa, 1, 0x07);
    delay(5);

    uint32_t rxStatus = pn5180_readRegister(0x13);
    uint16_t rxLen = rxStatus & 0x1FF;

    if (rxLen == 511) {

        Serial.println("STUCK: rxLen=511");
        return 0xFF;
    }

    if (rxLen < 2) {
        // Try REQA
        pn5180_writeRegister(0x03, 0xFFFFFFFF);
        delay(2);
        uint8_t reqa = 0x26;
        pn5180_sendData(&reqa, 1, 0x07);
        delay(5);

        rxStatus = pn5180_readRegister(0x13);
        rxLen = rxStatus & 0x1FF;
        if (rxLen == 511) return 0xFF;
        if (rxLen < 2) return 0;
    }

    // Read ATQA
    uint8_t atqa[2];
    pn5180_readData(atqa, 2);
    if (atqa[0] == 0xFF && atqa[1] == 0xFF) return 0xFF;
    if (atqa[0] == 0xFF || atqa[0] == 0x00) return 0;

    // Anti-collision Level 1
    pn5180_writeRegister(0x03, 0xFFFFFFFF);
    sysConfig = pn5180_readRegister(0x00);
    pn5180_writeRegister(0x00, (sysConfig & 0xFFFFFFF8) | 0x03);
    delay(2);

    uint8_t anticol[2] = {0x93, 0x20};
    pn5180_sendData(anticol, 2, 0x00);
    delay(10);

    rxStatus = pn5180_readRegister(0x13);
    rxLen = rxStatus & 0x1FF;
    if (rxLen < 5 || rxLen > 64) return 0;

    uint8_t uidBuf[5];
    pn5180_readData(uidBuf, 5);
    memcpy(uid, uidBuf, 4);

    uint8_t bcc = uid[0] ^ uid[1] ^ uid[2] ^ uid[3];
    if (bcc != uidBuf[4]) return 0;

    // SELECT command to get SAK
    pn5180_writeRegister(0x03, 0xFFFFFFFF);
    pn5180_setTransceiveMode();
    delay(2);

    // Enable TX CRC for SELECT
    pn5180_writeRegisterOrMask(0x19, 0x01);
    pn5180_writeRegisterOrMask(0x12, 0x01);

    uint8_t selectCmd[7] = {0x93, 0x70, uid[0], uid[1], uid[2], uid[3], bcc};
    pn5180_sendData(selectCmd, 7, 0x00);
    delay(10);

    rxStatus = pn5180_readRegister(0x13);
    rxLen = rxStatus & 0x1FF;

    // SAK response should be 1 byte (+ 2 CRC bytes stripped by PN5180 with CRC on)
    // But sometimes we get garbage rxLen - validate it
    if (rxLen >= 1 && rxLen <= 3) {
        uint8_t sakBuf[3];
        pn5180_readData(sakBuf, rxLen);
        *sak = sakBuf[0];

        Serial.print("SAK: 0x");
        Serial.print(*sak, HEX);
        Serial.print(" (rxLen=");
        Serial.print(rxLen);
        Serial.println(")");
    } else if (rxLen > 3) {
        // Suspicious rxLen - might have stale data, flush and retry
        Serial.print("WARNING: SAK rxLen=");
        Serial.print(rxLen);
        Serial.println(" - flushing buffer");

        // Read and discard the data
        uint8_t discard[16];
        pn5180_readData(discard, rxLen > 16 ? 16 : rxLen);

        // Try to get just the first byte as SAK
        *sak = discard[0];
        Serial.print("Using first byte as SAK: 0x");
        Serial.println(*sak, HEX);
    } else {
        Serial.println("No SAK response");
        *sak = 0;
    }

    return 4;
}

uint8_t getTagType(uint8_t sak) {
    if (sak == 0x00) return TAG_TYPE_NTAG;
    if (sak == 0x08) return TAG_TYPE_MIFARE_1K;
    if (sak == 0x18) return TAG_TYPE_MIFARE_4K;
    return TAG_TYPE_UNKNOWN;
}

// ============================================================================
// Tag Scanning
// ============================================================================

bool scanTag() {
    static uint8_t noTagCount = 0;

    if (noTagCount > 3) {

        Serial.println("No tag for 3 scans - HARD RESET");
        pn5180_hardReset();
        noTagCount = 0;
        return false;
    }

    pn5180_rfOff();
    delay(20);
    pn5180_writeRegister(0x03, 0xFFFFFFFF);
    delay(5);
    pn5180_loadRfConfig(0x00, 0x80);
    delay(10);
    pn5180_rfOn();
    delay(30);
    pn5180_setTransceiveMode();

    uint8_t uid[10];
    uint8_t sak = 0;
    uint8_t uidLen = activateTypeA(uid, &sak);

    if (uidLen == 0xFF) {
        consecutiveFailures++;
        noTagCount++;
        if (consecutiveFailures >= MAX_FAILURES_BEFORE_RESET) {
            pn5180_hardReset();
            noTagCount = 0;
        }
        tagPresent = false;
        tagDataValid = false;
        return false;
    }

    if (uidLen > 0 && uidLen <= 10) {
        consecutiveFailures = 0;
        noTagCount = 0;

        // Check if this is a new tag
        bool newTag = (tagUidLen != uidLen) || memcmp(tagUid, uid, uidLen) != 0;

        tagUidLen = uidLen;
        memcpy(tagUid, uid, uidLen);
        tagSak = sak;
        tagType = getTagType(sak);
        tagPresent = true;

        Serial.print("scanTag: SAK=0x");
        Serial.print(sak, HEX);
        Serial.print(" -> tagType=");
        Serial.println(tagType);

        if (newTag) {
            tagDataValid = false;
            keysGenerated = false;


            Serial.print("New tag detected! Type: ");
            Serial.println(tagType);

            // Generate keys for Bambu tags
            if (tagType == TAG_TYPE_MIFARE_1K || tagType == TAG_TYPE_MIFARE_4K) {
                hkdf_derive_keys(tagUid, tagUidLen);
            }
        }

        return true;
    }

    noTagCount++;
    tagPresent = false;
    tagDataValid = false;
    return false;
}

// ============================================================================
// I2C Command Processing
// ============================================================================

void processCommand() {
    if (cmdLength == 0) return;

    processingCommand = true;  // Prevent background scan interference

    uint8_t cmd = cmdBuffer[0];
    // Extract sequence number if present (2nd byte)
    cmdSeq = (cmdLength >= 2) ? cmdBuffer[1] : 0;

    Serial.print("[#");
    Serial.print(cmdSeq);
    Serial.print("] CMD: 0x");
    Serial.println(cmd, HEX);

    switch (cmd) {
        case CMD_GET_STATUS:
            respBuffer[0] = lastStatus;
            respBuffer[1] = tagPresent ? 1 : 0;
            respLength = 2;
            break;

        case CMD_GET_PRODUCT_VERSION:
            respBuffer[0] = 0;
            respBuffer[1] = cachedVersion[0];
            respBuffer[2] = cachedVersion[1];
            respLength = 3;
            break;

        case CMD_SCAN_TAG:
            if (scanTag()) {
                respBuffer[0] = 0;  // Success
                respBuffer[1] = tagUidLen;
                memcpy((void*)&respBuffer[2], tagUid, tagUidLen);
                respLength = 2 + tagUidLen;
                // Protect card state for 2 seconds for follow-up CMD_READ_TAG_DATA
                scanProtectionUntil = millis() + 2000;
            } else {
                respBuffer[0] = 1;  // No tag
                respLength = 1;
            }
            break;

        case CMD_READ_TAG_DATA:
            Serial.print("READ_TAG_DATA: tagPresent=");
            Serial.print(tagPresent);
            Serial.print(" tagType=");
            Serial.print(tagType);
            Serial.print(" tagSak=0x");
            Serial.println(tagSak, HEX);

            if (!tagPresent) {
                respBuffer[0] = 1;  // No tag
                respLength = 1;
            } else if (tagType == TAG_TYPE_MIFARE_1K || tagType == TAG_TYPE_MIFARE_4K) {
                // Read Bambu tag data
                if (!tagDataValid) {
                    if (!readBambuTagData()) {
                        respBuffer[0] = 2;  // Read error
                        respLength = 1;
                        break;
                    }
                }
                // Response format:
                // [0] = status (0 = success)
                // [1] = tag type
                // [2] = uid length
                // [3..3+uidLen] = uid
                // Then for each block: 16 bytes
                // Blocks: 1, 2, 4, 5 = 64 bytes total
                respBuffer[0] = 0;  // Success
                respBuffer[1] = tagType;
                respBuffer[2] = tagUidLen;
                memcpy((void*)&respBuffer[3], tagUid, tagUidLen);
                int offset = 3 + tagUidLen;
                memcpy((void*)&respBuffer[offset], tagBlocks[0], 16); offset += 16;  // Block 1
                memcpy((void*)&respBuffer[offset], tagBlocks[1], 16); offset += 16;  // Block 2
                memcpy((void*)&respBuffer[offset], tagBlocks[2], 16); offset += 16;  // Block 4
                memcpy((void*)&respBuffer[offset], tagBlocks[3], 16); offset += 16;  // Block 5
                respLength = offset;
                Serial.print("Sending ");
                Serial.print(respLength);
                Serial.println(" bytes of tag data");
            } else if (tagType == TAG_TYPE_NTAG) {
                // Read NTAG pages 4-20 (NDEF data area)
                uint8_t ntagData[68];  // 17 pages * 4 bytes
                if (!ntag_readPages(4, ntagData, 17)) {
                    respBuffer[0] = 2;  // Read error
                    respLength = 1;
                    break;
                }
                respBuffer[0] = 0;  // Success
                respBuffer[1] = tagType;
                respBuffer[2] = tagUidLen;
                memcpy((void*)&respBuffer[3], tagUid, tagUidLen);
                int offset = 3 + tagUidLen;
                memcpy((void*)&respBuffer[offset], ntagData, 68);
                respLength = offset + 68;
                Serial.print("Sending ");
                Serial.print(respLength);
                Serial.println(" bytes of NTAG data");
            } else {
                respBuffer[0] = 3;  // Unknown tag type
                respLength = 1;
            }
            break;

        default:
            respBuffer[0] = 0xFF;
            respLength = 1;
    }

    processingCommand = false;  // Allow background scans again
}

void i2cReceive(int n) {

    Serial.print("I2C RX: ");
    Serial.print(n);
    Serial.print(" bytes: ");
    cmdLength = 0;
    while (Wire.available() && cmdLength < 64) {
        cmdBuffer[cmdLength++] = Wire.read();
    }
    for (int i = 0; i < cmdLength; i++) {
        Serial.print(cmdBuffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    cmdReady = true;
}

void i2cRequest() {
    Serial.print("I2C REQ: ");
    if (respLength > 0) {
        Serial.print(respLength);
        Serial.println(" bytes");
        Wire.write((uint8_t*)respBuffer, respLength);
        respLength = 0;
    } else {
        Serial.println("no data, sending 0xFF");
        Wire.write(0xFF);
    }
}

// ============================================================================
// Setup and Main Loop
// ============================================================================

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    delay(2000);
    Serial.println("Pico NFC Bridge v2.0 starting...");
    Serial.println("Features: MIFARE Classic + NTAG + Bambu HKDF");

    pinMode(PN5180_NSS, OUTPUT);
    digitalWrite(PN5180_NSS, HIGH);
    pinMode(PN5180_RST, OUTPUT);
    digitalWrite(PN5180_RST, HIGH);
    pinMode(PN5180_BUSY, INPUT);

    SPI.setRX(16);
    SPI.setTX(19);
    SPI.setSCK(18);
    SPI.begin();
    Serial.println("SPI OK");

    digitalWrite(PN5180_RST, LOW);
    delay(10);
    digitalWrite(PN5180_RST, HIGH);
    delay(50);
    waitBusy();
    Serial.println("Reset OK");

    pn5180_readEeprom(0x10, cachedVersion, 2);
    Serial.print("PN5180 version: ");
    Serial.print(cachedVersion[0]);
    Serial.print(".");
    Serial.println(cachedVersion[1]);

    if (cachedVersion[0] == 0xFF && cachedVersion[1] == 0xFF) {
        Serial.println("PN5180 ERROR!");
        lastStatus = 3;
    } else {
        pn5180_loadRfConfig(0x00, 0x80);
        pn5180_rfOn();
        Serial.println("RF ON (ISO14443A)");
        lastStatus = 0;
    }

    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);
    Wire.begin(I2C_ADDR);
    Wire.onReceive(i2cReceive);
    Wire.onRequest(i2cRequest);
    Serial.println("I2C ready at 0x55");

    Serial.println("Ready!");
}

void loop() {
    static uint32_t lastBlink = 0;
    static uint32_t lastScan = 0;
    static uint32_t lastI2cStatus = 0;

    if (millis() - lastBlink > 500) {
        lastBlink = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    if (millis() - lastI2cStatus > 5000) {
        lastI2cStatus = millis();

        Serial.print("I2C slave @ 0x");
        Serial.print(I2C_ADDR, HEX);
        Serial.print(" | Tag: ");
        Serial.println(tagPresent ? "YES" : "no");
    }

    if (millis() - lastScan > 1000) {
        lastScan = millis();

        // Skip scan if processing a command or in protection window
        if (processingCommand) {

            Serial.println("(skip scan - cmd)");
        } else if (millis() < scanProtectionUntil) {

            Serial.println("(skip scan - protected)");
        } else if (millis() - lastResetTime >= RESET_COOLDOWN_MS) {
            bool found = scanTag();
            if (found) {

                Serial.print("TAG: ");
                for (int i = 0; i < tagUidLen; i++) {
                    if (tagUid[i] < 0x10) Serial.print("0");
                    Serial.print(tagUid[i], HEX);
                }
                Serial.print(" Type: ");
                switch (tagType) {
                    case TAG_TYPE_NTAG: Serial.println("NTAG"); break;
                    case TAG_TYPE_MIFARE_1K: Serial.println("MIFARE 1K"); break;
                    case TAG_TYPE_MIFARE_4K: Serial.println("MIFARE 4K"); break;
                    default: Serial.println("Unknown"); break;
                }
            } else if (!tagPresent) {

                Serial.println(".");
            }
        } else {

            Serial.println("(cooldown)");
        }
    }

    if (cmdReady) {
        processCommand();
        cmdReady = false;
    }
}
