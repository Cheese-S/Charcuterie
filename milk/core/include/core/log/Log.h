#pragma once
#include <core/container/IString.h>
#include <core/log/ILog.h>

namespace mk::log
{
// forward declare
struct Pad;

struct LogContext
{
    u64         count;
    StringView  userMsg;
    LogCategory category;
    StringView  filename;
    LogLevel    level;
    u8          tid;
    u16         ms;
    u16         line;
    std::tm     tm;
};

struct Lexer
{
    usize      i;
    StringView pattern;
};

Result compilePattern(StringView                  pattern,
                      VectorView<IFormatterPtr>   flagFormatterMap,
                      LogSystem::FormatterVector& formatters);
Result lexPadding(Lexer& lex, Pad& pad);
Result lexFlagFormatter(Lexer&                      lex,
                        VectorView<IFormatterPtr>   flagFomartterMap,
                        LogSystem::FormatterVector& formatters,
                        Pad                         pad);
void   lexLiteralFormatter(Lexer& lex, LogSystem::FormatterVector& formatters);
} // namespace mk::log
