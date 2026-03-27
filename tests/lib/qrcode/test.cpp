// #############################################################################
// ### Author(s) : Pascal Roobrouck - @strooom                               ###
// ### License : https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode ###
// #############################################################################
#include <zephyr/ztest.h>
#include <stdio.h>
#include <reedsolomon.hpp>
#include <qrcode.hpp>

// Helper macro: check each byte of an array equals val
#define ZASSERT_EACH_EQUAL_UINT8(val, arr, len) do { for (size_t _i = 0; _i < (size_t)(len); _i++) { zassert_equal((uint8_t)(val), (arr)[_i]); } } while (0)


#pragma region testing internal data initialization / setting

ZTEST(qrcode_suite, test_initialization) {
    static constexpr uint32_t testVersion{1};
    const uint32_t size = qrCode::size(testVersion);
    static const errorCorrectionLevel testErrorCorrectionLevel{errorCorrectionLevel::low};
    qrCode::initialize(testVersion, testErrorCorrectionLevel);
    zassert_equal(testVersion, qrCode::theVersion);
    zassert_equal(size, qrCode::modules.getWidthHeightInBits());
    zassert_equal(size, qrCode::isData.getWidthHeightInBits());
    zassert_equal(testErrorCorrectionLevel, qrCode::theErrorCorrectionLevel);

    ZASSERT_EACH_EQUAL_UINT8(0, qrCode::modules.data, qrCode::modules.getSizeInBytes());
    ZASSERT_EACH_EQUAL_UINT8(0xFF, qrCode::isData.data, qrCode::isData.getSizeInBytes());
    ZASSERT_EACH_EQUAL_UINT8(0, qrCode::buffer.data, qrCode::buffer.lengthInBytes);
}
#pragma endregion
#pragma region testing internal helpers

ZTEST(qrcode_suite, test_size) {
    zassert_equal(21U, qrCode::size(1U));
    zassert_equal(81U, qrCode::size(16U));
    zassert_equal(177U, qrCode::size(40U));
}

#pragma endregion
#pragma region testing encoding user data into payload
ZTEST(qrcode_suite, test_isNumeric) {
    // single character
    zassert_true(qrCode::isNumeric('0'));
    zassert_true(qrCode::isNumeric('1'));
    zassert_true(qrCode::isNumeric('8'));
    zassert_true(qrCode::isNumeric('9'));
    zassert_false(qrCode::isNumeric('/'));
    zassert_false(qrCode::isNumeric(':'));
    // null-terminated string
    zassert_true(qrCode::isNumeric("0123456789"));
    zassert_false(qrCode::isNumeric("0/1"));
    zassert_false(qrCode::isNumeric("0:1"));
    // byte array with length
    const uint8_t testData1[10]{48, 49, 50, 51, 52, 53, 54, 55, 56, 57};
    zassert_true(qrCode::isNumeric(testData1, 4));
    const uint8_t testData2[2]{47, 58};
    zassert_false(qrCode::isNumeric(testData2, 2));
}

ZTEST(qrcode_suite, test_isAlphanumeric) {
    // single character
    zassert_true(qrCode::isAlphanumeric('0'));
    zassert_true(qrCode::isAlphanumeric('9'));
    zassert_true(qrCode::isAlphanumeric('A'));
    zassert_true(qrCode::isAlphanumeric('Z'));
    zassert_true(qrCode::isAlphanumeric(' '));
    zassert_true(qrCode::isAlphanumeric('$'));
    zassert_true(qrCode::isAlphanumeric('%'));
    zassert_true(qrCode::isAlphanumeric('*'));
    zassert_true(qrCode::isAlphanumeric('+'));
    zassert_true(qrCode::isAlphanumeric('-'));
    zassert_true(qrCode::isAlphanumeric('.'));
    zassert_true(qrCode::isAlphanumeric('/'));
    zassert_true(qrCode::isAlphanumeric(':'));
    zassert_false(qrCode::isAlphanumeric('_'));
    zassert_false(qrCode::isAlphanumeric('#'));
    zassert_false(qrCode::isAlphanumeric('^'));
    zassert_false(qrCode::isAlphanumeric('('));
    zassert_false(qrCode::isAlphanumeric('{'));
    // null-terminated string
    zassert_true(qrCode::isAlphanumeric("0123456789ABCDEF"));
    zassert_true(qrCode::isAlphanumeric("HTTPS://WWW.STROOOM.BE"));
    zassert_false(qrCode::isAlphanumeric("HTTPS://WWW.STROOOM.BE?"));
    // byte array with length
    const uint8_t testData1[13]{48, 57, 65, 90, 32, 36, 37, 42, 43, 45, 46, 47, 58};
    zassert_true(qrCode::isAlphanumeric(testData1, 13));
    const uint8_t testData2[2]{35, 40};
    zassert_false(qrCode::isAlphanumeric(testData2, 2));
}

ZTEST(qrcode_suite, test_getEncodingFormat) {
    zassert_equal(encodingFormat::numeric, qrCode::getEncodingFormat("0123456789"));
    zassert_equal(encodingFormat::alphanumeric, qrCode::getEncodingFormat("HTTPS://WWW.STROOOM.BE"));
    zassert_equal(encodingFormat::byte, qrCode::getEncodingFormat("HTTPS://WWW.STROOOM.BE?"));

    const uint8_t testData1[10]{48, 49, 50, 51, 52, 53, 54, 55, 56, 57};
    zassert_equal(encodingFormat::numeric, qrCode::getEncodingFormat(testData1, 10));
    const uint8_t testData2[13]{48, 57, 65, 90, 32, 36, 37, 42, 43, 45, 46, 47, 58};
    zassert_equal(encodingFormat::alphanumeric, qrCode::getEncodingFormat(testData2, 13));
    const uint8_t testData3[2]{35, 40};
    zassert_equal(encodingFormat::byte, qrCode::getEncodingFormat(testData3, 2));
}

ZTEST(qrcode_suite, test_payloadLengthInBits) {
    zassert_equal(41, qrCode::payloadLengthInBits(8, 1, encodingFormat::numeric));
    zassert_equal(57, qrCode::payloadLengthInBits(8, 1, encodingFormat::alphanumeric));
    zassert_equal(76, qrCode::payloadLengthInBits(8, 1, encodingFormat::byte));

    zassert_equal(151, qrCode::payloadLengthInBits(41, 1, encodingFormat::numeric));
    zassert_equal(151, qrCode::payloadLengthInBits(25, 1, encodingFormat::alphanumeric));
    zassert_equal(148, qrCode::payloadLengthInBits(17, 1, encodingFormat::byte));
}

ZTEST(qrcode_suite, test_characterCountIndicatorLength) {
    zassert_equal(10, qrCode::characterCountIndicatorLength(1, encodingFormat::numeric));
    zassert_equal(9, qrCode::characterCountIndicatorLength(1, encodingFormat::alphanumeric));
    zassert_equal(8, qrCode::characterCountIndicatorLength(1, encodingFormat::byte));

    zassert_equal(12, qrCode::characterCountIndicatorLength(10, encodingFormat::numeric));
    zassert_equal(11, qrCode::characterCountIndicatorLength(10, encodingFormat::alphanumeric));
    zassert_equal(16, qrCode::characterCountIndicatorLength(10, encodingFormat::byte));

    zassert_equal(14, qrCode::characterCountIndicatorLength(40, encodingFormat::numeric));
    zassert_equal(13, qrCode::characterCountIndicatorLength(40, encodingFormat::alphanumeric));
    zassert_equal(16, qrCode::characterCountIndicatorLength(40, encodingFormat::byte));
}

ZTEST(qrcode_suite, test_compress) {
    zassert_equal(0, qrCode::compressNumeric('0'));
    zassert_equal(1, qrCode::compressNumeric('1'));
    zassert_equal(8, qrCode::compressNumeric('8'));
    zassert_equal(9, qrCode::compressNumeric('9'));

    zassert_equal(0, qrCode::compressAlphanumeric('0'));
    zassert_equal(1, qrCode::compressAlphanumeric('1'));
    zassert_equal(8, qrCode::compressAlphanumeric('8'));
    zassert_equal(9, qrCode::compressAlphanumeric('9'));
    zassert_equal(10, qrCode::compressAlphanumeric('A'));
    zassert_equal(11, qrCode::compressAlphanumeric('B'));
    zassert_equal(34, qrCode::compressAlphanumeric('Y'));
    zassert_equal(35, qrCode::compressAlphanumeric('Z'));
    zassert_equal(36, qrCode::compressAlphanumeric(' '));
    zassert_equal(37, qrCode::compressAlphanumeric('$'));
    zassert_equal(38, qrCode::compressAlphanumeric('%'));
    zassert_equal(39, qrCode::compressAlphanumeric('*'));
    zassert_equal(40, qrCode::compressAlphanumeric('+'));
    zassert_equal(41, qrCode::compressAlphanumeric('-'));
    zassert_equal(42, qrCode::compressAlphanumeric('.'));
    zassert_equal(43, qrCode::compressAlphanumeric('/'));
    zassert_equal(44, qrCode::compressAlphanumeric(':'));
}

