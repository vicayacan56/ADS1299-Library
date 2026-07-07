#include <stdint.h>
#include <stdio.h>

#include <deque>
#include <vector>

#include "ADS1299Plus.h"
#include "ADS1299_Registers.h"

SPIClass SPI;

class FakeHAL : public ADS1299_HAL {
public:
    void begin() override { beginCount++; }
    void end() override { endCount++; }

    void beginTransaction(const ADS1299_SpiConfig& config) override
    {
        transactionCount++;
        lastConfig = config;
    }

    void endTransaction() override { endTransactionCount++; }

    void csLow() override
    {
        csLowCount++;
        csActive = true;
    }

    void csHigh() override
    {
        csHighCount++;
        csActive = false;
    }

    uint8_t spiTransfer(uint8_t data) override
    {
        tx.push_back(data);
        if (rx.empty()) {
            return 0;
        }

        const uint8_t value = rx.front();
        rx.pop_front();
        return value;
    }

    void delayMicroseconds(uint32_t us) override { delayUsTotal += us; }
    void delayMilliseconds(uint32_t ms) override { delayMsTotal += ms; }

    void setStart(bool high) override { startHigh = high; }
    void setReset(bool high) override { resetHigh = high; }
    void setPwdn(bool high) override { pwdnHigh = high; }
    bool readDrdy() override { return drdyHigh; }

    void queueRx(uint8_t value) { rx.push_back(value); }

    void queueRx(const uint8_t* data, size_t n)
    {
        for (size_t i = 0; i < n; ++i) {
            rx.push_back(data[i]);
        }
    }

    bool sawTx(uint8_t value) const
    {
        for (size_t i = 0; i < tx.size(); ++i) {
            if (tx[i] == value) {
                return true;
            }
        }
        return false;
    }

    bool sawSequence(uint8_t a, uint8_t b, uint8_t c) const
    {
        if (tx.size() < 3) {
            return false;
        }

        for (size_t i = 0; i + 2 < tx.size(); ++i) {
            if (tx[i] == a && tx[i + 1] == b && tx[i + 2] == c) {
                return true;
            }
        }
        return false;
    }

    int beginCount = 0;
    int endCount = 0;
    int transactionCount = 0;
    int endTransactionCount = 0;
    int csLowCount = 0;
    int csHighCount = 0;
    uint32_t delayUsTotal = 0;
    uint32_t delayMsTotal = 0;
    bool csActive = false;
    bool startHigh = false;
    bool resetHigh = true;
    bool pwdnHigh = true;
    bool drdyHigh = false;
    ADS1299_SpiConfig lastConfig;
    std::vector<uint8_t> tx;
    std::deque<uint8_t> rx;
};

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

static ADS1299Plus::Pins testPins()
{
    ADS1299Plus::Pins pins = {
        10,
        SCK,
        MOSI,
        MISO,
        7,
        9,
        8,
        ADS1299Plus::ADS_PIN_UNUSED
    };
    return pins;
}

static void testPureHelpers()
{
    EXPECT_EQ(4, ADS1299Plus::channelsFromDeviceID(0x1C));
    EXPECT_EQ(6, ADS1299Plus::channelsFromDeviceID(0x1D));
    EXPECT_EQ(8, ADS1299Plus::channelsFromDeviceID(0x1E));
    EXPECT_EQ(0, ADS1299Plus::channelsFromDeviceID(0x00));

    const uint8_t plusOne[3] = {0x00, 0x00, 0x01};
    const uint8_t minusOne[3] = {0xFF, 0xFF, 0xFF};
    const uint8_t minNegative[3] = {0x80, 0x00, 0x00};
    const uint8_t maxPositive[3] = {0x7F, 0xFF, 0xFF};

    EXPECT_EQ(1, ADS1299Plus::unpack24(plusOne));
    EXPECT_EQ(-1, ADS1299Plus::unpack24(minusOne));
    EXPECT_EQ(-8388608, ADS1299Plus::unpack24(minNegative));
    EXPECT_EQ(8388607, ADS1299Plus::unpack24(maxPositive));

    const uint32_t status = 0xC1234F;
    EXPECT_TRUE(ADS1299Plus::statusHasSync(status));
    EXPECT_EQ(0x12, ADS1299Plus::statusLoffP(status));
    EXPECT_EQ(0x34, ADS1299Plus::statusLoffN(status));
    EXPECT_EQ(0x0F, ADS1299Plus::statusGPIO(status));
    EXPECT_TRUE(!ADS1299Plus::statusHasSync(0xA00000));
}

