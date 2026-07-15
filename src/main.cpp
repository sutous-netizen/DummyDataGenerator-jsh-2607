#include <chrono>
#include <iostream>
#include <string>

#include "DummyDataGenerator.h"

namespace {

struct Options {
    int sampleCount = 12;
    int orderCount = 30;
    unsigned seed = 0;
    std::string outputDir = "data";
};

Options ParseArgs(int argc, char* argv[]) {
    Options options;
    options.seed = static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto nextValue = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string(name) + " 옵션에 값이 필요합니다.");
            }
            return argv[++i];
        };

        if (arg == "--samples") {
            options.sampleCount = std::stoi(nextValue("--samples"));
        } else if (arg == "--orders") {
            options.orderCount = std::stoi(nextValue("--orders"));
        } else if (arg == "--seed") {
            options.seed = static_cast<unsigned>(std::stoul(nextValue("--seed")));
        } else if (arg == "--out") {
            options.outputDir = nextValue("--out");
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "사용법: DummyDataGenerator [--samples N] [--orders N] [--seed N] [--out DIR]\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("알 수 없는 옵션: " + arg);
        }
    }
    return options;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const Options options = ParseArgs(argc, argv);

        std::cout << "===============================================\n";
        std::cout << " 반도체 시료 생산주문관리 시스템 - Dummy Data Generator\n";
        std::cout << "===============================================\n";
        std::cout << "시료 생성 개수 : " << options.sampleCount << "\n";
        std::cout << "주문 생성 개수 : " << options.orderCount << "\n";
        std::cout << "랜덤 시드      : " << options.seed << "\n";
        std::cout << "출력 경로      : " << options.outputDir << "\n\n";

        ddg::DummyDataGenerator generator(options.seed);

        const auto samples = generator.GenerateSamples(options.sampleCount);
        const auto orders = generator.GenerateOrders(options.orderCount, samples);

        const std::string samplesPath = options.outputDir + "/samples.json";
        const std::string ordersPath = options.outputDir + "/orders.json";

        generator.SaveSamplesToJson(samples, samplesPath);
        generator.SaveOrdersToJson(orders, ordersPath);

        std::cout << "생성 완료:\n";
        std::cout << "  - " << samplesPath << " (" << samples.size() << "건)\n";
        std::cout << "  - " << ordersPath << " (" << orders.size() << "건)\n";

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "오류 발생: " << ex.what() << "\n";
        return 1;
    }
}
