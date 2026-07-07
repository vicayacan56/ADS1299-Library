#include <stdint.h>
#include <stdio.h>

#include "core/ADS1299_Protocol.h"

class ProtocolFakeHAL : public ADS1299_HAL {
public:
    void begin() override { beginCount++; }
    void end() override { endCount++; }
    void beginTransaction(const ADS1299_SpiConfig& config) override { lastConfig = config; transactionCount++; }
    void endTransaction() override { endTransactionCount++; }
    void csLow() override { csLowCount++; }
    void csHigh() override { csHighCount++; }
    uint8_t spiTransfer(uint8_t data) override { lastTransfer = data; return 0; }
    void delayMicroseconds(uint32_t us) override { delayUsTotal += us; }
    void delayMilliseconds(uint32_t ms) override { delayMsTotal += ms; }
    void setStart(bool high) override { startHigh = high; }
    void setReset(bool high) override { resetHigh = high; }
    void setPwdn(bool high) override { pwdnHigh = high; }
    bool readDrdy() override { return drdyHigh; }

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

int main()
{
    testProtocolSkeleton();

    if (g_failures != 0) {
        printf("protocol tests failed: %d\n", g_failures);
        return 1;
    }

    printf("protocol tests passed\n");
    return 0;
}
