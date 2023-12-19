#pragma once

#include "rad/IO/Logging.h"

extern rad::LogCategory g_logVFX;
#define LogVFX(Level, Format, ...) g_logVFX.Print(rad::LogLevel::Level, Format, ##__VA_ARGS__)
