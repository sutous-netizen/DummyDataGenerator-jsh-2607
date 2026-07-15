# Dummy Data Generator - 설계 요약

## 목적

"반도체 시료 생산주문관리 시스템(S-Semi)" 개발에 앞서, 해당 시스템의 도메인 모델(시료 `Sample`, 주문
`Order`)을 기준으로 테스트용 더미 데이터를 생성하는 PoC(Proof of Concept) 도구다. 생성된 더미 데이터는
DB 대신 **JSON 파일**로 저장되며, 두 파일(`samples.json`, `orders.json`)이 `sampleId`로 서로 참조되어
하나의 연결된 데이터셋처럼 동작한다.

## 폴더 구조

```
include/
  Models.h              # Sample / Order 도메인 모델, OrderStatus enum
  JsonWriter.h           # 문자열 이스케이프 등 최소 JSON 직렬화 유틸
  DummyDataGenerator.h   # 생성기 클래스 선언
src/
  Models.cpp             # OrderStatus -> 문자열 변환
  DummyDataGenerator.cpp # 랜덤 데이터 생성 및 JSON 파일 저장 로직
  main.cpp               # CLI 진입점 (옵션 파싱, 실행)
data/                    # 실행 시 생성되는 JSON 산출물 (gitignore 대상)
docs/
  DESIGN.md              # 본 문서
```

## 데이터 모델

PDF 기능 명세(시료 관리 / 시료 주문 / 주문 상태 흐름)를 기준으로 필드를 구성했다.

### Sample (시료)

| 필드 | 타입 | 설명 |
|---|---|---|
| sampleId | string | `S-001` 형식의 고유 ID |
| name | string | 시료명 (예: 실리콘 웨이퍼-8인치) |
| avgProcessTimeMin | double | 평균 생산시간 (min/ea), 0.2~1.0 범위 |
| yield | double | 수율 (정상 시료 / 총 생산 시료), 0.75~0.98 범위 |
| stock | int | 현재 재고 수량, 0~1000 범위 (0은 재고 고갈 케이스) |

### Order (주문)

| 필드 | 타입 | 설명 |
|---|---|---|
| orderId | string | `ORD-YYYYMMDD-NNNN` 형식의 고유 ID |
| sampleId | string | 참조하는 시료 ID (Sample 목록에서만 선택) |
| customerName | string | 고객명 (연구소/팹리스/대학 연구실 풀에서 랜덤 선택) |
| quantity | int | 주문 수량, 10~500 범위 |
| status | string | `RESERVED` / `REJECTED` / `PRODUCING` / `CONFIRMED` / `RELEASE` |
| orderedAt | string | 주문 접수 일시 (ISO 8601), 최근 7일 이내로 분산 |

주문 상태는 실제 운영 분포를 흉내 내기 위해 가중치를 두었다(RESERVED 15% / CONFIRMED 30% /
PRODUCING 15% / RELEASE 30% / REJECTED 10%). REJECTED는 예외적인 흐름이므로 낮은 비중만 부여했다.

## 핵심 동작 흐름

1. `DummyDataGenerator(seed)` 생성 — 시드를 고정하면 항상 동일한 데이터를 재현할 수 있고, 시드를
   지정하지 않으면 실행 시각 기반으로 매번 다른 데이터를 생성한다.
2. `GenerateSamples(count)` — 시료명 풀에서 랜덤 선택하여 `count`개의 `Sample`을 생성한다.
3. `GenerateOrders(count, samples)` — 반드시 앞서 생성된 `samples` 중에서만 `sampleId`를 참조하여
   `Order`를 생성한다. 이 참조 관계가 두 JSON 파일을 "연결"시키는 지점이다.
4. `SaveSamplesToJson` / `SaveOrdersToJson` — 외부 라이브러리 없이 직접 구현한 `JsonWriter`로 문자열을
   이스케이프하며 JSON 텍스트를 만들고, 출력 경로의 상위 폴더를 자동 생성한 뒤 파일로 저장한다.

## CLI 사용법

```
DummyDataGenerator.exe [--samples N] [--orders N] [--seed N] [--out DIR]
```

| 옵션 | 기본값 | 설명 |
|---|---|---|
| --samples | 12 | 생성할 시료 개수 |
| --orders | 30 | 생성할 주문 개수 |
| --seed | 실행 시각 기반 | 랜덤 시드 (재현 가능한 데이터셋 생성용) |
| --out | data | JSON 파일을 저장할 디렉터리 |

실행하면 `<out>/samples.json`, `<out>/orders.json` 두 파일이 생성/덮어쓰기 되며, 콘솔에 생성 건수를
요약 출력한다.

## 검증

`MSBuild`(Debug|x64)로 정상 빌드되는 것을 확인했고, `--samples 8 --orders 15 --seed 42` 옵션으로
직접 실행하여 `data/samples.json`, `data/orders.json`이 sampleId로 서로 참조되도록 올바르게
생성되는 것을 확인했다. 소스 파일은 한글 리터럴을 포함하므로 프로젝트 설정에 `/utf-8` 컴파일 옵션을
추가해 MSVC가 UTF-8로 소스를 해석하도록 했다.

## 후속 프로젝트(SampleOrderSystem)와의 연계

이 PoC에서 정의한 `Sample` / `Order` 필드와 JSON 스키마는, 이후 별도 저장소로 구현할 "반도체 시료
생산주문관리 시스템"의 데이터 영속성 계층에서 초기 테스트 데이터로 그대로 활용할 수 있도록 맞췄다.
