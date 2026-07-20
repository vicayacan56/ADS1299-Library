#include <stdint.h>
#include <stdio.h>
#include <deque>
#include <vector>

#include "ADS1299_Registers.h"
#include "core/ADS1299_Core.h"
#include "core/ADS1299_Protocol.h"

static const int EVENT_CS_LOW = -1;
static const int EVENT_CS_HIGH = -2;
static const int EVENT_TRANSFER_BASE = 0x1000;
static const int EVENT_DELAY_US_BASE = 0x2000;

class ProtocolFakeHAL : public ADS1299_HAL {
public:
    void begin() override { beginCount++; }
    void end() override { endCount++; }
    void beginTransaction(const ADS1299_SpiConfig& config) override { lastConfig = config; transactionCount++; }
    void endTransaction() override { endTransactionCount++; }
    void csLow() override { csLowCount++; events.push_back(EVENT_CS_LOW); }
    void csHigh() override { csHighCount++; events.push_back(EVENT_CS_HIGH); }
    uint8_t spiTransfer(uint8_t data) override
    {
        lastTransfer = data;
        events.push_back(EVENT_TRANSFER_BASE + data);
        tx.push_back(data);
        if (!rx.empty()) {
            const uint8_t value = rx.front();
            rx.pop_front();
            return value;
        }
        return 0;
    }
    void delayMicroseconds(uint32_t us) override
    {
        delayUsTotal += us;
        events.push_back(EVENT_DELAY_US_BASE + (int)us);
    }
    void delayMilliseconds(uint32_t ms) override { delayMsTotal += ms; }
    void setStart(bool high) override { startHigh = high; }
    void setReset(bool high) override { resetHigh = high; }
    void setPwdn(bool high) override { pwdnHigh = high; }
    bool readDrdy() override { return drdyHigh; }

    void clearEvents()
    {
        events.clear();
        csLowCount = 0;
        csHighCount = 0;
        lastTransfer = 0;
        delayUsTotal = 0;
        tx.clear();
    }

    void queueRx(uint8_t value) { rx.push_back(value); }

    void queueRx(const uint8_t* data, size_t n)
    {
        for (size_t i = 0; i < n; ++i) {
            rx.push_back(data[i]);
        }
    }

