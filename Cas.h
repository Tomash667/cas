#pragma once

typedef const char* cstring;

bool ParseAndRun(cstring input, bool optimize = true, bool decompile = false);

// helper
cstring Format(cstring fmt, ...);
