#include "DummyDataGenerator.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_set>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Models.h"

using namespace ddg;
using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;

namespace {

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

} // namespace

// ---------------------------------------------------------------------------
// GenerateSamples
// ---------------------------------------------------------------------------

TEST(GenerateSamplesTest, ReturnsRequestedCountWithSequentialUniqueIds) {
    DummyDataGenerator generator(42);
    const auto samples = generator.GenerateSamples(8);

    ASSERT_EQ(samples.size(), 8u);

    std::unordered_set<std::string> ids;
    for (size_t i = 0; i < samples.size(); ++i) {
        const std::string expectedId = "S-" + std::string(3 - std::to_string(i + 1).size(), '0') + std::to_string(i + 1);
        EXPECT_EQ(samples[i].sampleId, expectedId);
        EXPECT_TRUE(ids.insert(samples[i].sampleId).second) << "sampleId가 중복됨: " << samples[i].sampleId;
    }
}

TEST(GenerateSamplesTest, ZeroCountReturnsEmptyVector) {
    DummyDataGenerator generator(1);
    EXPECT_TRUE(generator.GenerateSamples(0).empty());
}

TEST(GenerateSamplesTest, FieldsAreWithinExpectedRanges) {
    DummyDataGenerator generator(7);
    const auto samples = generator.GenerateSamples(20);

    for (const Sample& s : samples) {
        EXPECT_FALSE(s.name.empty());
        EXPECT_THAT(s.avgProcessTimeMin, AllOf(Ge(0.2), Le(1.0)));
        EXPECT_THAT(s.yield, AllOf(Ge(0.75), Le(0.98)));
        EXPECT_THAT(s.stock, AllOf(Ge(0), Le(1000)));
    }
}

TEST(GenerateSamplesTest, SameSeedProducesIdenticalSamples) {
    DummyDataGenerator generatorA(123);
    DummyDataGenerator generatorB(123);

    const auto samplesA = generatorA.GenerateSamples(10);
    const auto samplesB = generatorB.GenerateSamples(10);

    ASSERT_EQ(samplesA.size(), samplesB.size());
    for (size_t i = 0; i < samplesA.size(); ++i) {
        EXPECT_EQ(samplesA[i].sampleId, samplesB[i].sampleId);
        EXPECT_EQ(samplesA[i].name, samplesB[i].name);
        EXPECT_DOUBLE_EQ(samplesA[i].avgProcessTimeMin, samplesB[i].avgProcessTimeMin);
        EXPECT_DOUBLE_EQ(samplesA[i].yield, samplesB[i].yield);
        EXPECT_EQ(samplesA[i].stock, samplesB[i].stock);
    }
}

// ---------------------------------------------------------------------------
// GenerateOrders
// ---------------------------------------------------------------------------

TEST(GenerateOrdersTest, ThrowsWhenSamplesAreEmpty) {
    DummyDataGenerator generator(1);
    EXPECT_THROW(generator.GenerateOrders(5, {}), std::invalid_argument);
}

TEST(GenerateOrdersTest, ReturnsRequestedCount) {
    DummyDataGenerator generator(1);
    const auto samples = generator.GenerateSamples(5);
    const auto orders = generator.GenerateOrders(30, samples);
    EXPECT_EQ(orders.size(), 30u);
}

TEST(GenerateOrdersTest, ZeroCountReturnsEmptyVectorWhenSamplesExist) {
    DummyDataGenerator generator(1);
    const auto samples = generator.GenerateSamples(3);
    EXPECT_TRUE(generator.GenerateOrders(0, samples).empty());
}

TEST(GenerateOrdersTest, EveryOrderReferencesAnExistingSampleId) {
    DummyDataGenerator generator(9);
    const auto samples = generator.GenerateSamples(5);
    const auto orders = generator.GenerateOrders(50, samples);

    std::unordered_set<std::string> validSampleIds;
    for (const Sample& s : samples) {
        validSampleIds.insert(s.sampleId);
    }

    for (const Order& o : orders) {
        EXPECT_TRUE(validSampleIds.count(o.sampleId) > 0)
            << "존재하지 않는 sampleId를 참조함: " << o.sampleId;
    }
}

TEST(GenerateOrdersTest, FieldsMatchExpectedFormatsAndRanges) {
    DummyDataGenerator generator(3);
    const auto samples = generator.GenerateSamples(4);
    const auto orders = generator.GenerateOrders(15, samples);

    const std::regex orderIdPattern(R"(ORD-\d{8}-\d{4})");
    const std::unordered_set<std::string> validStatuses = {
        "RESERVED", "REJECTED", "PRODUCING", "CONFIRMED", "RELEASE"};

    for (const Order& o : orders) {
        EXPECT_TRUE(std::regex_match(o.orderId, orderIdPattern)) << o.orderId;
        EXPECT_THAT(o.quantity, AllOf(Ge(10), Le(500)));
        EXPECT_TRUE(validStatuses.count(ToString(o.status)) > 0);
        EXPECT_FALSE(o.customerName.empty());
        EXPECT_FALSE(o.orderedAt.empty());
    }
}