    int beginCount = 0;
    int endCount = 0;
    int transactionCount = 0;
    int endTransactionCount = 0;
    int csLowCount = 0;
    int csHighCount = 0;
    uint8_t lastTransfer = 0;
    uint32_t delayUsTotal = 0;
    uint32_t delayMsTotal = 0;
    bool startHigh = false;
    bool resetHigh = true;
    bool pwdnHigh = true;
    bool drdyHigh = false;
    ADS1299_SpiConfig lastConfig;
    std::vector<int> events;
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

static void expectCommandWithDelay(ProtocolFakeHAL& hal, uint8_t command, uint32_t delayUs)
{
    EXPECT_EQ(4, (int)hal.events.size());
    if (hal.events.size() == 4) {
        EXPECT_EQ(EVENT_CS_LOW, hal.events[0]);
        EXPECT_EQ(EVENT_TRANSFER_BASE + command, hal.events[1]);
        EXPECT_EQ(EVENT_CS_HIGH, hal.events[2]);
        EXPECT_EQ(EVENT_DELAY_US_BASE + (int)delayUs, hal.events[3]);
    }
    EXPECT_EQ(1, hal.csLowCount);
    EXPECT_EQ(1, hal.csHighCount);
    EXPECT_EQ(command, hal.lastTransfer);
    EXPECT_EQ(delayUs, hal.delayUsTotal);
}

static void expectCommandWithoutDelay(ProtocolFakeHAL& hal, uint8_t command)
{
    EXPECT_EQ(3, (int)hal.events.size());
    if (hal.events.size() == 3) {
        EXPECT_EQ(EVENT_CS_LOW, hal.events[0]);
        EXPECT_EQ(EVENT_TRANSFER_BASE + command, hal.events[1]);
        EXPECT_EQ(EVENT_CS_HIGH, hal.events[2]);
    }
    EXPECT_EQ(1, hal.csLowCount);
    EXPECT_EQ(1, hal.csHighCount);
    EXPECT_EQ(command, hal.lastTransfer);
    EXPECT_EQ(0, hal.delayUsTotal);
}

static void expectTransferSequence(ProtocolFakeHAL& hal, const uint8_t* data, size_t n, uint32_t delayUs)
{
    EXPECT_EQ((int)(n + 3), (int)hal.events.size());
    if (hal.events.size() == n + 3) {
        EXPECT_EQ(EVENT_CS_LOW, hal.events[0]);
        for (size_t i = 0; i < n; ++i) {
            EXPECT_EQ(EVENT_TRANSFER_BASE + data[i], hal.events[i + 1]);
        }
        EXPECT_EQ(EVENT_CS_HIGH, hal.events[n + 1]);
        EXPECT_EQ(EVENT_DELAY_US_BASE + (int)delayUs, hal.events[n + 2]);
    }
    EXPECT_EQ(1, hal.csLowCount);
    EXPECT_EQ(1, hal.csHighCount);
    EXPECT_EQ(delayUs, hal.delayUsTotal);
}

static void append24(std::vector<uint8_t>& frame, int32_t value)
{
    const uint32_t raw = ((uint32_t)value) & 0x00FFFFFFUL;
    frame.push_back((uint8_t)((raw >> 16) & 0xFF));
    frame.push_back((uint8_t)((raw >> 8) & 0xFF));
    frame.push_back((uint8_t)(raw & 0xFF));
}

static void queueFrame(ProtocolFakeHAL& hal,
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

static void expectFrameRead(ProtocolFakeHAL& hal, size_t nbytes)
{
    EXPECT_EQ((int)(nbytes + 2), (int)hal.events.size());
    if (hal.events.size() == nbytes + 2) {
        EXPECT_EQ(EVENT_CS_LOW, hal.events[0]);
        for (size_t i = 0; i < nbytes; ++i) {
            EXPECT_EQ(EVENT_TRANSFER_BASE + ADS_CMD_NOP, hal.events[i + 1]);
        }
        EXPECT_EQ(EVENT_CS_HIGH, hal.events[nbytes + 1]);
    }
    EXPECT_EQ(1, hal.csLowCount);
    EXPECT_EQ(1, hal.csHighCount);
    EXPECT_EQ((int)nbytes, (int)hal.tx.size());
    for (size_t i = 0; i < hal.tx.size(); ++i) {
        EXPECT_EQ(ADS_CMD_NOP, hal.tx[i]);
    }
}

static void expectRdataFrameRead(ProtocolFakeHAL& hal, size_t nbytes)
{
    EXPECT_EQ((int)(nbytes + 5), (int)hal.events.size());
    if (hal.events.size() == nbytes + 5) {
        EXPECT_EQ(EVENT_CS_LOW, hal.events[0]);
        EXPECT_EQ(EVENT_TRANSFER_BASE + ADS_CMD_RDATA, hal.events[1]);
        EXPECT_EQ(EVENT_CS_HIGH, hal.events[2]);
        EXPECT_EQ(EVENT_CS_LOW, hal.events[3]);
        for (size_t i = 0; i < nbytes; ++i) {
            EXPECT_EQ(EVENT_TRANSFER_BASE + ADS_CMD_NOP, hal.events[i + 4]);
        }
        EXPECT_EQ(EVENT_CS_HIGH, hal.events[nbytes + 4]);
    }
    EXPECT_EQ(2, hal.csLowCount);
    EXPECT_EQ(2, hal.csHighCount);
    EXPECT_EQ((int)(nbytes + 1), (int)hal.tx.size());
    if (!hal.tx.empty()) {
        EXPECT_EQ(ADS_CMD_RDATA, hal.tx[0]);
    }
    for (size_t i = 1; i < hal.tx.size(); ++i) {
        EXPECT_EQ(ADS_CMD_NOP, hal.tx[i]);
    }
    EXPECT_EQ(0, hal.delayUsTotal);
}

static void testProtocolSkeleton()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);

    EXPECT_TRUE(protocol.attached());
    EXPECT_TRUE(!protocol.isRDATACActive());
    EXPECT_EQ(0, hal.beginCount);
    EXPECT_EQ(0, hal.transactionCount);
    EXPECT_EQ(0, hal.csLowCount);
    EXPECT_EQ(0, hal.csHighCount);
    EXPECT_EQ(0, hal.delayUsTotal);
}