ZTEST(qrcode_suite, test_encodeData) {
    // used https://www.nayuki.io/page/creating-a-qr-code-step-by-step to create test-vectors

    // 1. Numeric encoding mode
    {
        qrCode::initialize(1, errorCorrectionLevel::low);
        qrCode::encodeData("0123456789");

        bitVector<200U> expected;
        expected.reset();
        expected.appendBits(0b0001, 4);                                      // set mode to numeric - always 4 bits
        expected.appendBits(0b0000001010, 10);                               // count - 10 bits
        expected.appendBits(0b00000011000101011001101010011010, 32);         // data 1/2
        expected.appendBits(0b0000001100010101100110101001101001, 2);        // data 2/2
        expected.appendBits(0b0000, 4);                                      // terminator
        expected.appendBits(0b0000, 4);                                      // bit padding
        expected.appendBits(0b11101100000100011110110000010001, 32);         // byte padding
        expected.appendBits(0b11101100000100011110110000010001, 32);         // byte padding
        expected.appendBits(0b11101100000100011110110000010001, 32);         // byte padding

        zassert_equal(152U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    {
        // 2. Alphanumeric encoding mode

        qrCode::initialize(1, errorCorrectionLevel::low);
        qrCode::encodeData("PROJECT NAYUKI");

        bitVector<200U> expected;
        expected.reset();
        expected.appendBits(0b0010, 4);                                     // set mode to alfa-numeric - always 4 bits
        expected.appendBits(0b000001110, 9);                                // count - 9 bits
        expected.appendBits(0b10010000000100010010110101000001, 32);        // data 1/3
        expected.appendBits(0b01010011110110000010101110000110, 32);        // data 2/3
        expected.appendBits(0b0001110010110, 13);                           // data 3/3

        expected.appendBits(0b0000, 4);                                     // terminator
        expected.appendBits(0b00, 2);                                       // bit padding
        expected.appendBits(0b11101100000100011110110000010001, 32);        // byte padding
        expected.appendBits(0b111011000001000111101100, 24);                // byte padding

        zassert_equal(152U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    {
        // 3. Byte encoding mode

        qrCode::initialize(2, errorCorrectionLevel::low);
        qrCode::encodeData("https://github.com/Strooom");

        bitVector<300U> expected;
        expected.reset();
        expected.appendBits(0b0100, 4);                                     // set mode to byte - always 4 bits
        expected.appendBits(0b00011010, 8);                                 // count - 8 bits
        expected.appendBits(0b01101000011101000111010001110000, 32);        // data 1/7
        expected.appendBits(0b01110011001110100010111100101111, 32);        // data 2/7
        expected.appendBits(0b01100111011010010111010001101000, 32);        // data 3/7
        expected.appendBits(0b01110101011000100010111001100011, 32);        // data 4/7
        expected.appendBits(0b01101111011011010010111101010011, 32);        // data 5/7
        expected.appendBits(0b01110100011100100110111101101111, 32);        // data 6/7
        expected.appendBits(0b0110111101101101, 16);                        // data 7/7

        expected.appendBits(0b0000, 4);                                     // terminator
        expected.appendBits(0b11101100000100011110110000010001, 32);        // byte padding
        expected.appendBits(0b1110110000010001, 16);                        // byte padding

        zassert_equal(272U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    {
        qrCode::initialize(5, errorCorrectionLevel::quartile);
        qrCode::encodeData("https://en.wikipedia.org/wiki/QR_code#Error_correction");
        zassert_equal(62U, qrCode::buffer.levelInBytes());
    }

    // Edge Cases Tests : Bit padding bits 0..7
    // 0 bits padded
    {
        qrCode::initialize(1, errorCorrectionLevel::high);
        qrCode::encodeData("0123");

        bitVector<200U> expected;
        expected.reset();
        expected.appendBits(0b0001, 4);                                     // set mode to numeric - always 4 bits
        expected.appendBits(0b0000000100, 10);                              // count - 10 bits
        expected.appendBits(0b00000011000011, 14);                          // data
        expected.appendBits(0b0000, 4);                                     // terminator
        expected.appendBits(0b0, 0);                                        // bit padding : 0 (zero) bit
        expected.appendBits(0b11101100000100011110110000010001, 32);        // byte padding
        expected.appendBits(0b11101100, 8);                                 // byte padding

        zassert_equal(72U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    // 1 bits padded
    {
        qrCode::initialize(1, errorCorrectionLevel::high);
        qrCode::encodeData("A");

        bitVector<200U> expected;
        expected.reset();
        expected.appendBits(0b0010, 4);                                     // set mode to alfanumeric - always 4 bits
        expected.appendBits(0b000000001, 9);                                // count - 9 bits
        expected.appendBits(0b001010, 6);                                   // data
        expected.appendBits(0b0000, 4);                                     // terminator
        expected.appendBits(0b0, 1);                                        // bit padding : 1 bit
        expected.appendBits(0b11101100000100011110110000010001, 32);        // byte padding
        expected.appendBits(0b1110110000010001, 16);                        // byte padding

        zassert_equal(72U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    // 6 bits padded
    {
        qrCode::initialize(1, errorCorrectionLevel::high);
        qrCode::encodeData("0123456");

        bitVector<200U> expected;
        expected.reset();
        expected.appendBits(0b0001, 4);                             // set mode to numeric - always 4 bits
        expected.appendBits(0b0000000111, 10);                      // count - 10 bits
        expected.appendBits(0b000000110001010110010110, 24);        // data
        expected.appendBits(0b0000, 4);                             // terminator
        expected.appendBits(0b000000, 6);                           // bit padding : 6 bits
        expected.appendBits(0b111011000001000111101100, 24);        // byte padding

        zassert_equal(72U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    // 7 bits padded
    {
        qrCode::initialize(1, errorCorrectionLevel::high);
        qrCode::encodeData("01");

        bitVector<200U> expected;
        expected.reset();
        expected.appendBits(0b0001, 4);                                     // set mode to numeric - always 4 bits
        expected.appendBits(0b0000000010, 10);                              // count - 10 bits
        expected.appendBits(0b0000001, 7);                                  // data
        expected.appendBits(0b0000, 4);                                     // terminator
        expected.appendBits(0b0000000, 7);                                  // bit padding : 6 bits
        expected.appendBits(0b11101100000100011110110000010001, 32);        // byte padding
        expected.appendBits(0b11101100, 8);                                 // byte padding

        zassert_equal(72U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    // 1 bit terminator
    {
        qrCode::initialize(1, errorCorrectionLevel::high);
        qrCode::encodeData("01234567890123456");

        bitVector<200U> expected;
        expected.reset();
        expected.appendBits(0b0001, 4);                                     // set mode to numeric - always 4 bits
        expected.appendBits(0b0000010001, 10);                              // count - 10 bits
        expected.appendBits(0b00000011000101011001101010011011, 32);        // data
        expected.appendBits(0b1000010100111010100111000, 25);               // data
        expected.appendBits(0b0, 1);                                        // terminator, 1-bit only

        zassert_equal(72U, qrCode::buffer.levelInBits());
        zassert_mem_equal(expected.data, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
}
#pragma endregion
#pragma region testing error correction

ZTEST(qrcode_suite, test_nmbrOfErrorCorrectionModules) {
    zassert_equal(7 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(1, errorCorrectionLevel::low));
    zassert_equal(10 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(1, errorCorrectionLevel::medium));
    zassert_equal(13 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(1, errorCorrectionLevel::quartile));
    zassert_equal(17 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(1, errorCorrectionLevel::high));

    zassert_equal(10 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(2, errorCorrectionLevel::low));
    zassert_equal(16 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(2, errorCorrectionLevel::medium));
    zassert_equal(22 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(2, errorCorrectionLevel::quartile));
    zassert_equal(28 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(2, errorCorrectionLevel::high));

    zassert_equal(15 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(3, errorCorrectionLevel::low));
    zassert_equal(26 * 1 * 8, qrCode::nmbrOfErrorCorrectionModules(3, errorCorrectionLevel::medium));
    zassert_equal(18 * 2 * 8, qrCode::nmbrOfErrorCorrectionModules(3, errorCorrectionLevel::quartile));
    zassert_equal(22 * 2 * 8, qrCode::nmbrOfErrorCorrectionModules(3, errorCorrectionLevel::high));
}

ZTEST(qrcode_suite, test_blockCalculations) {
    zassert_equal(19, qrCode::blockLengthGroup1(1, errorCorrectionLevel::low));
    zassert_equal(1, qrCode::nmbrBlocksGroup1(1, errorCorrectionLevel::low));
    zassert_equal(20, qrCode::blockLengthGroup2(1, errorCorrectionLevel::low));
    zassert_equal(0, qrCode::nmbrBlocksGroup2(1, errorCorrectionLevel::low));

    zassert_equal(13, qrCode::blockLengthGroup1(3, errorCorrectionLevel::high));
    zassert_equal(2, qrCode::nmbrBlocksGroup1(3, errorCorrectionLevel::high));
    zassert_equal(14, qrCode::blockLengthGroup2(3, errorCorrectionLevel::high));
    zassert_equal(0, qrCode::nmbrBlocksGroup2(3, errorCorrectionLevel::high));

    zassert_equal(9, qrCode::blockLengthGroup1(4, errorCorrectionLevel::high));
    zassert_equal(4, qrCode::nmbrBlocksGroup1(4, errorCorrectionLevel::high));
    zassert_equal(10, qrCode::blockLengthGroup2(4, errorCorrectionLevel::high));
    zassert_equal(0, qrCode::nmbrBlocksGroup2(4, errorCorrectionLevel::high));

    zassert_equal(15, qrCode::blockLengthGroup1(5, errorCorrectionLevel::quartile));
    zassert_equal(2, qrCode::nmbrBlocksGroup1(5, errorCorrectionLevel::quartile));
    zassert_equal(16, qrCode::blockLengthGroup2(5, errorCorrectionLevel::quartile));
    zassert_equal(2, qrCode::nmbrBlocksGroup2(5, errorCorrectionLevel::quartile));

    zassert_equal(11, qrCode::blockLengthGroup1(5, errorCorrectionLevel::high));
    zassert_equal(2, qrCode::nmbrBlocksGroup1(5, errorCorrectionLevel::high));
    zassert_equal(12, qrCode::blockLengthGroup2(5, errorCorrectionLevel::high));
    zassert_equal(2, qrCode::nmbrBlocksGroup2(5, errorCorrectionLevel::high));
}

ZTEST(qrcode_suite, test_blockContents) {
    {
        // Test-vector from https://dev.to/maxart2501/let-s-develop-a-qr-code-generator-part-viii-different-sizes-1e0e#codeword-blocks
        qrCode::initialize(5, errorCorrectionLevel::quartile);
        qrCode::encodeData("https://en.wikipedia.org/wiki/QR_code#Error_correction");
        zassert_equal(62U, qrCode::buffer.levelInBytes());

        uint8_t expectedDataBlock0[15] = {67, 102, 135, 71, 71, 7, 51, 162, 242, 246, 86, 226, 231, 118, 150};
        uint8_t expectedDataBlock1[15] = {182, 151, 6, 86, 70, 150, 18, 230, 247, 38, 114, 247, 118, 150, 182};
        uint8_t expectedDataBlock2[16] = {146, 245, 21, 37, 246, 54, 246, 70, 82, 52, 87, 39, 38, 247, 37, 246};
        uint8_t expectedDataBlock3[16] = {54, 247, 39, 38, 86, 55, 70, 150, 246, 224, 236, 17, 236, 17, 236, 17};

        zassert_mem_equal(expectedDataBlock0, qrCode::buffer.data + qrCode::blockOffset(0, 5, errorCorrectionLevel::quartile), qrCode::blockLength(0, 5, errorCorrectionLevel::quartile));
        zassert_mem_equal(expectedDataBlock1, qrCode::buffer.data + qrCode::blockOffset(1, 5, errorCorrectionLevel::quartile), qrCode::blockLength(1, 5, errorCorrectionLevel::quartile));
        zassert_mem_equal(expectedDataBlock2, qrCode::buffer.data + qrCode::blockOffset(2, 5, errorCorrectionLevel::quartile), qrCode::blockLength(2, 5, errorCorrectionLevel::quartile));
        zassert_mem_equal(expectedDataBlock3, qrCode::buffer.data + qrCode::blockOffset(3, 5, errorCorrectionLevel::quartile), qrCode::blockLength(3, 5, errorCorrectionLevel::quartile));
    }
}

ZTEST(qrcode_suite, test_getErrorCorrectionBytes) {
    {
        uint8_t source[15] = {67, 102, 135, 71, 71, 7, 51, 162, 242, 246, 86, 226, 231, 118, 150};
        uint8_t ecc[18];
        reedSolomon::getErrorCorrectionBytes(ecc, 18, source, 15);
        uint8_t expectedEcc[18] = {0x9A, 0x96, 0x89, 0xE1, 0xF5, 0xB0, 0xDE, 0x9F, 0x29, 0x87, 0x70, 0x7D, 0x4B, 0xE0, 0x88, 0x1F, 0x85, 0xF9};
        zassert_mem_equal(expectedEcc, ecc, 18);
    }
    {
        uint8_t source[15] = {182, 151, 6, 86, 70, 150, 18, 230, 247, 38, 114, 247, 118, 150, 182};
        uint8_t ecc[18];
        reedSolomon::getErrorCorrectionBytes(ecc, 18, source, 15);
        uint8_t expectedEcc[18] = {0xDB, 0x64, 0x3A, 0x56, 0x2F, 0xBF, 0x01, 0x06, 0x79, 0x34, 0xDB, 0xC0, 0xAE, 0x0D, 0x4E, 0x6E, 0x0F, 0xC9};
        zassert_mem_equal(expectedEcc, ecc, 18);
    }
    {
        uint8_t source[16] = {146, 245, 21, 37, 246, 54, 246, 70, 82, 52, 87, 39, 38, 247, 37, 246};
        uint8_t ecc[18];
        reedSolomon::getErrorCorrectionBytes(ecc, 18, source, 16);
        uint8_t expectedEcc[18] = {0xA3, 0xE1, 0xDD, 0x45, 0xC3, 0x39, 0x8B, 0x1D, 0x42, 0xE7, 0x31, 0x5E, 0xA5, 0xE8, 0x24, 0x32, 0xBF, 0xAB};
        zassert_mem_equal(expectedEcc, ecc, 18);
    }
    {
        uint8_t source[16] = {54, 247, 39, 38, 86, 55, 70, 150, 246, 224, 236, 17, 236, 17, 236, 17};
        uint8_t ecc[18];
        reedSolomon::getErrorCorrectionBytes(ecc, 18, source, 16);
        uint8_t expectedEcc[18] = {0xF3, 0x6A, 0x27, 0x26, 0xB2, 0x7B, 0xBD, 0x43, 0xB4, 0x1A, 0xC8, 0xD4, 0xFE, 0xDB, 0xDD, 0x83, 0x9D, 0x61};
        zassert_mem_equal(expectedEcc, ecc, 18);
    }
}

ZTEST(qrcode_suite, test_addErrorCorrectionBytes) {
    {
        qrCode::initialize(1, errorCorrectionLevel::low);
        qrCode::encodeData("0123456789");
        zassert_equal(19U, qrCode::buffer.levelInBytes());
        qrCode::addErrorCorrection();
        zassert_equal(26U, qrCode::buffer.levelInBytes());
        uint8_t expectedData[26] = {0x10, 0x28, 0x0C, 0x56, 0x6A, 0x69, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0x27, 0xAA, 0x26, 0x08, 0xEB, 0xFF, 0xD6};
        zassert_mem_equal(expectedData, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    {
        qrCode::initialize(1, errorCorrectionLevel::medium);
        qrCode::encodeData("0123456789");
        zassert_equal(16U, qrCode::buffer.levelInBytes());
        qrCode::addErrorCorrection();
        zassert_equal(26U, qrCode::buffer.levelInBytes());
        uint8_t expectedData[26] = {0x10, 0x28, 0x0C, 0x56, 0x6A, 0x69, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x2D, 0xB7, 0x27, 0xFB, 0xF5, 0xD3, 0xAA, 0x8F, 0xD9, 0x11};
        zassert_mem_equal(expectedData, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    {
        qrCode::initialize(1, errorCorrectionLevel::high);
        qrCode::encodeData("0123456789");
        zassert_equal(9U, qrCode::buffer.levelInBytes());
        qrCode::addErrorCorrection();
        zassert_equal(26U, qrCode::buffer.levelInBytes());
        uint8_t expectedData[26] = {0x10, 0x28, 0x0C, 0x56, 0x6A, 0x69, 0x00, 0xEC, 0x11, 0xE5, 0x3D, 0x0A, 0xBB, 0xA0, 0x26, 0x0A, 0x07, 0xA7, 0x8A, 0x51, 0xCD, 0xDD, 0x96, 0xB1, 0x68, 0x5D};
        zassert_mem_equal(expectedData, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
    {
        qrCode::initialize(5, errorCorrectionLevel::quartile);
        qrCode::encodeData("https://en.wikipedia.org/wiki/QR_code#Error_correction");
        zassert_equal(62U, qrCode::buffer.levelInBytes());
        qrCode::addErrorCorrection();
        zassert_equal(134U, qrCode::buffer.levelInBytes());
        uint8_t expectedData[134] = {
            67, 102, 135, 71, 71, 7, 51, 162, 242, 246, 86, 226, 231, 118, 150,
            182, 151, 6, 86, 70, 150, 18, 230, 247, 38, 114, 247, 118, 150, 182,
            146, 245, 21, 37, 246, 54, 246, 70, 82, 52, 87, 39, 38, 247, 37, 246,
            54, 247, 39, 38, 86, 55, 70, 150, 246, 224, 236, 17, 236, 17, 236, 17,
            0x9A, 0x96, 0x89, 0xE1, 0xF5, 0xB0, 0xDE, 0x9F, 0x29, 0x87, 0x70, 0x7D, 0x4B, 0xE0, 0x88, 0x1F, 0x85, 0xF9,
            0xDB, 0x64, 0x3A, 0x56, 0x2F, 0xBF, 0x01, 0x06, 0x79, 0x34, 0xDB, 0xC0, 0xAE, 0x0D, 0x4E, 0x6E, 0x0F, 0xC9,
            0xA3, 0xE1, 0xDD, 0x45, 0xC3, 0x39, 0x8B, 0x1D, 0x42, 0xE7, 0x31, 0x5E, 0xA5, 0xE8, 0x24, 0x32, 0xBF, 0xAB,
            0xF3, 0x6A, 0x27, 0x26, 0xB2, 0x7B, 0xBD, 0x43, 0xB4, 0x1A, 0xC8, 0xD4, 0xFE, 0xDB, 0xDD, 0x83, 0x9D, 0x61};
        zassert_mem_equal(expectedData, qrCode::buffer.data, qrCode::buffer.levelInBytes());
    }
}

ZTEST(qrcode_suite, test_getDataOffset) {
    qrCode::initialize(5, errorCorrectionLevel::quartile);
    // This qrCode has 62 data bytes organized in 4 blocks of 15 / 15 / 16 / 16 bytes, each block has 18 ecc bytes

    zassert_equal(0, qrCode::dataOffset(0, 0));
    zassert_equal(15, qrCode::dataOffset(1, 0));
    zassert_equal(30, qrCode::dataOffset(2, 0));
    zassert_equal(46, qrCode::dataOffset(3, 0));

    zassert_equal(14, qrCode::dataOffset(0, 14));
    zassert_equal(29, qrCode::dataOffset(1, 14));
    zassert_equal(45, qrCode::dataOffset(2, 15));
    zassert_equal(61, qrCode::dataOffset(3, 15));
}

ZTEST(qrcode_suite, test_getEccOffset) {
    qrCode::initialize(5, errorCorrectionLevel::quartile);
    // This qrCode has 62 data bytes organized in 4 blocks of 15 / 15 / 16 / 16 bytes, each block has 18 ecc bytes

    zassert_equal(62, qrCode::eccOffset(0, 0));
    zassert_equal(80, qrCode::eccOffset(1, 0));
    zassert_equal(98, qrCode::eccOffset(2, 0));
    zassert_equal(116, qrCode::eccOffset(3, 0));

    zassert_equal(79, qrCode::eccOffset(0, 17));
    zassert_equal(97, qrCode::eccOffset(1, 17));
    zassert_equal(115, qrCode::eccOffset(2, 17));
    zassert_equal(133, qrCode::eccOffset(3, 17));
}

ZTEST(qrcode_suite, test_interleave) {
    {
        qrCode::initialize(5, errorCorrectionLevel::quartile);
        qrCode::encodeData("https://en.wikipedia.org/wiki/QR_code#Error_correction");
        qrCode::addErrorCorrection();
        qrCode::interleaveData();
        // 4 blocks of 15 / 15 / 16 / 16 bytes, each block has 18 ecc bytes
        uint8_t expectedData[134U] = {0x43, 0xB6, 0x92, 0x36, 0x66, 0x97, 0xF5, 0xF7, 0x87, 0x06, 0x15, 0x27, 0x47, 0x56, 0x25, 0x26, 0x47, 0x46, 0xF6, 0x56, 0x07, 0x96, 0x36, 0x37, 0x33, 0x12, 0xF6, 0x46, 0xA2, 0xE6, 0x46, 0x96, 0xF2, 0xF7, 0x52, 0xF6, 0xF6, 0x26, 0x34, 0xE0, 0x56, 0x72, 0x57, 0xEC, 0xE2, 0xF7, 0x27, 0x11, 0xE7, 0x76, 0x26, 0xEC, 0x76, 0x96, 0xF7, 0x11, 0x96, 0xB6, 0x25, 0xEC, 0xF6, 0x11, 0x9A, 0xDB, 0xA3, 0xF3, 0x96, 0x64, 0xE1, 0x6A, 0x89, 0x3A, 0xDD, 0x27, 0xE1, 0x56, 0x45, 0x26, 0xF5, 0x2F, 0xC3, 0xB2, 0xB0, 0xBF, 0x39, 0x7B, 0xDE, 0x01, 0x8B, 0xBD, 0x9F, 0x06, 0x1D, 0x43, 0x29, 0x79, 0x42, 0xB4, 0x87, 0x34, 0xE7, 0x1A, 0x70, 0xDB, 0x31, 0xC8, 0x7D, 0xC0, 0x5E, 0xD4, 0x4B, 0xAE, 0xA5, 0xFE, 0xE0, 0x0D, 0xE8, 0xDB, 0x88, 0x4E, 0x24, 0xDD, 0x1F, 0x6E, 0x32, 0x83, 0x85, 0x0F, 0xBF, 0x9D, 0xF9, 0xC9, 0xAB, 0x61};
        zassert_mem_equal(expectedData, qrCode::buffer.data, 134U);
    }
    {
        qrCode::initialize(4, errorCorrectionLevel::medium);
        qrCode::encodeData("https://en.wikipedia.org/wiki/QR_code#Error_correction");
        qrCode::addErrorCorrection();
        qrCode::interleaveData();
        // 2 blocks of 31 / 31 bytes, each block has 18 ecc bytes
        uint8_t expectedData[100U] = {0x43, 0x15, 0x66, 0x25, 0x87, 0xF6, 0x47, 0x36, 0x47, 0xF6, 0x07, 0x46, 0x33, 0x52, 0xA2, 0x34, 0xF2, 0x57, 0xF6, 0x27, 0x56, 0x26, 0xE2, 0xF7, 0xE7, 0x25, 0x76, 0xF6, 0x96, 0x36, 0xB6, 0xF7, 0x97, 0x27, 0x06, 0x26, 0x56, 0x56, 0x46, 0x37, 0x96, 0x46, 0x12, 0x96, 0xE6, 0xF6, 0xF7, 0xE0, 0x26, 0xEC, 0x72, 0x11, 0xF7, 0xEC, 0x76, 0x11, 0x96, 0xEC, 0xB6, 0x11, 0x92, 0xEC, 0xF5, 0x11, 0x28, 0x42, 0x41, 0xCB, 0x90, 0x4A, 0xF3, 0xD7, 0xA2, 0x85, 0xCA, 0x2F, 0x89, 0x68, 0xDD, 0xC4, 0x22, 0x4B, 0x72, 0x66, 0x54, 0x2B, 0x55, 0xAC, 0x30, 0x98, 0x07, 0x33, 0xE7, 0xAC, 0x67, 0x32, 0x71, 0x88, 0x12, 0x36};
        zassert_mem_equal(expectedData, qrCode::buffer.data, 100U);
    }
}

#pragma endregion
#pragma region testing drawing patterns and user data

ZTEST(qrcode_suite, test_drawAllFindersAndSeparators) {
    static constexpr uint32_t testVersion{2};
    qrCode::initialize(testVersion, errorCorrectionLevel::low);
    qrCode::drawAllFinderPatternsAndSeparators(testVersion);
    static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
    bitVector<testNmbrOfModules> expectedModules;
    bitVector<testNmbrOfModules> expectedIsData;
    zassert_equal(625, testNmbrOfModules);
    zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
    zassert_equal(testNmbrOfModules, expectedModules.length);
    zassert_equal(testNmbrOfModules, expectedIsData.length);

    expectedModules.appendBits(0b1111111000000000001111111, 25);
    expectedModules.appendBits(0b1000001000000000001000001, 25);
    expectedModules.appendBits(0b1011101000000000001011101, 25);
    expectedModules.appendBits(0b1011101000000000001011101, 25);
    expectedModules.appendBits(0b1011101000000000001011101, 25);
    expectedModules.appendBits(0b1000001000000000001000001, 25);
    expectedModules.appendBits(0b1111111000000000001111111, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b1111111000000000000000000, 25);
    expectedModules.appendBits(0b1000001000000000000000000, 25);
    expectedModules.appendBits(0b1011101000000000000000000, 25);
    expectedModules.appendBits(0b1011101000000000000000000, 25);
    expectedModules.appendBits(0b1011101000000000000000000, 25);
    expectedModules.appendBits(0b1000001000000000000000000, 25);
    expectedModules.appendBits(0b1111111000000000000000000, 25);

    zassert_equal(testNmbrOfModules, expectedModules.levelInBits());

    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b0000000011111111100000000, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);
    expectedIsData.appendBits(0b0000000011111111111111111, 25);

    zassert_equal(testNmbrOfModules, expectedIsData.levelInBits());

    for (uint32_t y = 0; y < 25; y++) {
        for (uint32_t x = 0; x < 25; x++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
            zassert_equal(expectedModules.getBit(y * 25 + x), qrCode::modules.getBit(x, y), testFailMessage);
            zassert_equal(expectedIsData.getBit(y * 25 + x), qrCode::isData.getBit(x, y), testFailMessage);
        }
    }
}

ZTEST(qrcode_suite, test_drawDarkModule) {
    static constexpr uint32_t testVersion{2};
    qrCode::initialize(testVersion, errorCorrectionLevel::low);
    qrCode::drawDarkModule(testVersion);
    static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
    bitVector<testNmbrOfModules> expectedModules;
    bitVector<testNmbrOfModules> expectedIsData;
    zassert_equal(625, testNmbrOfModules);
    zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
    zassert_equal(testNmbrOfModules, expectedModules.length);
    zassert_equal(testNmbrOfModules, expectedIsData.length);

    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(000000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000010000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);

    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);

    zassert_equal(testNmbrOfModules, expectedIsData.levelInBits());

    for (uint32_t y = 0; y < 25; y++) {
        for (uint32_t x = 0; x < 25; x++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
            zassert_equal(expectedModules.getBit(y * 25 + x), qrCode::modules.getBit(x, y), testFailMessage);
            zassert_equal(expectedIsData.getBit(y * 25 + x), qrCode::isData.getBit(x, y), testFailMessage);
        }
    }
}

ZTEST(qrcode_suite, test_nmbrOfAlignmentPatterns) {
    zassert_equal(0, qrCode::nmbrOfAlignmentPatterns(1));
    zassert_equal(1, qrCode::nmbrOfAlignmentPatterns(2));
    zassert_equal(1, qrCode::nmbrOfAlignmentPatterns(6));
    zassert_equal(6, qrCode::nmbrOfAlignmentPatterns(7));
    zassert_equal(6, qrCode::nmbrOfAlignmentPatterns(13));
    zassert_equal(13, qrCode::nmbrOfAlignmentPatterns(14));
    zassert_equal(13, qrCode::nmbrOfAlignmentPatterns(20));
    zassert_equal(22, qrCode::nmbrOfAlignmentPatterns(21));
    zassert_equal(22, qrCode::nmbrOfAlignmentPatterns(27));
    zassert_equal(33, qrCode::nmbrOfAlignmentPatterns(28));
    zassert_equal(33, qrCode::nmbrOfAlignmentPatterns(34));
    zassert_equal(46, qrCode::nmbrOfAlignmentPatterns(35));
    zassert_equal(46, qrCode::nmbrOfAlignmentPatterns(40));
}

ZTEST(qrcode_suite, test_alignmentPatternSpacing) {
    // Version 1 has no alignment patterns and so also no spacing
    zassert_equal(18 - 6, qrCode::alignmentPatternSpacing(2));
    zassert_equal(22 - 6, qrCode::alignmentPatternSpacing(3));
    zassert_equal(26 - 6, qrCode::alignmentPatternSpacing(4));
    zassert_equal(30 - 6, qrCode::alignmentPatternSpacing(5));
    zassert_equal(34 - 6, qrCode::alignmentPatternSpacing(6));

    zassert_equal(38 - 22, qrCode::alignmentPatternSpacing(7));
    zassert_equal(42 - 24, qrCode::alignmentPatternSpacing(8));
    zassert_equal(46 - 26, qrCode::alignmentPatternSpacing(9));
    zassert_equal(50 - 28, qrCode::alignmentPatternSpacing(10));
    zassert_equal(54 - 30, qrCode::alignmentPatternSpacing(11));
    zassert_equal(58 - 32, qrCode::alignmentPatternSpacing(12));
    zassert_equal(62 - 34, qrCode::alignmentPatternSpacing(13));

    zassert_equal(66 - 46, qrCode::alignmentPatternSpacing(14));
    zassert_equal(46 - 26, qrCode::alignmentPatternSpacing(14));
    zassert_equal(70 - 48, qrCode::alignmentPatternSpacing(15));
    zassert_equal(48 - 26, qrCode::alignmentPatternSpacing(15));
    zassert_equal(74 - 50, qrCode::alignmentPatternSpacing(16));
    zassert_equal(50 - 26, qrCode::alignmentPatternSpacing(16));
    zassert_equal(78 - 54, qrCode::alignmentPatternSpacing(17));
    zassert_equal(54 - 30, qrCode::alignmentPatternSpacing(17));
    zassert_equal(82 - 56, qrCode::alignmentPatternSpacing(18));
    zassert_equal(56 - 30, qrCode::alignmentPatternSpacing(18));
    zassert_equal(86 - 58, qrCode::alignmentPatternSpacing(19));
    zassert_equal(58 - 30, qrCode::alignmentPatternSpacing(19));
    zassert_equal(90 - 62, qrCode::alignmentPatternSpacing(20));
    zassert_equal(62 - 34, qrCode::alignmentPatternSpacing(20));

    // TODO : complete for higher versions

    zassert_equal(138 - 112, qrCode::alignmentPatternSpacing(32));
    zassert_equal(112 - 86, qrCode::alignmentPatternSpacing(32));
    zassert_equal(86 - 60, qrCode::alignmentPatternSpacing(32));
    zassert_equal(60 - 34, qrCode::alignmentPatternSpacing(32));

    zassert_equal(170 - 142, qrCode::alignmentPatternSpacing(40));
    zassert_equal(142 - 114, qrCode::alignmentPatternSpacing(40));
    zassert_equal(114 - 86, qrCode::alignmentPatternSpacing(40));
    zassert_equal(86 - 58, qrCode::alignmentPatternSpacing(40));
    zassert_equal(58 - 30, qrCode::alignmentPatternSpacing(40));
}

ZTEST(qrcode_suite, test_alignmentPatternCoordinates) {
    zassert_equal(6, qrCode::alignmentPatternCoordinate(2, 0));
    zassert_equal(18, qrCode::alignmentPatternCoordinate(2, 1));
    zassert_equal(6, qrCode::alignmentPatternCoordinate(7, 0));
    zassert_equal(22, qrCode::alignmentPatternCoordinate(7, 1));
    zassert_equal(38, qrCode::alignmentPatternCoordinate(7, 2));
    zassert_equal(6, qrCode::alignmentPatternCoordinate(32, 0));
    zassert_equal(34, qrCode::alignmentPatternCoordinate(32, 1));
    zassert_equal(60, qrCode::alignmentPatternCoordinate(32, 2));
    zassert_equal(86, qrCode::alignmentPatternCoordinate(32, 3));
    zassert_equal(112, qrCode::alignmentPatternCoordinate(32, 4));
    zassert_equal(138, qrCode::alignmentPatternCoordinate(32, 5));
}

ZTEST(qrcode_suite, test_drawAllAlignmentPatterns) {
    static constexpr uint32_t testVersion{2};
    qrCode::initialize(testVersion, errorCorrectionLevel::low);
    qrCode::drawAllAlignmentPatterns(testVersion);
    static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
    bitVector<testNmbrOfModules> expectedModules;
    bitVector<testNmbrOfModules> expectedIsData;
    zassert_equal(625, testNmbrOfModules);
    zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
    zassert_equal(testNmbrOfModules, expectedModules.length);
    zassert_equal(testNmbrOfModules, expectedIsData.length);

    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(000000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000111110000, 25);
    expectedModules.appendBits(0b0000000000000000100010000, 25);
    expectedModules.appendBits(0b0000000000000000101010000, 25);
    expectedModules.appendBits(0b0000000000000000100010000, 25);
    expectedModules.appendBits(0b0000000000000000111110000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);

    zassert_equal(testNmbrOfModules, expectedModules.levelInBits());

    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111000001111, 25);
    expectedIsData.appendBits(0b1111111111111111000001111, 25);
    expectedIsData.appendBits(0b1111111111111111000001111, 25);
    expectedIsData.appendBits(0b1111111111111111000001111, 25);
    expectedIsData.appendBits(0b1111111111111111000001111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);

    zassert_equal(testNmbrOfModules, expectedIsData.levelInBits());

    for (uint32_t y = 0; y < 25; y++) {
        for (uint32_t x = 0; x < 25; x++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
            zassert_equal(expectedModules.getBit(y * 25 + x), qrCode::modules.getBit(x, y), testFailMessage);
            zassert_equal(expectedIsData.getBit(y * 25 + x), qrCode::isData.getBit(x, y), testFailMessage);
        }
    }
}

ZTEST(qrcode_suite, test_drawTimingPatterns) {
    static constexpr uint32_t testVersion{2};
    qrCode::initialize(testVersion, errorCorrectionLevel::low);
    qrCode::drawTimingPattern(testVersion);
    static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
    bitVector<testNmbrOfModules> expectedModules;
    bitVector<testNmbrOfModules> expectedIsData;
    zassert_equal(625, testNmbrOfModules);
    zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
    zassert_equal(testNmbrOfModules, expectedModules.length);
    zassert_equal(testNmbrOfModules, expectedIsData.length);

    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(000000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000010101010100000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000001000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000001000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000001000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000001000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000001000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);
    expectedModules.appendBits(0b0000000000000000000000000, 25);

    zassert_equal(testNmbrOfModules, expectedModules.levelInBits());

    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111100000000011111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111110111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);

    zassert_equal(testNmbrOfModules, expectedIsData.levelInBits());

    for (uint32_t y = 0; y < 25; y++) {
        for (uint32_t x = 0; x < 25; x++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
            zassert_equal(expectedModules.getBit(y * 25 + x), qrCode::modules.getBit(x, y), testFailMessage);
            zassert_equal(expectedIsData.getBit(y * 25 + x), qrCode::isData.getBit(x, y), testFailMessage);
        }
    }
}

ZTEST(qrcode_suite, test_drawDummyFormatInfo) {
    static constexpr uint32_t testVersion{2};
    qrCode::initialize(testVersion, errorCorrectionLevel::low);
    qrCode::drawDummyFormatBits(testVersion);
    static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
    bitVector<testNmbrOfModules> expectedIsData;
    zassert_equal(625, testNmbrOfModules);
    zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
    zassert_equal(testNmbrOfModules, expectedIsData.length);

    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b0000001001111111100000000, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111111111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);
    expectedIsData.appendBits(0b1111111101111111111111111, 25);

    zassert_equal(testNmbrOfModules, expectedIsData.levelInBits());

    for (uint32_t y = 0; y < 25; y++) {
        for (uint32_t x = 0; x < 25; x++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
            zassert_equal(false, qrCode::modules.getBit(x, y), testFailMessage);
            zassert_equal(expectedIsData.getBit(y * 25 + x), qrCode::isData.getBit(x, y), testFailMessage);
        }
    }
}

ZTEST(qrcode_suite, test_formatInfo) {
    static constexpr uint16_t formatInfoBits[nmbrOfErrorCorrectionLevels][nmbrOfMasks]{
        {0b111011111000100, 0b111001011110011, 0b111110110101010, 0b111100010011101, 0b110011000101111, 0b110001100011000, 0b110110001000001, 0b110100101110110},        // formatInfo bits taken from https://www.thonky.com/qr-code-tutorial/format-version-tables
        {0b101010000010010, 0b101000100100101, 0b101111001111100, 0b101101101001011, 0b100010111111001, 0b100000011001110, 0b100111110010111, 0b100101010100000},
        {0b011010101011111, 0b011000001101000, 0b011111100110001, 0b011101000000110, 0b010010010110100, 0b010000110000011, 0b010111011011010, 0b010101111101101},
        {0b001011010001001, 0b001001110111110, 0b001110011100111, 0b001100111010000, 0b000011101100010, 0b000001001010101, 0b000110100001100, 0b000100000111011}};

    for (uint32_t eccLevelIndex = 0; eccLevelIndex < 4; eccLevelIndex++) {
        for (uint32_t maskTypeIndex = 0; maskTypeIndex < 8; maskTypeIndex++) {
            zassert_equal(formatInfoBits[eccLevelIndex][maskTypeIndex], qrCode::calculateFormatInfo(static_cast<errorCorrectionLevel>(eccLevelIndex), maskTypeIndex));
        }
    }
}

ZTEST(qrcode_suite, test_drawFormatInfo) {
    static constexpr uint32_t testVersion{1};
    static constexpr errorCorrectionLevel testErrorCorrectionLevel{errorCorrectionLevel::low};
    static constexpr uint32_t testMask{4};

    qrCode::initialize(testVersion, testErrorCorrectionLevel);
    zassert_equal(0b110011000101111, qrCode::calculateFormatInfo(testErrorCorrectionLevel, testMask));

    qrCode::drawFormatInfo(testErrorCorrectionLevel, testMask);

    static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
    bitVector<testNmbrOfModules> expectedModules;

    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b110011000000000101111, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);

    for (uint32_t y = 0; y < 21; y++) {
        for (uint32_t x = 0; x < 21; x++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
            zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
        }
    }

    qrCode::initialize(testVersion, testErrorCorrectionLevel);
    static constexpr uint16_t allOnes{0b111111111111111};
    qrCode::drawFormatInfoCopy1(allOnes);
    qrCode::drawFormatInfoCopy2(allOnes);
    expectedModules.reset();
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b111111011000011111111, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000000000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);

    for (uint32_t y = 0; y < 21; y++) {
        for (uint32_t x = 0; x < 21; x++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
            zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
        }
    }
}

// ZTEST(qrcode_suite, test_drawVersionInfo) { TEST_IGNORE_MESSAGE("TODO: implement me"); }

ZTEST(qrcode_suite, test_drawPayload) {
    static constexpr uint32_t testVersion{1};
    qrCode::initialize(testVersion, errorCorrectionLevel::low);

    bitVector<208> expectedBuffer;
    expectedBuffer.appendBits(0b00010000000001000000000011101100, 32);        // https://www.nayuki.io/page/creating-a-qr-code-step-by-step with payload "0"
    expectedBuffer.appendBits(0b00010001111011000001000111101100, 32);
    expectedBuffer.appendBits(0b00010001111011000001000111101100, 32);
    expectedBuffer.appendBits(0b00010001111011000001000111101100, 32);
    expectedBuffer.appendBits(0b00010001111011000001000110000011, 32);
    expectedBuffer.appendBits(0b00000111010001011100001111000011, 32);
    expectedBuffer.appendBits(0b1100110000011001, 16);

    qrCode::encodeData("0");
    qrCode::addErrorCorrection();

    for (uint32_t bitIndex = 0; bitIndex < 208; bitIndex++) {
        char testFailMessage[32];
        snprintf(testFailMessage, 30, "mismatch %d", bitIndex);
        zassert_equal(expectedBuffer.getBit(bitIndex), qrCode::buffer.getBit(bitIndex), testFailMessage);
    }

    qrCode::drawPatterns(testVersion);
    qrCode::drawPayload(testVersion);

    static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
    bitVector<testNmbrOfModules> expectedModules;
    expectedModules.appendBits(0b111111100111001111111, 21);        // https://www.nayuki.io/page/creating-a-qr-code-step-by-step with payload "0"
    expectedModules.appendBits(0b100000100010001000001, 21);
    expectedModules.appendBits(0b101110100111001011101, 21);
    expectedModules.appendBits(0b101110100000001011101, 21);
    expectedModules.appendBits(0b101110100000001011101, 21);
    expectedModules.appendBits(0b100000100101101000001, 21);
    expectedModules.appendBits(0b111111101010101111111, 21);
    expectedModules.appendBits(0b000000000000100000000, 21);
    expectedModules.appendBits(0b000000100101100000000, 21);
    expectedModules.appendBits(0b000011011011011101100, 21);

    expectedModules.appendBits(0b101100100000001000100, 21);
    expectedModules.appendBits(0b010000000001011101100, 21);
    expectedModules.appendBits(0b101111111110000000000, 21);
    expectedModules.appendBits(0b000000001000000000000, 21);

    expectedModules.appendBits(0b111111100001110111010, 21);
    expectedModules.appendBits(0b100000100100100010000, 21);
    expectedModules.appendBits(0b101110100111110111000, 21);
    expectedModules.appendBits(0b101110100101011101100, 21);
    expectedModules.appendBits(0b101110100000001000100, 21);
    expectedModules.appendBits(0b100000100101011101110, 21);
    expectedModules.appendBits(0b111111100100000000000, 21);

    uint32_t bitIndex{0};
    for (int32_t y = 20; y > 0; y--) {
        for (int32_t x = 20; x > 0; x--) {
            char testFailMessage[64];
            snprintf(testFailMessage, 63, "mismatch bit %d, (%d,%d)", bitIndex, x, y);
            zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
            bitIndex++;
        }
    }

    // TODO : also do this for a V2 which has alignment patterns and V7 which has version info
}

ZTEST(qrcode_suite, test_masking) {
    {
        static constexpr uint32_t testVersion{1};
        static constexpr uint32_t maskType{0};
        qrCode::initialize(testVersion, errorCorrectionLevel::low);
        qrCode::applyMask(maskType);
        static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
        bitVector<testNmbrOfModules> expectedModules;
        zassert_equal(441, testNmbrOfModules);
        zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
        zassert_equal(testNmbrOfModules, expectedModules.length);

        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);
        expectedModules.appendBits(0b010101010101010101010, 21);
        expectedModules.appendBits(0b101010101010101010101, 21);

        for (uint32_t y = 0; y < 21; y++) {
            for (uint32_t x = 0; x < 21; x++) {
                char testFailMessage[32];
                snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
                zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
            }
        }
    }
    {
        static constexpr uint32_t testVersion{1};
        static constexpr uint32_t maskType{1};
        qrCode::initialize(testVersion, errorCorrectionLevel::low);
        qrCode::applyMask(maskType);
        static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
        bitVector<testNmbrOfModules> expectedModules;
        zassert_equal(441, testNmbrOfModules);
        zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
        zassert_equal(testNmbrOfModules, expectedModules.length);

        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);
        expectedModules.appendBits(0b000000000000000000000, 21);
        expectedModules.appendBits(0b111111111111111111111, 21);

        for (uint32_t y = 0; y < 21; y++) {
            for (uint32_t x = 0; x < 21; x++) {
                char testFailMessage[32];
                snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
                zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
            }
        }
    }
    {
        static constexpr uint32_t testVersion{1};
        static constexpr uint32_t maskType{2};
        qrCode::initialize(testVersion, errorCorrectionLevel::low);
        qrCode::applyMask(maskType);
        static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
        bitVector<testNmbrOfModules> expectedModules;
        zassert_equal(441, testNmbrOfModules);
        zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
        zassert_equal(testNmbrOfModules, expectedModules.length);

        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);

        for (uint32_t y = 0; y < 21; y++) {
            for (uint32_t x = 0; x < 21; x++) {
                char testFailMessage[32];
                snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
                zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
            }
        }
    }
    {
        static constexpr uint32_t testVersion{1};
        static constexpr uint32_t maskType{3};
        qrCode::initialize(testVersion, errorCorrectionLevel::low);
        qrCode::applyMask(maskType);
        static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
        bitVector<testNmbrOfModules> expectedModules;
        zassert_equal(441, testNmbrOfModules);
        zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
        zassert_equal(testNmbrOfModules, expectedModules.length);

        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b001001001001001001001, 21);
        expectedModules.appendBits(0b010010010010010010010, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b001001001001001001001, 21);
        expectedModules.appendBits(0b010010010010010010010, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b001001001001001001001, 21);
        expectedModules.appendBits(0b010010010010010010010, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b001001001001001001001, 21);
        expectedModules.appendBits(0b010010010010010010010, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b001001001001001001001, 21);
        expectedModules.appendBits(0b010010010010010010010, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b001001001001001001001, 21);
        expectedModules.appendBits(0b010010010010010010010, 21);
        expectedModules.appendBits(0b100100100100100100100, 21);
        expectedModules.appendBits(0b001001001001001001001, 21);
        expectedModules.appendBits(0b010010010010010010010, 21);

        for (uint32_t y = 0; y < 21; y++) {
            for (uint32_t x = 0; x < 21; x++) {
                char testFailMessage[32];
                snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
                zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
            }
        }
    }
    {
        static constexpr uint32_t testVersion{1};
        static constexpr uint32_t maskType{4};
        qrCode::initialize(testVersion, errorCorrectionLevel::low);
        qrCode::applyMask(maskType);
        static constexpr uint32_t testNmbrOfModules{(testVersion * 4 + 17) * (testVersion * 4 + 17)};
        bitVector<testNmbrOfModules> expectedModules;
        zassert_equal(441, testNmbrOfModules);
        zassert_equal(testNmbrOfModules, qrCode::modules.getSizeInBits());
        zassert_equal(testNmbrOfModules, expectedModules.length);

        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b000111000111000111000, 21);
        expectedModules.appendBits(0b111000111000111000111, 21);

        for (uint32_t y = 0; y < 21; y++) {
            for (uint32_t x = 0; x < 21; x++) {
                char testFailMessage[32];
                snprintf(testFailMessage, 30, "mismatch %d,%d", x, y);
                zassert_equal(expectedModules.getBit(y * 21 + x), qrCode::modules.getBit(x, y), testFailMessage);
            }
        }
    }
}

#pragma endregion
#pragma region testing api
ZTEST(qrcode_suite, test_versionNeeded) {
    // numeric data
    const char testData1[]{"01234567890123456789012345678901234567890"};                           // 41 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData1, errorCorrectionLevel::low));             //
    const char testData2[]{"012345678901234567890123456789012345678901"};                          // 42 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData2, errorCorrectionLevel::low));             //
    const char testData3[]{"0123456789012345678901234567890123"};                                  // 34 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData3, errorCorrectionLevel::medium));          //
    const char testData4[]{"01234567890123456789012345678901234"};                                 // 35 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData4, errorCorrectionLevel::medium));          //
    const char testData5[]{"012345678901234567890123456"};                                         // 27 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData5, errorCorrectionLevel::quartile));        //
    const char testData6[]{"0123456789012345678901234567"};                                        // 28 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData6, errorCorrectionLevel::quartile));        //
    const char testData7[]{"01234567890123456"};                                                   // 17 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData7, errorCorrectionLevel::high));            //
    const char testData8[]{"012345678901234567"};                                                  // 18 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData8, errorCorrectionLevel::high));            //

    // alfanumeric data
    const char testData10[]{"A123456789012345678901234"};                                           // 25 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData10, errorCorrectionLevel::low));             //
    const char testData11[]{"A1234567890123456789012345"};                                          // 26 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData11, errorCorrectionLevel::low));             //
    const char testData12[]{"A1234567890123456789"};                                                // 20 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData12, errorCorrectionLevel::medium));          //
    const char testData13[]{"A12345678901234567890"};                                               // 21 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData13, errorCorrectionLevel::medium));          //
    const char testData14[]{"A123456789012345"};                                                    // 16 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData14, errorCorrectionLevel::quartile));        //
    const char testData15[]{"A1234567890123456"};                                                   // 17 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData15, errorCorrectionLevel::quartile));        //
    const char testData16[]{"A123456789"};                                                          // 10 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData16, errorCorrectionLevel::high));            //
    const char testData17[]{"A1234567890"};                                                         // 11 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData17, errorCorrectionLevel::high));            //

    // byte data
    const char testData20[]{"#1234567890123456"};                                                   // 17 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData20, errorCorrectionLevel::low));             //
    const char testData21[]{"#12345678901234567"};                                                  // 18 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData21, errorCorrectionLevel::low));             //
    const char testData22[]{"#1234567890123"};                                                      // 14 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData22, errorCorrectionLevel::medium));          //
    const char testData23[]{"#12345678901234"};                                                     // 15 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData23, errorCorrectionLevel::medium));          //
    const char testData24[]{"#1234567890"};                                                         // 11 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData24, errorCorrectionLevel::quartile));        //
    const char testData25[]{"#12345678901"};                                                        // 12 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData25, errorCorrectionLevel::quartile));        //
    const char testData26[]{"#123456"};                                                             // 7 numeric characters
    zassert_equal(1, qrCode::versionNeeded(testData26, errorCorrectionLevel::high));            //
    const char testData27[]{"#1234567"};                                                            // 8 numeric characters
    zassert_equal(2, qrCode::versionNeeded(testData27, errorCorrectionLevel::high));            //

    // edge case
    const char testData54[55]{"#12345678901234567890123456789012345678901234567890123"};
    zassert_equal(0, qrCode::versionNeeded(testData54, errorCorrectionLevel::high));        // This case still fails, need to investigate in detail why
}

ZTEST(qrcode_suite, test_errorCorrectionPossible) {
    {
        const char testData[]{"01234567890123456"};                                                              // 17 numeric characters
        zassert_equal(errorCorrectionLevel::high, qrCode::errorCorrectionLevelPossible(testData, 1));        //
    }
    {
        const char testData[]{"012345678901234567"};                                                                 // 18 numeric characters
        zassert_equal(errorCorrectionLevel::quartile, qrCode::errorCorrectionLevelPossible(testData, 1));        //
    }
    {
        const char testData[]{"012345678901234567890123456"};                                                        // 27 numeric characters
        zassert_equal(errorCorrectionLevel::quartile, qrCode::errorCorrectionLevelPossible(testData, 1));        //
    }
    {
        const char testData[]{"0123456789012345678901234567"};                                                     // 28 numeric characters
        zassert_equal(errorCorrectionLevel::medium, qrCode::errorCorrectionLevelPossible(testData, 1));        //
    }
    {
        const char testData[]{"0123456789012345678901234567890123"};                                               // 34 numeric characters
        zassert_equal(errorCorrectionLevel::medium, qrCode::errorCorrectionLevelPossible(testData, 1));        //
    }
    {
        const char testData[]{"01234567890123456789012345678901234"};                                           // 35 numeric characters
        zassert_equal(errorCorrectionLevel::low, qrCode::errorCorrectionLevelPossible(testData, 1));        //
    }
}

ZTEST(qrcode_suite, test_generate) {
    static constexpr char testInput[]{"test-driven-design"};
    uint32_t testVersionNeeded = qrCode::versionNeeded(testInput, errorCorrectionLevel::low);
    zassert_equal(2, testVersionNeeded);
    errorCorrectionLevel testErrorCorrectionPossible = qrCode::errorCorrectionLevelPossible(testInput, testVersionNeeded);
    zassert_equal(errorCorrectionLevel::quartile, testErrorCorrectionPossible);
    qrCode::generate(testInput, testVersionNeeded, testErrorCorrectionPossible);
    zassert_equal(2, qrCode::theMask);
    zassert_equal(0b011111100110001, qrCode::calculateFormatInfo(qrCode::theErrorCorrectionLevel, qrCode::theMask));
    {
        bitVector<352> expectedBuffer;
        expectedBuffer.appendBits(0b01000001001001110100011001010111, 32);
        expectedBuffer.appendBits(0b00110111010000101101011001000111, 32);
        expectedBuffer.appendBits(0b00100110100101110110011001010110, 32);
        expectedBuffer.appendBits(0b11100010110101100100011001010111, 32);
        expectedBuffer.appendBits(0b00110110100101100111011011100000, 32);
        expectedBuffer.appendBits(0b11101100000100010000111110010111, 32);
        expectedBuffer.appendBits(0b11101001000001000110001001111111, 32);
        expectedBuffer.appendBits(0b11101111001101000110011001001111, 32);
        expectedBuffer.appendBits(0b00010000001100000000001100101010, 32);
        expectedBuffer.appendBits(0b11011000011110011001000011001000, 32);
        expectedBuffer.appendBits(0b11000110101101001110001000000110, 32);

        for (uint32_t bitIndex = 0; bitIndex < 352; bitIndex++) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch %d", bitIndex);
            zassert_equal(expectedBuffer.getBit(bitIndex), qrCode::buffer.getBit(bitIndex), testFailMessage);
        }
    }

    bitVector<625> expectedModules;
    expectedModules.appendBits(0b1111111011010100101111111, 25);
    expectedModules.appendBits(0b1000001001101111101000001, 25);
    expectedModules.appendBits(0b1011101000010000101011101, 25);
    expectedModules.appendBits(0b1011101000101001001011101, 25);
    expectedModules.appendBits(0b1011101011100100101011101, 25);
    expectedModules.appendBits(0b1000001011000111101000001, 25);
    expectedModules.appendBits(0b1111111010101010101111111, 25);
    expectedModules.appendBits(0b0000000001111101000000000, 25);
    expectedModules.appendBits(0b0111111101000010000110001, 25);
    expectedModules.appendBits(0b1011100001010000100101010, 25);
    expectedModules.appendBits(0b1001011010011001111010111, 25);
    expectedModules.appendBits(0b1010100001110010011100011, 25);
    expectedModules.appendBits(0b0111001111010001111110111, 25);
    expectedModules.appendBits(0b1000100001010010100100000, 25);
    expectedModules.appendBits(0b1000011111010001110101011, 25);
    expectedModules.appendBits(0b1011000111010101100011001, 25);
    expectedModules.appendBits(0b1011111000001111111111111, 25);
    expectedModules.appendBits(0b0000000011111111100010110, 25);
    expectedModules.appendBits(0b1111111010001000101010011, 25);
    expectedModules.appendBits(0b1000001010010010100010000, 25);
    expectedModules.appendBits(0b1011101010001111111111101, 25);
    expectedModules.appendBits(0b1011101010000111001010011, 25);
    expectedModules.appendBits(0b1011101011101110000101001, 25);
    expectedModules.appendBits(0b1000001010101101110010001, 25);
    expectedModules.appendBits(0b1111111001000000011000111, 25);

    for (int32_t y = 24; y >= 0; y--) {
        for (int32_t x = 24; x >= 0; x--) {
            char testFailMessage[32];
            snprintf(testFailMessage, 30, "mismatch (%d,%d)", x, y);
            zassert_equal(expectedModules.getBit(y * 25 + x), qrCode::modules.getBit(x, y), testFailMessage);
        }
    }
}

#pragma endregion

ZTEST(qrcode_suite, test_nmbrOfRawDataModules) {
    zassert_equal(208, qrCode::nmbrOfDataModules(1));
    zassert_equal(359, qrCode::nmbrOfDataModules(2));
    zassert_equal(567, qrCode::nmbrOfDataModules(3));
    zassert_equal(807, qrCode::nmbrOfDataModules(4));
    zassert_equal(1079, qrCode::nmbrOfDataModules(5));
    zassert_equal(1383, qrCode::nmbrOfDataModules(6));
    zassert_equal(1568, qrCode::nmbrOfDataModules(7));
    zassert_equal(1936, qrCode::nmbrOfDataModules(8));
    zassert_equal(2336, qrCode::nmbrOfDataModules(9));
    zassert_equal(2768, qrCode::nmbrOfDataModules(10));
    zassert_equal(3232, qrCode::nmbrOfDataModules(11));
    zassert_equal(3728, qrCode::nmbrOfDataModules(12));
    zassert_equal(4256, qrCode::nmbrOfDataModules(13));
    zassert_equal(4651, qrCode::nmbrOfDataModules(14));
    zassert_equal(5243, qrCode::nmbrOfDataModules(15));
    zassert_equal(5867, qrCode::nmbrOfDataModules(16));
}

ZTEST(qrcode_suite, test_calculatePayloadLength) {
    zassert_equal(41, qrCode::payloadLengthInBits(8, 1, encodingFormat::numeric));
    zassert_equal(57, qrCode::payloadLengthInBits(8, 1, encodingFormat::alphanumeric));
    zassert_equal(76, qrCode::payloadLengthInBits(8, 1, encodingFormat::byte));

    zassert_equal(151, qrCode::payloadLengthInBits(41, 1, encodingFormat::numeric));
    zassert_equal(151, qrCode::payloadLengthInBits(25, 1, encodingFormat::alphanumeric));
    zassert_equal(148, qrCode::payloadLengthInBits(17, 1, encodingFormat::byte));
}

// ZTEST(qrcode_suite, test_versionInfo) {
//     static constexpr uint32_t versionBits[34]{
//         0b000111110010010100,
//         0b001000010110111100,
//         0b001001101010011001,
//         0b001010010011010011,
//         0b001011101111110110,
//         0b001100011101100010,
//         0b001101100001000111,
//         0b001110011000001101,
//         0b001111100100101000,
//         0b010000101101111000,
//         0b010001010001011101,
//         0b010010101000010111,
//         0b010011010100110010,
//         0b010100100110100110,
//         0b010101011010000011,
//         0b0010110100011001001,        // ??
//         0b010111011111101100,
//         0b011000111011000100,
//         0b011001000111100001,
//         0b011010111110101011,
//         0b011011000010001110,
//         0b011100110000011010,
//         0b011101001100111111,
//         0b011110110101110101,
//         0b011111001001010000,
//         0b100000100111010101,
//         0b100001011011110000,
//         0b100010100010111010,
//         0b100011011110011111,
//         0b100100101100001011,
//         0b100101010000101110,
//         0b100110101001100100,
//         0b100111010101000001,
//         0b101000110001101001};        // versionInfo bits taken from https://www.thonky.com/qr-code-tutorial/format-version-tables
// }

ZTEST_SUITE(qrcode_suite, NULL, NULL, NULL, NULL, NULL);
