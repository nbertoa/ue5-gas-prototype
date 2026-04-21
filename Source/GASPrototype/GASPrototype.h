#pragma once

#include "CoreMinimal.h"

/**
 * @file GASPrototype.h
 * @brief Main header for the GASPrototype game module.
 *
 * Includes the custom log category used across the entire project.
 * All .cpp files in this module include this header (directly or indirectly),
 * making LogGP available everywhere without additional includes.
 */

// -------------------------------------------------------------------------
// CUSTOM LOG CATEGORY
// -------------------------------------------------------------------------
// Parameters:
//   LogGP  — Category name used in UE_LOG calls: UE_LOG(LogGP, Log, ...)
//   Log    — Default verbosity: shows Log, Warning, and Error messages.
//   All    — Compile-time verbosity: no log levels stripped at compile time.
DECLARE_LOG_CATEGORY_EXTERN(LogGP,
                            Log,
                            All);
