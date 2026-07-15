#pragma once

#include <string>
#include <vector>

#include "Models.h"

namespace ddg {

// 반도체 시료 생산주문관리 시스템(S-Semi)을 테스트하기 위한 더미 데이터 생성기.
// 생성된 Sample / Order는 sampleId로 서로 참조되어, JSON 파일 두 개가
// 하나의 연결된 데이터셋처럼 동작한다.
class DummyDataGenerator {
public:
    explicit DummyDataGenerator(unsigned seed);

    std::vector<Sample> GenerateSamples(int count);
    std::vector<Order> GenerateOrders(int count, const std::vector<Sample>& samples);

    // JSON 파일로 저장 (samplesPath / ordersPath 두 파일이 sampleId로 연결됨)
    void SaveSamplesToJson(const std::vector<Sample>& samples, const std::string& path) const;
    void SaveOrdersToJson(const std::vector<Order>& orders, const std::string& path) const;

private:
    unsigned seed_;
};

} // namespace ddg