static void testProtocolAttachability()
{
    ADS1299_Protocol protocol;
    uint8_t regValue = 0xAA;
    uint8_t regData = 0x55;
    uint32_t status = 0;
    int32_t channels[ADS1299Core::MAX_CHANNELS] = {0};

    EXPECT_TRUE(!protocol.attached());
    EXPECT_TRUE(!protocol.isRDATACActive());
    protocol.cmdWakeup();
    protocol.cmdRDATAC();
    protocol.cmdRDATA();
    protocol.cmdReset();
    EXPECT_TRUE(!protocol.isRDATACActive());
    EXPECT_TRUE(!protocol.writeReg(ADS_REG_CONFIG1, 0x96));
    EXPECT_TRUE(!protocol.readReg(ADS_REG_CONFIG1, regValue));
    EXPECT_TRUE(!protocol.writeRegs(ADS_REG_CONFIG1, &regData, 1));
    EXPECT_TRUE(!protocol.readRegs(ADS_REG_CONFIG1, &regData, 1));
    EXPECT_TRUE(!protocol.readFrameRDATAC(ADS1299Core::MIN_CHANNELS, status, channels, ADS1299Core::MIN_CHANNELS));
    EXPECT_TRUE(!protocol.readDataOnDemand(ADS1299Core::MIN_CHANNELS, status, channels, ADS1299Core::MIN_CHANNELS));

    ProtocolFakeHAL hal;
    protocol.attach(hal);
    EXPECT_TRUE(protocol.attached());
    EXPECT_TRUE(!protocol.isRDATACActive());
    protocol.cmdWakeup();
    expectCommandWithDelay(hal, ADS_CMD_WAKEUP, 3);

    hal.clearEvents();
    protocol.cmdRDATAC();
    EXPECT_TRUE(protocol.isRDATACActive());
    expectCommandWithDelay(hal, ADS_CMD_RDATAC, 3);

    ProtocolFakeHAL secondHal;
    protocol.attach(secondHal);
    EXPECT_TRUE(protocol.attached());
    EXPECT_TRUE(!protocol.isRDATACActive());
    protocol.cmdStop();
    expectCommandWithDelay(secondHal, ADS_CMD_STOP, 3);
}

static void testCommandDispatch()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);

    protocol.cmdWakeup();
    expectCommandWithDelay(hal, ADS_CMD_WAKEUP, 3);

    hal.clearEvents();
    protocol.cmdStandby();
    expectCommandWithDelay(hal, ADS_CMD_STANDBY, 3);

    hal.clearEvents();
    protocol.cmdStart();
    expectCommandWithDelay(hal, ADS_CMD_START, 3);

    hal.clearEvents();
    protocol.cmdStop();
    expectCommandWithDelay(hal, ADS_CMD_STOP, 3);

    hal.clearEvents();
    protocol.cmdRDATA();
    expectCommandWithoutDelay(hal, ADS_CMD_RDATA);
    EXPECT_TRUE(!protocol.isRDATACActive());
}

static void testRdatacStateTransitions()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);

    protocol.cmdRDATAC();
    expectCommandWithDelay(hal, ADS_CMD_RDATAC, 3);
    EXPECT_TRUE(protocol.isRDATACActive());

    hal.clearEvents();
    protocol.cmdSDATAC();
    expectCommandWithDelay(hal, ADS_CMD_SDATAC, 3);
    EXPECT_TRUE(!protocol.isRDATACActive());

    hal.clearEvents();
    protocol.cmdRDATAC();
    EXPECT_TRUE(protocol.isRDATACActive());

    hal.clearEvents();
    protocol.cmdReset();
    expectCommandWithDelay(hal, ADS_CMD_RESET, 20);
    EXPECT_TRUE(!protocol.isRDATACActive());
}

static void testSingleRegisterAccess()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);

    EXPECT_TRUE(protocol.writeReg(ADS_REG_CONFIG1, 0x96));
    const uint8_t writeExpected[] = {
        (uint8_t)(ADS_CMD_WREG | ADS_REG_CONFIG1),
        0x00,
        0x96
    };
    expectTransferSequence(hal, writeExpected, sizeof(writeExpected), 3);

    hal.clearEvents();
    hal.queueRx(0x00);
    hal.queueRx(0x00);
    hal.queueRx(0xAB);
    uint8_t value = 0;
    EXPECT_TRUE(protocol.readReg(ADS_REG_ID, value));
    EXPECT_EQ(0xAB, value);
    const uint8_t readExpected[] = {
        (uint8_t)(ADS_CMD_RREG | ADS_REG_ID),
        0x00,
        ADS_CMD_NOP
    };
    expectTransferSequence(hal, readExpected, sizeof(readExpected), 3);
}

