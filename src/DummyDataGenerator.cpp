#include "DummyDataGenerator.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

#include "JsonWriter.h"

namespace ddg {

namespace {

// 시료 이름 후보 (PDF 예시 UI에서 사용된 반도체 시료명을 참고)
const std::array<std::string, 10> kSampleNamePool = {
    "실리콘 웨이퍼-8인치",  "GaN 에피텍셜-4인치",   "SiC 파워기판-6인치",
    "포토레지스트-PR7",     "산화막 웨이퍼-SiO2",   "실리콘 웨이퍼-12인치",
    "GaAs 에피텍셜-3인치",  "질화막 웨이퍼-Si3N4",  "폴리실리콘 기판-6인치",
    "사파이어 기판-2인치",
};

// 고객사 이름 후보 (연구소 / 팹리스 / 대학 연구실)
const std::array<std::string, 10> kCustomerPool = {
    "삼성전자 파운드리", "SK하이닉스", "LG이노텍",       "DB하이텍",
    "한국반도체연구원",  "카이스트 반도체연구실", "서울대 공정연구실",
    "팹리스코리아",      "네오세미텍",       "퀀텀파운드리",
};

// 주문 상태 분포: 정상 흐름(RESERVED/CONFIRMED/PRODUCING/RELEASE)이 대다수,
// REJECTED는 예외적인 흐름이므로 낮은 비중으로 부여한다.
const std::array<OrderStatus, 5> kStatusPool = {
    OrderStatus::Reserved, OrderStatus::Confirmed, OrderStatus::Producing,
    OrderStatus::Released, OrderStatus::Rejected,
};
const std::array<double, 5> kStatusWeights = {0.15, 0.30, 0.15, 0.30, 0.10};

std::string FormatSampleId(int index) {
    std::ostringstream oss;
    oss << "S-" << std::setw(3) << std::setfill('0') << index;
    return oss.str();
}

std::string FormatOrderId(const std::chrono::system_clock::time_point& tp, int sequence) {
    const std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &time);
#else
    localtime_r(&time, &tmBuf);
#endif
    std::ostringstream oss;
    oss << "ORD-" << std::put_time(&tmBuf, "%Y%m%d") << "-" << std::setw(4)
        << std::setfill('0') << sequence;
    return oss.str();
}

std::string FormatIso8601(const std::chrono::system_clock::time_point& tp) {
    const std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &time);
#else
    localtime_r(&time, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

void EnsureParentDirectoryExists(const std::string& path) {
    const std::filesystem::path filePath(path);
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path());
    }
}

} // namespace

DummyDataGenerator::DummyDataGenerator(unsigned seed) : seed_(seed) {}

std::vector<Sample> DummyDataGenerator::GenerateSamples(int count) {
    std::vector<Sample> samples;
    samples.reserve(static_cast<size_t>(count));

    std::mt19937 rng(seed_);
    std::uniform_real_distribution<double> processTimeDist(0.2, 1.0);
    std::uniform_real_distribution<double> yieldDist(0.75, 0.98);
    std::uniform_int_distribution<int> stockDist(0, 1000);
    std::uniform_int_distribution<size_t> namePoolDist(0, kSampleNamePool.size() - 1);

    for (int i = 1; i <= count; ++i) {
        Sample sample;
        sample.sampleId = FormatSampleId(i);
        sample.name = kSampleNamePool[namePoolDist(rng)];
        sample.avgProcessTimeMin = processTimeDist(rng);
        sample.yield = yieldDist(rng);
        sample.stock = stockDist(rng);
        samples.push_back(std::move(sample));
    }
    return samples;
}

std::vector<Order> DummyDataGenerator::GenerateOrders(int count, const std::vector<Sample>& samples) {
    if (samples.empty()) {
        throw std::invalid_argument("주문을 생성하려면 최소 1개 이상의 시료가 필요합니다.");
    }

    std::vector<Order> orders;
    orders.reserve(static_cast<size_t>(count));

    std::mt19937 rng(seed_ + 1);
    std::uniform_int_distribution<size_t> sampleIndexDist(0, samples.size() - 1);
    std::uniform_int_distribution<size_t> customerIndexDist(0, kCustomerPool.size() - 1);
    std::uniform_int_distribution<int> quantityDist(10, 500);
    std::discrete_distribution<size_t> statusDist(kStatusWeights.begin(), kStatusWeights.end());
    std::uniform_int_distribution<int> minutesAgoDist(0, 60 * 24 * 7); // 최근 7일 이내 분산

    const auto now = std::chrono::system_clock::now();

    for (int i = 1; i <= count; ++i) {
        Order order;
        order.orderId = FormatOrderId(now, i);
        order.sampleId = samples[sampleIndexDist(rng)].sampleId;
        order.customerName = kCustomerPool[customerIndexDist(rng)];
        order.quantity = quantityDist(rng);
        order.status = kStatusPool[statusDist(rng)];

        const auto orderedAt = now - std::chrono::minutes(minutesAgoDist(rng));
        order.orderedAt = FormatIso8601(orderedAt);

        orders.push_back(std::move(order));
    }
    return orders;
}

void DummyDataGenerator::SaveSamplesToJson(const std::vector<Sample>& samples, const std::string& path) const {
    EnsureParentDirectoryExists(path);

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"generatedAt\": " << JsonWriter::QuoteString(FormatIso8601(std::chrono::system_clock::now())) << ",\n";
    oss << "  \"count\": " << samples.size() << ",\n";
    oss << "  \"samples\": [\n";
    for (size_t i = 0; i < samples.size(); ++i) {
        const Sample& s = samples[i];
        oss << "    {\n";
        oss << "      \"sampleId\": " << JsonWriter::QuoteString(s.sampleId) << ",\n";
        oss << "      \"name\": " << JsonWriter::QuoteString(s.name) << ",\n";
        oss << "      \"avgProcessTimeMin\": " << JsonWriter::FormatDouble(s.avgProcessTimeMin) << ",\n";
        oss << "      \"yield\": " << JsonWriter::FormatDouble(s.yield) << ",\n";
        oss << "      \"stock\": " << s.stock << "\n";
        oss << "    }" << (i + 1 < samples.size() ? "," : "") << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("파일을 열 수 없습니다: " + path);
    }
    file << oss.str();
}

void DummyDataGenerator::SaveOrdersToJson(const std::vector<Order>& orders, const std::string& path) const {
    EnsureParentDirectoryExists(path);

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"generatedAt\": " << JsonWriter::QuoteString(FormatIso8601(std::chrono::system_clock::now())) << ",\n";
    oss << "  \"count\": " << orders.size() << ",\n";
    oss << "  \"orders\": [\n";
    for (size_t i = 0; i < orders.size(); ++i) {
        const Order& o = orders[i];
        oss << "    {\n";
        oss << "      \"orderId\": " << JsonWriter::QuoteString(o.orderId) << ",\n";
        oss << "      \"sampleId\": " << JsonWriter::QuoteString(o.sampleId) << ",\n";
        oss << "      \"customerName\": " << JsonWriter::QuoteString(o.customerName) << ",\n";
        oss << "      \"quantity\": " << o.quantity << ",\n";
        oss << "      \"status\": " << JsonWriter::QuoteString(ToString(o.status)) << ",\n";
        oss << "      \"orderedAt\": " << JsonWriter::QuoteString(o.orderedAt) << "\n";
        oss << "    }" << (i + 1 < orders.size() ? "," : "") << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("파일을 열 수 없습니다: " + path);
    }
    file << oss.str();
}

} // namespace ddg
