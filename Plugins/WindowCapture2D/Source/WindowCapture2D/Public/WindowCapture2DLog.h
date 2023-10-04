#pragma once

DEFINE_LOG_CATEGORY_STATIC(LogWindowCapture2D, Display, All);
#define UE_WC2D_LOG(Verbosity, Format, ...) UE_LOG(LogWindowCapture2D, Verbosity, Format, __VA_ARGS__)