#include <stdint.h>
#include <stdio.h>

#include "ADS1299_Registers.h"
#include "core/ADS1299_Core.h"

static int g_failures = 0;

#define EXPECT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        ++g_failures; \
    } \
} while (0)

#define EXPECT_EQ(expected, actual) do { \
    const long long expectedValue = (long long)(expected); \
    const long long actualValue = (long long)(actual); \
    if (expectedValue != actualValue) { \
        printf("FAIL %s:%d: expected %lld got %lld\n", __FILE__, __LINE__, expectedValue, actualValue); \
        ++g_failures; \
    } \
} while (0)

static void testDeviceAndFrameHelpers()
{
    EXPECT_EQ(4, ADS1299Core::channelsFromDeviceID(0x1C));
    EXPECT_EQ(6, ADS1299Core::channelsFromDeviceID(0x1D));
    EXPECT_EQ(8, ADS1299Core::channelsFromDeviceID(0x1E));
    EXPECT_EQ(0, ADS1299Core::channelsFromDeviceID(0x00));
    EXPECT_EQ(0, ADS1299Core::channelsFromDeviceID(0x1F));

    EXPECT_EQ(15, ADS1299Core::bytesPerFrame(4));
    EXPECT_EQ(21, ADS1299Core::bytesPerFrame(6));
    EXPECT_EQ(27, ADS1299Core::bytesPerFrame(8));

    const uint8_t plusOne[3] = {0x00, 0x00, 0x01};
    const uint8_t minusOne[3] = {0xFF, 0xFF, 0xFF};
    const uint8_t minNegative[3] = {0x80, 0x00, 0x00};
    const uint8_t maxPositive[3] = {0x7F, 0xFF, 0xFF};

    EXPECT_EQ(1, ADS1299Core::unpack24(plusOne));
    EXPECT_EQ(-1, ADS1299Core::unpack24(minusOne));
    EXPECT_EQ(-8388608, ADS1299Core::unpack24(minNegative));
    EXPECT_EQ(8388607, ADS1299Core::unpack24(maxPositive));
}

static void testStatusAndDecode()
{
    const uint32_t status = 0xC1234F;
    EXPECT_TRUE(ADS1299Core::statusHasSync(status));
    EXPECT_EQ(0x12, ADS1299Core::statusLoffP(status));
    EXPECT_EQ(0x34, ADS1299Core::statusLoffN(status));
    EXPECT_EQ(0x0F, ADS1299Core::statusGPIO(status));
    EXPECT_TRUE(!ADS1299Core::statusHasSync(0xA00000));

    const uint8_t frame[] = {
        0xC0, 0x00, 0x0F,
        0x00, 0x00, 0x01,
        0xFF, 0xFF, 0xFF,
        0x80, 0x00, 0x00,
        0x7F, 0xFF, 0xFF
    };

    uint32_t decodedStatus = 0;
    int32_t channels[ADS1299Core::MAX_CHANNELS] = {99, 99, 99, 99, 99, 99, 99, 99};

    EXPECT_TRUE(ADS1299Core::decodeFrame(frame, 4, decodedStatus, channels, ADS1299Core::MAX_CHANNELS));
    EXPECT_EQ(0xC0000F, decodedStatus);
    EXPECT_EQ(1, channels[0]);
    EXPECT_EQ(-1, channels[1]);
    EXPECT_EQ(-8388608, channels[2]);
    EXPECT_EQ(8388607, channels[3]);
    EXPECT_EQ(0, channels[4]);
    EXPECT_TRUE(!ADS1299Core::decodeFrame(frame, 4, decodedStatus, channels, 3));
}