static void testBurstRegisterAccess()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);

    const uint8_t writeData[] = {0x96, 0xC0, 0xE8};
    EXPECT_TRUE(protocol.writeRegs(ADS_REG_CONFIG1, writeData, sizeof(writeData)));
    const uint8_t writeExpected[] = {
        (uint8_t)(ADS_CMD_WREG | ADS_REG_CONFIG1),
        0x02,
        0x96,
        0xC0,
        0xE8
    };
    expectTransferSequence(hal, writeExpected, sizeof(writeExpected), 3);

    hal.clearEvents();
    const uint8_t rx[] = {0x00, 0x00, 0x11, 0x22, 0x33};
    hal.queueRx(rx, sizeof(rx));
    uint8_t out[3] = {0};
    EXPECT_TRUE(protocol.readRegs(ADS_REG_CONFIG1, out, sizeof(out)));
    EXPECT_EQ(0x11, out[0]);
    EXPECT_EQ(0x22, out[1]);
    EXPECT_EQ(0x33, out[2]);
    const uint8_t readExpected[] = {
        (uint8_t)(ADS_CMD_RREG | ADS_REG_CONFIG1),
        0x02,
        ADS_CMD_NOP,
        ADS_CMD_NOP,
        ADS_CMD_NOP
    };
    expectTransferSequence(hal, readExpected, sizeof(readExpected), 3);
}

static void testRegisterAccessGuards()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);

    uint8_t value = 0x55;
    EXPECT_TRUE(!protocol.readReg((uint8_t)(ADS_REG_CONFIG4 + 1), value));
    EXPECT_TRUE(!protocol.writeReg((uint8_t)(ADS_REG_CONFIG4 + 1), 0x00));
    EXPECT_TRUE(!protocol.writeRegs(ADS_REG_CONFIG1, nullptr, 1));
    EXPECT_TRUE(!protocol.readRegs(ADS_REG_CONFIG1, nullptr, 1));
    EXPECT_TRUE(!protocol.writeRegs(ADS_REG_CONFIG4, &value, 2));
    EXPECT_EQ(0, (int)hal.events.size());

    protocol.cmdRDATAC();
    hal.clearEvents();
    EXPECT_TRUE(!protocol.readReg(ADS_REG_CONFIG1, value));
    EXPECT_TRUE(!protocol.writeReg(ADS_REG_CONFIG1, 0x96));
    EXPECT_TRUE(!protocol.readRegs(ADS_REG_CONFIG1, &value, 1));
    EXPECT_TRUE(!protocol.writeRegs(ADS_REG_CONFIG1, &value, 1));
    EXPECT_EQ(0, (int)hal.events.size());
}

static void testRdatacFrameTransfer()
{
    const int32_t samples[] = {
        0x000001,
        -1,
        0x007F00,
        -8388608,
        0x000123,
        -2,
        0x001000,
        8388607
    };

    for (uint8_t channels = ADS1299Core::MIN_CHANNELS; channels <= ADS1299Core::MAX_CHANNELS; channels += 2) {
        ProtocolFakeHAL hal;
        ADS1299_Protocol protocol(hal);
        protocol.cmdRDATAC();
        hal.clearEvents();

        uint32_t status = 0;
        int32_t out[ADS1299Core::MAX_CHANNELS] = {0};
        queueFrame(hal, channels, 0xC0A55A, samples);
        EXPECT_TRUE(protocol.readFrameRDATAC(channels, status, out, channels));
        EXPECT_EQ(0xC0A55A, status);
        for (uint8_t i = 0; i < channels; ++i) {
            EXPECT_EQ(samples[i], out[i]);
        }
        expectFrameRead(hal, ADS1299Core::bytesPerFrame(channels));
    }
}

