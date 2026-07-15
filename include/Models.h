#pragma once

#include <string>

namespace ddg {

// 시료(Sample) - 반도체 시료 생산주문관리 시스템의 기본 단위
struct Sample {
    std::string sampleId;          // ex) S-001
    std::string name;               // ex) 실리콘 웨이퍼-8인치
    double avgProcessTimeMin = 0.0; // 평균 생산시간 (min/ea)
    double yield = 0.0;             // 수율 (정상 시료 / 총 생산 시료)
    int stock = 0;                  // 현재 재고 수량
};

// 주문 상태
enum class OrderStatus {
    Reserved,   // 주문 접수
    Rejected,   // 주문 거절
    Producing,  // 승인 완료, 재고 부족으로 생산 중
    Confirmed,  // 승인 완료, 출고 대기 중
    Released,   // 출고 완료
};

std::string ToString(OrderStatus status);

// 주문(Order)
struct Order {
    std::string orderId;      // ex) ORD-20260715-0001
    std::string sampleId;     // 참조하는 시료 ID
    std::string customerName; // 고객명
    int quantity = 0;         // 주문 수량
    OrderStatus status = OrderStatus::Reserved;
    std::string orderedAt;    // 주문 접수 일시 (ISO 8601)
};

} // namespace ddg
