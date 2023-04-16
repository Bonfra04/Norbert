#pragma once

#include <stdio.h>

#define Test(name) void __attribute__((constructor)) name()

#define SucceedTest() do { printf("\033[1;34m[\033[1;32m+\033[1;34m]\033[0m Passed: \033[1;32m%s\033[0m\n", __func__); return; } while(0)
#define FailTest() do { printf("\033[1;34m[\033[1;31m-\033[1;34m]\033[0m Failed: \033[1;31m%s (%s:%d)\033[0m\n", __func__, __FILE__, __LINE__); return; } while(0)
#define Assert(condition) do { if(!condition) FailTest(); } while(0)