static void testRdatacFrameGuards()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);
    uint32_t status = 0;
    int32_t out[ADS1299Core::MAX_CHANNELS] = {0};

    EXPECT_TRUE(!protocol.readFrameRDATAC(ADS1299Core::MIN_CHANNELS, status, out, ADS1299Core::MIN_CHANNELS));
    EXPECT_EQ(0, (int)hal.events.size());

    protocol.cmdRDATAC();
    hal.clearEvents();
    EXPECT_TRUE(!protocol.readFrameRDATAC(ADS1299Core::MIN_CHANNELS, status, out, ADS1299Core::MIN_CHANNELS - 1));
    EXPECT_TRUE(!protocol.readFrameRDATAC(0, status, out, ADS1299Core::MAX_CHANNELS));
    EXPECT_TRUE(!protocol.readFrameRDATAC((uint8_t)(ADS1299Core::MAX_CHANNELS + 1), status, out, ADS1299Core::MAX_CHANNELS));
    EXPECT_TRUE(!protocol.readFrameRDATAC(ADS1299Core::MIN_CHANNELS, status, nullptr, ADS1299Core::MIN_CHANNELS));
    EXPECT_EQ(0, (int)hal.events.size());

    const int32_t samples[] = {1, 2, 3, 4};
    queueFrame(hal, ADS1299Core::MIN_CHANNELS, 0x00A55A, samples);
    EXPECT_TRUE(!protocol.readFrameRDATAC(ADS1299Core::MIN_CHANNELS, status, out, ADS1299Core::MIN_CHANNELS));
    EXPECT_EQ(0x00A55A, status);
    expectFrameRead(hal, ADS1299Core::bytesPerFrame(ADS1299Core::MIN_CHANNELS));
}

static void testReadDataOnDemandTransfer()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);
    const uint8_t channels = ADS1299Core::MAX_CHANNELS;
    const int32_t samples[] = {
        -1,
        0,
        1,
        0x001234,
        -0x001234,
        0x007FFFFF,
        -8388608,
        0x000077
    };
    uint32_t status = 0;
    int32_t out[ADS1299Core::MAX_CHANNELS] = {0};

    hal.queueRx(0x00);
    queueFrame(hal, channels, 0xC01234, samples);
    EXPECT_TRUE(protocol.readDataOnDemand(channels, status, out, channels));
    EXPECT_EQ(0xC01234, status);
    for (uint8_t i = 0; i < channels; ++i) {
        EXPECT_EQ(samples[i], out[i]);
    }
    expectRdataFrameRead(hal, ADS1299Core::bytesPerFrame(channels));
}

static void testReadDataOnDemandGuards()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);
    uint32_t status = 0;
    int32_t out[ADS1299Core::MAX_CHANNELS] = {0};

    EXPECT_TRUE(!protocol.readDataOnDemand(ADS1299Core::MIN_CHANNELS, status, out, ADS1299Core::MIN_CHANNELS - 1));
    EXPECT_TRUE(!protocol.readDataOnDemand(0, status, out, ADS1299Core::MAX_CHANNELS));
    EXPECT_TRUE(!protocol.readDataOnDemand((uint8_t)(ADS1299Core::MAX_CHANNELS + 1), status, out, ADS1299Core::MAX_CHANNELS));
    EXPECT_TRUE(!protocol.readDataOnDemand(ADS1299Core::MIN_CHANNELS, status, nullptr, ADS1299Core::MIN_CHANNELS));
    EXPECT_EQ(0, (int)hal.events.size());

    protocol.cmdRDATAC();
    hal.clearEvents();
    EXPECT_TRUE(!protocol.readDataOnDemand(ADS1299Core::MIN_CHANNELS, status, out, ADS1299Core::MIN_CHANNELS));
    EXPECT_EQ(0, (int)hal.events.size());
}

int main()
{
    testProtocolSkeleton();
    testProtocolAttachability();
    testCommandDispatch();
    testRdatacStateTransitions();
    testSingleRegisterAccess();
    testBurstRegisterAccess();
    testRegisterAccessGuards();
    testRdatacFrameTransfer();
    testRdatacFrameGuards();
    testReadDataOnDemandTransfer();
    testReadDataOnDemandGuards();

    if (g_failures != 0) {
        printf("protocol tests failed: %d\n", g_failures);
        return 1;
    }

    printf("protocol tests passed\n");
    return 0;
}
