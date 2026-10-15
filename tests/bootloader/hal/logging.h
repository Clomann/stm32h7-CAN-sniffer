#pragma once

void test_log_write(const char *fmt, ...);

#define EXAMPLE_LOG(...) test_log_write(__VA_ARGS__)
