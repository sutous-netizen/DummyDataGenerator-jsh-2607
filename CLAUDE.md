# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

"반도체 시료 생산주문관리 시스템(S-Semi)" 개발에 앞서, 해당 시스템의 도메인 모델(시료 `Sample`, 주문
`Order`)을 기준으로 테스트용 더미 데이터를 생성하는 C++ PoC(Proof of Concept) 도구다. DB 없이 JSON
파일(`samples.json`, `orders.json`)만 산출하며, 두 파일은 `sampleId`로 서로 참조되어 하나의 연결된
데이터셋처럼 동작한다. 자세한 설계 배경은 `docs/DummyDataGenerator.md`에 있다.

이 PoC에서 정의한 `Sample`/`Order` 필드와 JSON 스키마는, 이후 별도 저장소로 구현될 실제
"반도체 시료 생산주문관리 시스템"의 초기 테스트 데이터로 그대로 재사용될 예정이다.

## 빌드 / 실행

Visual Studio 프로젝트(MSBuild, `DummyDataGenerator.vcxproj`)이며 별도의 CMake/Makefile은 없다.

```powershell
# 빌드 (Debug|x64)
msbuild DummyDataGenerator.vcxproj -p:Configuration=Debug -p:Platform=x64

# 실행 (기본값: samples 12, orders 30, seed는 실행 시각 기반, out=data)
x64\Debug\DummyDataGenerator.exe

# 옵션 지정 실행 예시 (재현 가능한 데이터셋)
x64\Debug\DummyDataGenerator.exe --samples 8 --orders 15 --seed 42 --out data
```

주요 CLI 옵션: `--samples N`, `--orders N`, `--seed N`, `--out DIR`, `--help`. 실행하면
`<out>/samples.json`, `<out>/orders.json`을 덮어쓰기 생성하고 콘솔에 생성 건수를 요약 출력한다.

별도의 테스트 실행 커맨드는 없다 — `packages.config`/`.vcxproj`에 gmock/gtest(1.11.0) NuGet 참조가
걸려 있지만, 현재 `src/`에는 이를 사용하는 테스트 코드가 존재하지 않는다.

## 아키텍처

```
include/
  Models.h              # Sample / Order 도메인 모델, OrderStatus enum, ToString() 선언
  JsonWriter.h           # 문자열 이스케이프 등 최소 JSON 직렬화 유틸 (외부 JSON 라이브러리 미사용)
  DummyDataGenerator.h   # 생성기 클래스 선언
src/
  Models.cpp             # OrderStatus -> 문자열 변환 구현
  DummyDataGenerator.cpp # 랜덤 데이터 생성 및 JSON 파일 저장 로직
  main.cpp               # CLI 진입점 (옵션 파싱, 실행)
data/                    # 실행 시 생성되는 JSON 산출물 (gitignore 대상, 커밋 금지)
```

핵심 흐름 (`DummyDataGenerator` 클래스, `include/DummyDataGenerator.h` / `src/DummyDataGenerator.cpp`):

1. `DummyDataGenerator(seed)` — 시드를 고정하면 항상 동일한 데이터를 재현할 수 있다.
2. `GenerateSamples(count)` — 고정된 시료명 풀(`kSampleNamePool`)에서 랜덤 선택해 `Sample` 목록을 만든다.
3. `GenerateOrders(count, samples)` — 반드시 앞서 생성된 `samples`의 `sampleId`만 참조하여 `Order`를
   생성한다. 이 참조 관계가 `samples.json`과 `orders.json`을 "연결"시키는 지점이므로, 주문 생성 로직을
   수정할 때는 항상 실제 존재하는 sampleId만 참조하도록 유지해야 한다. 주문 상태(`OrderStatus`)는
   실제 운영 분포를 흉내 내기 위해 가중치(`kStatusWeights`: RESERVED 15% / CONFIRMED 30% /
   PRODUCING 15% / RELEASE 30% / REJECTED 10%)를 적용한다.
4. `SaveSamplesToJson` / `SaveOrdersToJson` — `JsonWriter`로 문자열을 이스케이프하며 JSON 텍스트를
   직접 조립하고, 출력 경로의 상위 폴더를 자동 생성한 뒤 파일로 저장한다.

### 한글/UTF-8 관련 주의사항

- 소스 파일이 한글 리터럴(시료명, 고객사명 등)을 포함하므로, 모든 빌드 구성(Debug/Release, x86/x64)의
  `ClCompile`에 `/utf-8` 컴파일 옵션이 걸려 있다. 새 구성을 추가할 때도 이 옵션을 유지해야 한다.
- 한국어 Windows 콘솔(CP949)에서 `std::cout`으로 한글이 깨지는 것을 막기 위해, `main()` 시작 시
  `SetConsoleOutputCP(CP_UTF8)` / `SetConsoleCP(CP_UTF8)`로 콘솔 코드페이지를 전환한다
  (`src/main.cpp`). 콘솔 출력 관련 코드를 건드릴 때 이 처리를 제거하지 말 것.
  

## commit 정책
제목 해더에 아래 키워드로 시작한다.
[test] : 테스트 코드 반영
[fix] : bug 수정
[feat] : 기능 구현
[refac] : 코드 리팩토링
[xxx] : 기타 정보 표시

commit message 는 아래 내용으로 한다.
- 요약 정보
- 빌드결과
- 테스트 결과 표시