static void testHalBeginAndRegisterWrite()
{
    FakeHAL hal;
    ADS1299Plus ads(hal, testPins());

    // begin() performs RESET, SDATAC, STOP and then RREG ID.
    const uint8_t beginRx[] = {0, 0, 0, 0, 0, 0x1E};
    hal.queueRx(beginRx, sizeof(beginRx));

    EXPECT_TRUE(ads.begin());
    EXPECT_EQ(8, ads.channelCount());
    EXPECT_EQ(1, hal.beginCount);
    EXPECT_EQ(1, hal.transactionCount);
    EXPECT_EQ(ADS1299_SafeSPI::DEFAULT_SPI_HZ, hal.lastConfig.clockHz);
    EXPECT_EQ((int)ADS1299_SpiBitOrder::MSB_FIRST, (int)hal.lastConfig.bitOrder);
    EXPECT_EQ((int)ADS1299_SpiMode::MODE1, (int)hal.lastConfig.mode);
    EXPECT_TRUE(hal.sawTx(ADS_CMD_RESET));
    EXPECT_TRUE(hal.sawTx(ADS_CMD_SDATAC));
    EXPECT_TRUE(hal.sawTx(ADS_CMD_STOP));
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_RREG | ADS_REG_ID), 0x00, ADS_CMD_NOP));

    EXPECT_TRUE(ads.writeReg(ADS_REG_CONFIG1, ADS1299Plus::kCFG1_Default));
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_WREG | ADS_REG_CONFIG1), 0x00, ADS1299Plus::kCFG1_Default));
}

static void testHalFrameDecode()
{
    FakeHAL hal;
    ADS1299Plus ads(hal, testPins());

    const uint8_t beginRx[] = {0, 0, 0, 0, 0, 0x1E};
    hal.queueRx(beginRx, sizeof(beginRx));
    EXPECT_TRUE(ads.begin());

    ads.cmdRDATAC();

    const uint8_t frame[] = {
        0xC0, 0x00, 0x0F,
        0x00, 0x00, 0x01,
        0xFF, 0xFF, 0xFF,
        0x80, 0x00, 0x00,
        0x7F, 0xFF, 0xFF,
        0x00, 0x00, 0x02,
        0x00, 0x00, 0x03,
        0x00, 0x00, 0x04,
        0x00, 0x00, 0x05
    };
    hal.queueRx(frame, sizeof(frame));

    uint32_t status = 0;
    int32_t channels[ADS1299Plus::MAX_CHANNELS] = {0};

    EXPECT_TRUE(ads.readFrameRDATAC(status, channels, ADS1299Plus::MAX_CHANNELS));
    EXPECT_EQ(0xC0000F, status);
    EXPECT_EQ(1, channels[0]);
    EXPECT_EQ(-1, channels[1]);
    EXPECT_EQ(-8388608, channels[2]);
    EXPECT_EQ(8388607, channels[3]);
    EXPECT_EQ(5, channels[7]);
}

int main()
{
    testPureHelpers();
    testHalBeginAndRegisterWrite();
    testHalFrameDecode();

    if (g_failures != 0) {
        printf("host tests failed: %d\n", g_failures);
        return 1;
    }

    printf("host tests passed\n");
    return 0;
}
