#pragma once

#include <string>
#include <dxgiformat.h>
#include <dxgicommon.h>

std::string EnumerateDxgiFormat(const DXGI_FORMAT Format);

std::string EnumerateDxgiColourSpace(const DXGI_COLOR_SPACE_TYPE ColourSpace);