static void testRegisterAndConfigHelpers()
{
    EXPECT_TRUE(ADS1299Core::validRegisterRange(ADS_REG_ID, 1));
    EXPECT_TRUE(ADS1299Core::validRegisterRange(ADS_REG_ID, ADS_REG_CONFIG4 + 1));
    EXPECT_TRUE(!ADS1299Core::validRegisterRange(ADS_REG_CONFIG4, 2));
    EXPECT_TRUE(!ADS1299Core::validRegisterRange(ADS_REG_ID, 0));

    EXPECT_TRUE(ADS1299Core::isValidChannel(1, 4));
    EXPECT_TRUE(ADS1299Core::isValidChannel(4, 4));
    EXPECT_TRUE(!ADS1299Core::isValidChannel(0, 4));
    EXPECT_TRUE(!ADS1299Core::isValidChannel(5, 4));

    EXPECT_EQ(ADS_REG_CH1SET, ADS1299Core::channelRegisterAddress(1));
    EXPECT_EQ(ADS_REG_CH8SET, ADS1299Core::channelRegisterAddress(8));
    EXPECT_EQ(0x0F, ADS1299Core::clipChannelMask(0xFF, 4));
    EXPECT_EQ(0x3F, ADS1299Core::clipChannelMask(0xFF, 6));
    EXPECT_EQ(0xFF, ADS1299Core::clipChannelMask(0xFF, 8));

    EXPECT_EQ((uint8_t)(ADS_CMD_RREG | ADS_REG_ID), ADS1299Core::readRegisterCommand(ADS_REG_ID));
    EXPECT_EQ((uint8_t)(ADS_CMD_WREG | ADS_REG_CONFIG1), ADS1299Core::writeRegisterCommand(ADS_REG_CONFIG1));

    EXPECT_EQ(0x96, ADS1299Core::withDataRate(0x90, ADS_DR_250));
    EXPECT_EQ((uint8_t)(0x96 | ADS_CFG1_CLK_EN), ADS1299Core::withClockOut(0x96, true));
    EXPECT_EQ((uint8_t)(0xFF & ~ADS_CFG1_CLK_EN), ADS1299Core::withClockOut(0xFF, false));
    EXPECT_EQ((uint8_t)(ADS_CH_DEFAULT_GAIN24() | ADS_CH_PD), ADS1299Core::withChannelPowerDown(ADS_CH_DEFAULT_GAIN24(), true));
    EXPECT_EQ((uint8_t)(0xFF & ~ADS_CH_PD), ADS1299Core::withChannelPowerDown(0xFF, false));
    EXPECT_EQ((uint8_t)((ADS_CH_DEFAULT_GAIN24() & 0x8F) | (ADS_GAIN_6 << 4)), ADS1299Core::withChannelGain(ADS_CH_DEFAULT_GAIN24(), ADS_GAIN_6));
    EXPECT_EQ((uint8_t)((ADS_CH_DEFAULT_GAIN24() & 0xF8) | ADS_MUX_TESTSIG), ADS1299Core::withChannelMux(ADS_CH_DEFAULT_GAIN24(), ADS_MUX_TESTSIG));
    EXPECT_EQ((uint8_t)(ADS_CH_DEFAULT_GAIN24() | ADS_CH_SRB2), ADS1299Core::withSRB2(ADS_CH_DEFAULT_GAIN24(), true));
    EXPECT_EQ(ADS_MISC1_SRB1, ADS1299Core::withSRB1(0x00, true));
    EXPECT_EQ(ADS_CFG3_PD_REFBUF, ADS1299Core::withInternalRef(0x00, true));
    EXPECT_EQ(ADS_CFG3_BIASREF_INT, ADS1299Core::withBiasInternalRef(0x00, true));
    EXPECT_EQ(ADS_CFG3_PD_BIAS, ADS1299Core::withBiasBuffer(0x00, true));
    EXPECT_EQ(ADS_CFG3_BIAS_LOFF_SENS, ADS1299Core::withBiasLoffSense(0x00, true));
    EXPECT_EQ(ADS_CFG3_BIAS_MEAS, ADS1299Core::withBiasMeasure(0x00, true));
    EXPECT_EQ(ADS_CFG4_SINGLE_SHOT, ADS1299Core::withSingleShot(0x00, true));
    EXPECT_EQ(ADS_CFG4_LOFF_COMP_EN, ADS1299Core::withLoffComparators(0x00, true));
}

int main()
{
    testDeviceAndFrameHelpers();
    testStatusAndDecode();
    testRegisterAndConfigHelpers();

    if (g_failures != 0) {
        printf("core tests failed: %d\n", g_failures);
        return 1;
    }

    printf("core tests passed\n");
    return 0;
}
