#pragma once

typedef const char* cstring;

bool ParseAndRun(cstring input);

// helper
cstring Format(cstring fmt, ...);
