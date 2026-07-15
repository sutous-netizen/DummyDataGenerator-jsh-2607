#include "JsonWriter.h"

#include <gtest/gtest.h>

using ddg::JsonWriter;

TEST(JsonWriterTest, EscapeString_PlainTextUnchanged) {
    EXPECT_EQ(JsonWriter::EscapeString("실리콘 웨이퍼-8인치"), "실리콘 웨이퍼-8인치");
    EXPECT_EQ(JsonWriter::EscapeString(""), "");
}

TEST(JsonWriterTest, EscapeString_EscapesDoubleQuoteAndBackslash) {
    EXPECT_EQ(JsonWriter::EscapeString("\"quoted\""), "\\\"quoted\\\"");
    EXPECT_EQ(JsonWriter::EscapeString("a\\b"), "a\\\\b");
}

TEST(JsonWriterTest, EscapeString_EscapesControlCharacters) {
    EXPECT_EQ(JsonWriter::EscapeString("line1\nline2"), "line1\\nline2");
    EXPECT_EQ(JsonWriter::EscapeString("a\rb"), "a\\rb");
    EXPECT_EQ(JsonWriter::EscapeString("a\tb"), "a\\tb");
}

TEST(JsonWriterTest, QuoteString_WrapsAndEscapes) {
    EXPECT_EQ(JsonWriter::QuoteString("abc"), "\"abc\"");
    EXPECT_EQ(JsonWriter::QuoteString("a\"b"), "\"a\\\"b\"");
}

TEST(JsonWriterTest, FormatDouble_UsesFixedFourDecimalPlaces) {
    EXPECT_EQ(JsonWriter::FormatDouble(1.0), "1.0000");
    EXPECT_EQ(JsonWriter::FormatDouble(0.0), "0.0000");
    EXPECT_EQ(JsonWriter::FormatDouble(0.12345), "0.1235"); // 반올림 확인
}
