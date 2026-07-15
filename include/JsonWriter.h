#pragma once

#include <sstream>
#include <string>

namespace ddg {

// 외부 의존성 없이 배열/객체 형태의 JSON 텍스트를 생성하는 최소 writer.
// PoC 범위(더미 데이터 저장)에 필요한 만큼만 지원한다.
class JsonWriter {
public:
    static std::string EscapeString(const std::string& value) {
        std::string escaped;
        escaped.reserve(value.size());
        for (char c : value) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n";  break;
                case '\r': escaped += "\\r";  break;
                case '\t': escaped += "\\t";  break;
                default:   escaped += c;      break;
            }
        }
        return escaped;
    }

    static std::string QuoteString(const std::string& value) {
        return "\"" + EscapeString(value) + "\"";
    }

    static std::string FormatDouble(double value) {
        std::ostringstream oss;
        oss.precision(4);
        oss << std::fixed << value;
        return oss.str();
    }
};

} // namespace ddg
