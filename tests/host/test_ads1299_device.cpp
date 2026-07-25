#include <stdint.h>
#include <stdio.h>

#include <deque>
#include <vector>

#include "ADS1299_Registers.h"
#include "core/ADS1299_Device.h"

class DeviceFakeHAL : public ADS1299_HAL {
public:
    void begin() override { beginCount++; }
    void end() override { endCount++; }
    void beginTransaction(const ADS1299_SpiConfig& config) override
    {
        transactionCount++;
        lastConfig = config;
    }
    void endTransaction() override { endTransactionCount++; }
    void csLow() override { csLowCount++; }
    void csHigh() override { csHighCount++; }
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

    void clearTransfers()
    {
        tx.clear();
        csLowCount = 0;
        csHighCount = 0;
        delayUsTotal = 0;
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

static void queueBeginId(DeviceFakeHAL& hal, uint8_t id)
{
    const uint8_t rx[] = {0, 0, 0, 0, 0, id};
    hal.queueRx(rx, sizeof(rx));
}

static void append24(std::vector<uint8_t>& frame, int32_t value)
{
    const uint32_t raw = ((uint32_t)value) & 0x00FFFFFFUL;
    frame.push_back((uint8_t)((raw >> 16) & 0xFF));
    frame.push_back((uint8_t)((raw >> 8) & 0xFF));
    frame.push_back((uint8_t)(raw & 0xFF));
}

static void queueFrame(DeviceFakeHAL& hal,
                       uint8_t channelCount,
                       uint32_t status,
                       const int32_t* samples)
{
    std::vector<uint8_t> frame;
    frame.push_back((uint8_t)((status >> 16) & 0xFF));
    frame.push_back((uint8_t)((status >> 8) & 0xFF));
    frame.push_back((uint8_t)(status & 0xFF));
    for (uint8_t i = 0; i < channelCount; ++i) {
        append24(frame, samples[i]);
    }
    hal.queueRx(frame.data(), frame.size());
}

static void testConstantsMirrorCore()
{
    EXPECT_EQ(ADS1299Core::MIN_CHANNELS, ADS1299_Device::MIN_CHANNELS);
    EXPECT_EQ(ADS1299Core::MAX_CHANNELS, ADS1299_Device::MAX_CHANNELS);
    EXPECT_EQ(ADS1299Core::BYTES_PER_FRAME_MAX, ADS1299_Device::BYTES_PER_FRAME_MAX);
    EXPECT_EQ(4, ADS1299_Device::channelsFromDeviceID(0x3C));
    EXPECT_EQ(6, ADS1299_Device::channelsFromDeviceID(0x3D));
    EXPECT_EQ(8, ADS1299_Device::channelsFromDeviceID(0x3E));
}

static void testBeginDetectsDeviceAndStartsTransaction()
{
    DeviceFakeHAL hal;
    ADS1299_Device device(hal);
    queueBeginId(hal, 0x3C);

    EXPECT_TRUE(device.begin());
    EXPECT_EQ(1, hal.beginCount);
    EXPECT_EQ(1, hal.transactionCount);
    EXPECT_EQ(ADS1299_Device::DEFAULT_SPI_HZ, hal.lastConfig.clockHz);
    EXPECT_EQ((int)ADS1299_SpiBitOrder::MSB_FIRST, (int)hal.lastConfig.bitOrder);
    EXPECT_EQ((int)ADS1299_SpiMode::MODE1, (int)hal.lastConfig.mode);
    EXPECT_EQ(5, hal.delayMsTotal);
    EXPECT_EQ(4, device.channelCount());
    EXPECT_EQ(15, device.bytesPerFrame());
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_RREG | ADS_REG_ID), 0x00, ADS_CMD_NOP));
}

static void testBeginRejectsInvalidId()
{
    DeviceFakeHAL hal;
    ADS1299_Device device(hal);
    queueBeginId(hal, 0x00);

    EXPECT_TRUE(!device.begin());
    EXPECT_EQ(ADS1299_Device::MAX_CHANNELS, device.channelCount());
}

static void testConfigureDefaultsWritesExpectedRegisters()
{
    DeviceFakeHAL hal;
    ADS1299_Device device(hal);
    queueBeginId(hal, 0x3C);
    EXPECT_TRUE(device.begin());
    hal.clearTransfers();

    EXPECT_TRUE(device.configureDefaults());
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_WREG | ADS_REG_CONFIG1), 0x00, ADS1299_Device::kCFG1_Default));
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_WREG | ADS_REG_CONFIG2), 0x00, ADS1299_Device::kCFG2_Default));
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_WREG | ADS_REG_CONFIG3), 0x00, ADS1299_Device::kCFG3_Default));
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_WREG | ADS_REG_CH1SET), 0x00, ADS1299_Device::kCH_Default()));
    EXPECT_TRUE(hal.sawSequence((uint8_t)(ADS_CMD_WREG | ADS_REG_CH4SET), 0x00, ADS1299_Device::kCH_Default()));
    EXPECT_TRUE(!hal.sawSequence((uint8_t)(ADS_CMD_WREG | ADS_REG_CH5SET), 0x00, ADS1299_Device::kCH_Default()));
}

static void testDeviceLevelPinsAndDataReady()
{
    DeviceFakeHAL hal;
    ADS1299_Device device(hal);

    EXPECT_TRUE(device.dataReady());
    hal.drdyHigh = true;
    EXPECT_TRUE(!device.dataReady());

    device.startConversions();
    EXPECT_TRUE(hal.startHigh);
    device.stopConversions();
    EXPECT_TRUE(!hal.startHigh);
    device.resetPulse();
    EXPECT_TRUE(hal.resetHigh);
    EXPECT_TRUE(hal.delayUsTotal >= 30);
    device.powerDown(true);
    EXPECT_TRUE(!hal.pwdnHigh);
    device.powerDown(false);
    EXPECT_TRUE(hal.pwdnHigh);
}

static void testReadFrameThroughProtocol()
{
    DeviceFakeHAL hal;
    ADS1299_Device device(hal);
    queueBeginId(hal, 0x3C);
    EXPECT_TRUE(device.begin());
    device.cmdRDATAC();
    hal.clearTransfers();

    const int32_t samples[] = {1, -1, 0x001234, -0x001234};
    queueFrame(hal, 4, 0xC00000, samples);

    uint32_t status = 0;
    int32_t out[ADS1299_Device::MAX_CHANNELS] = {0};
    EXPECT_TRUE(device.readFrameRDATAC(status, out, ADS1299_Device::MAX_CHANNELS));
    EXPECT_EQ(0xC00000, status);
    for (uint8_t i = 0; i < 4; ++i) {
        EXPECT_EQ(samples[i], out[i]);
    }
}

static void testEndReleasesTransactionAndHal()
{
    DeviceFakeHAL hal;
    ADS1299_Device device(hal);
    queueBeginId(hal, 0x3C);
    EXPECT_TRUE(device.begin());

    device.end();
    EXPECT_EQ(1, hal.endTransactionCount);
    EXPECT_EQ(1, hal.endCount);
}

int main()
{
    testConstantsMirrorCore();
    testBeginDetectsDeviceAndStartsTransaction();
    testBeginRejectsInvalidId();
    testConfigureDefaultsWritesExpectedRegisters();
    testDeviceLevelPinsAndDataReady();
    testReadFrameThroughProtocol();
    testEndReleasesTransactionAndHal();

    if (g_failures != 0) {
        printf("device tests failed: %d\n", g_failures);
        return 1;
    }

    printf("device tests passed\n");
    return 0;
}
