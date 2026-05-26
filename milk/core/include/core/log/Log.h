#pragma once
#include <core/container/IString.h>
#include <core/log/ILog.h>

namespace mk::log
{
// forward declare
struct Pad;

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
