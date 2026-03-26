// #############################################################################
// ### Author(s) : Pascal Roobrouck - @strooom                               ###
// ### License : https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode ###
// #############################################################################

#include <zephyr/ztest.h>
#include <hexascii.hpp>

// Setup called before each test
static void *hexascii_setup(void) {
    return NULL;
}

// Teardown called after each test
static void hexascii_teardown(void *fixture) {
    (void)fixture;
}

ZTEST(hexascii_suite, test_toUpperCase) {
    zassert_equal('A', hexAscii::toUpperCase('a'), "Failed to convert 'a' to 'A'");
    zassert_equal('Z', hexAscii::toUpperCase('z'), "Failed to convert 'z' to 'Z'");
    zassert_equal('0', hexAscii::toUpperCase('0'), "Failed to preserve '0'");
    zassert_equal('9', hexAscii::toUpperCase('9'), "Failed to preserve '9'");
    zassert_equal('A', hexAscii::toUpperCase('A'), "Failed to convert 'a' to 'A'");
    zassert_equal('Z', hexAscii::toUpperCase('Z'), "Failed to convert 'z' to 'Z'");
    zassert_equal('|', hexAscii::toUpperCase('|'), "Failed to convert '|' to '|'");
}

ZTEST(hexascii_suite, test_isHexCharacter) {
    zassert_true(hexAscii::isHexCharacter('0'), "Should recognize '0'");
    zassert_true(hexAscii::isHexCharacter('9'), "Should recognize '9'");
    zassert_true(hexAscii::isHexCharacter('A'), "Should recognize 'A'");
    zassert_true(hexAscii::isHexCharacter('F'), "Should recognize 'F'");
    zassert_true(hexAscii::isHexCharacter('a'), "Should recognize 'a'");
    zassert_true(hexAscii::isHexCharacter('f'), "Should recognize 'f'");
    
    zassert_false(hexAscii::isHexCharacter('@'), "Should reject '@'");
    zassert_false(hexAscii::isHexCharacter('G'), "Should reject 'G'");
    zassert_false(hexAscii::isHexCharacter('_'), "Should reject '_'");
    zassert_false(hexAscii::isHexCharacter('g'), "Should reject 'g'");
    zassert_false(hexAscii::isHexCharacter('.'), "Should reject '.'");
    zassert_false(hexAscii::isHexCharacter('='), "Should reject '='");
}

ZTEST(hexascii_suite, test_valueFromHexCharacter) {
    zassert_equal(0x00, hexAscii::valueFromHexCharacter('0'), "Failed for '0'");
    zassert_equal(0x09, hexAscii::valueFromHexCharacter('9'), "Failed for '9'");
    zassert_equal(0x0A, hexAscii::valueFromHexCharacter('A'), "Failed for 'A'");
    zassert_equal(0x0F, hexAscii::valueFromHexCharacter('F'), "Failed for 'F'");
    zassert_equal(0x0A, hexAscii::valueFromHexCharacter('a'), "Failed for 'a'");
    zassert_equal(0x0F, hexAscii::valueFromHexCharacter('f'), "Failed for 'f'");
    // Edge cases
    zassert_equal(0x00, hexAscii::valueFromHexCharacter('.'), "Failed for edge case");
    zassert_equal(0x00, hexAscii::valueFromHexCharacter(':'), "Failed for edge case");
    zassert_equal(0x00, hexAscii::valueFromHexCharacter('_'), "Failed for edge case");
    zassert_equal(0x00, hexAscii::valueFromHexCharacter('|'), "Failed for edge case");
    zassert_equal(0x00, hexAscii::valueFromHexCharacter('@'), "Failed for edge case");
}

ZTEST(hexascii_suite, test_hexCharacterFromValue) {
    zassert_equal('0', hexAscii::hexCharacterFromValue(0x00), "Failed for 0x00");
    zassert_equal('9', hexAscii::hexCharacterFromValue(0x09), "Failed for 0x09");
    zassert_equal('A', hexAscii::hexCharacterFromValue(0x0A), "Failed for 0x0A");
    zassert_equal('F', hexAscii::hexCharacterFromValue(0x0F), "Failed for 0x0F");
    // Edge cases
    zassert_equal('?', hexAscii::hexCharacterFromValue(0x10), "Failed for 0x10 (out of range)");
}

ZTEST(hexascii_suite, test_hexStringToByteArray) {
    const char input[17] = "0123456789ABCDEF";
    uint8_t output[8];
    uint8_t expectedOutput[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    hexAscii::hexStringToByteArray(output, input, 16U);
    zassert_mem_equal(expectedOutput, output, 8, "Byte array mismatch");
}

ZTEST(hexascii_suite, test_hexStringToByteArrayReversed) {
    const char input[17] = "0123456789ABCDEF";
    uint8_t output[8];
    uint8_t expectedOutput[8] = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    hexAscii::hexStringToByteArrayReversed(output, input, 16U);
    zassert_mem_equal(expectedOutput, output, 8, "Byte array mismatch");
}


ZTEST(hexascii_suite, test_byteArrayToHexString) {
    uint8_t input[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    char output[17];  // 1 extra for terminating zero
    const char expectedOutput[17] = "0123456789ABCDEF";
    hexAscii::byteArrayToHexString(output, input, 8);
    zassert_str_equal(expectedOutput, output, "Hex string mismatch");
    zassert_equal(0x00, output[16], "Missing null terminator");
}

ZTEST(hexascii_suite, test_byteArrayToHexStringReversed) {
    uint8_t input[8] = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    char output[17];  // 1 extra for terminating zero
    const char expectedOutput[17] = "0123456789ABCDEF";
    hexAscii::byteArrayToHexStringReversed(output, input, 8);
    zassert_str_equal(expectedOutput, output, "Hex string mismatch");
    zassert_equal(0x00, output[16], "Missing null terminator");
}

ZTEST(hexascii_suite, test_uint32ToHexString) {
    const char expectedOutput1[9] = "01234567";
    const char expectedOutput2[9] = "FEDCBA98";
    char output[9];
    
    hexAscii::uint32ToHexString(output, 0x01234567);
    zassert_str_equal(expectedOutput1, output, "Failed for 0x01234567");
    
    hexAscii::uint32ToHexString(output, 0xFEDCBA98);
    zassert_str_equal(expectedOutput2, output, "Failed for 0xFEDCBA98");
    zassert_equal(0x00, output[16], "Missing null terminator");
}


ZTEST(hexascii_suite, test_uint64ToHexString) {
    const char expectedOutput1[17] = "0123456789ABCDEF";
    const char expectedOutput2[17] = "FEDCBA9876543210";
    char output[17];
    
    hexAscii::uint64ToHexString(output, 0x0123456789ABCDEF);
    zassert_str_equal(expectedOutput1, output, "Failed for 0x0123456789ABCDEF");
    
    hexAscii::uint64ToHexString(output, 0xFEDCBA9876543210);
    zassert_str_equal(expectedOutput2, output, "Failed for 0xFEDCBA9876543210");
    zassert_equal(0x00, output[16], "Missing null terminator");
}

// Define test suite with setup/teardown callbacks
ZTEST_SUITE(hexascii_suite, NULL, hexascii_setup, hexascii_teardown, NULL, NULL);