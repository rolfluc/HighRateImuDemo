#pragma once
#include <stdint.h>

void InitUart();
void putChar(char c);
void putStrn(char* str, uint8_t len);