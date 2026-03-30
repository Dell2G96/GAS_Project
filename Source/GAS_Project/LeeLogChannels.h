#pragma once

#include "Containers/UnrealString.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLee, Log, All);


FString GetClientServerContextString(UObject* ContextObject = nullptr);