#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "ADS1299_Registers.h"
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

static void testProtocolSkeleton()
{
    ProtocolFakeHAL hal;
    ADS1299_Protocol protocol(hal);

    EXPECT_TRUE(!protocol.isRDATACActive());
    EXPECT_EQ(0, hal.beginCount);
    EXPECT_EQ(0, hal.transactionCount);
    EXPECT_EQ(0, hal.csLowCount);
    EXPECT_EQ(0, hal.csHighCount);
    EXPECT_EQ(0, hal.delayUsTotal);
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

int main()
{
    testProtocolSkeleton();
    testCommandDispatch();
    testRdatacStateTransitions();

    if (g_failures != 0) {
        printf("protocol tests failed: %d\n", g_failures);
        return 1;
    }

    printf("protocol tests passed\n");
    return 0;
}