TEST(GenerateOrdersTest, SameSeedProducesIdenticalOrders) {
    DummyDataGenerator generatorA(55);
    DummyDataGenerator generatorB(55);

    const auto samplesA = generatorA.GenerateSamples(6);
    const auto samplesB = generatorB.GenerateSamples(6);
    const auto ordersA = generatorA.GenerateOrders(20, samplesA);
    const auto ordersB = generatorB.GenerateOrders(20, samplesB);

    ASSERT_EQ(ordersA.size(), ordersB.size());
    for (size_t i = 0; i < ordersA.size(); ++i) {
        // orderedAt은 호출 시점의 실제 시각(now())에서 파생되므로 초 단위 경계에서
        // 두 호출 간 미세하게 달라질 수 있어 결정성 비교 대상에서 제외한다.
        EXPECT_EQ(ordersA[i].sampleId, ordersB[i].sampleId);
        EXPECT_EQ(ordersA[i].customerName, ordersB[i].customerName);
        EXPECT_EQ(ordersA[i].quantity, ordersB[i].quantity);
        EXPECT_EQ(ordersA[i].status, ordersB[i].status);
    }
}

// ---------------------------------------------------------------------------
// SaveSamplesToJson / SaveOrdersToJson
// ---------------------------------------------------------------------------

class DummyDataGeneratorJsonTest : public ::testing::Test {
protected:
    void SetUp() override {
        tempDir_ = std::filesystem::temp_directory_path() / "ddg_test_output";
        std::filesystem::remove_all(tempDir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDir_);
    }

    std::filesystem::path tempDir_;
};

TEST_F(DummyDataGeneratorJsonTest, SaveSamplesToJson_CreatesNestedDirectoriesAndFile) {
    DummyDataGenerator generator(2);
    const auto samples = generator.GenerateSamples(3);

    const auto path = tempDir_ / "nested" / "samples.json";
    EXPECT_FALSE(std::filesystem::exists(path.parent_path()));

    generator.SaveSamplesToJson(samples, path.string());

    ASSERT_TRUE(std::filesystem::exists(path));
    const std::string content = ReadFile(path);
    EXPECT_NE(content.find("\"count\": 3"), std::string::npos);
    for (const Sample& s : samples) {
        EXPECT_NE(content.find("\"" + s.sampleId + "\""), std::string::npos);
    }
}

TEST_F(DummyDataGeneratorJsonTest, SaveSamplesToJson_EscapesSpecialCharactersInName) {
    DummyDataGenerator generator(2);
    Sample sample;
    sample.sampleId = "S-999";
    sample.name = "특수문자 \"테스트\"\n샘플";
    sample.avgProcessTimeMin = 0.5;
    sample.yield = 0.9;
    sample.stock = 10;

    const auto path = tempDir_ / "samples.json";
    generator.SaveSamplesToJson({sample}, path.string());

    const std::string content = ReadFile(path);
    EXPECT_NE(content.find("\\\"테스트\\\""), std::string::npos);
    EXPECT_NE(content.find("\\n샘플"), std::string::npos);
}

TEST_F(DummyDataGeneratorJsonTest, SaveOrdersToJson_WritesStatusUsingToString) {
    DummyDataGenerator generator(2);
    Order order;
    order.orderId = "ORD-20260715-0001";
    order.sampleId = "S-001";
    order.customerName = "테스트고객";
    order.quantity = 100;
    order.status = OrderStatus::Released;
    order.orderedAt = "2026-07-15T00:00:00";

    const auto path = tempDir_ / "orders.json";
    generator.SaveOrdersToJson({order}, path.string());

    const std::string content = ReadFile(path);
    EXPECT_NE(content.find("\"count\": 1"), std::string::npos);
    EXPECT_NE(content.find("\"status\": \"RELEASE\""), std::string::npos);
    EXPECT_NE(content.find("\"orderId\": \"ORD-20260715-0001\""), std::string::npos);
}

TEST_F(DummyDataGeneratorJsonTest, SaveToJson_OverwritesExistingFileRatherThanAppending) {
    DummyDataGenerator generator(2);
    const auto path = tempDir_ / "samples.json";

    const auto firstBatch = generator.GenerateSamples(5);
    generator.SaveSamplesToJson(firstBatch, path.string());

    const auto secondBatch = generator.GenerateSamples(2);
    generator.SaveSamplesToJson(secondBatch, path.string());

    const std::string content = ReadFile(path);
    EXPECT_NE(content.find("\"count\": 2"), std::string::npos);
    EXPECT_EQ(content.find("\"count\": 5"), std::string::npos);
}
