#pragma once

const bool CI_MODE = ((_CI_MODE - 1) == 0);

void RunFileTest(cstring filename, cstring input, cstring output, bool optimize = true);
void RunFailureTest(cstring code, cstring error);
