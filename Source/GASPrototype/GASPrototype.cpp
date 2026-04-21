#include "GASPrototype.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGP);

/**
 * @file GASPrototype.cpp
 * @brief Main implementation file for the Game Module.
 *
 * This file contains the entry point for the game module logic. It registers
 * the module with the Unreal Engine core system, allowing the engine to recognize
 * and load the C++ classes defined within this project.
 */

// ----------------------------------------------------------------------------------------------------------------
// MODULE REGISTRATION
// ----------------------------------------------------------------------------------------------------------------

/**
 * @brief Macro to implement the primary game module.
 *
 * This macro handles the boilerplate code required to expose the module to the engine.
 *
 * @param ModuleImplClass  The class to use for the module implementation.
 * FDefaultGameModuleImpl is sufficient for most game logic unless
 * custom startup/shutdown code is needed (e.g., loading shader libraries).
 * @param ModuleName       The internal name of the module class (matches the .Build.cs name).
 * @param GameName         The string name of the game (used for logging and identification).
 */
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl,
                              GASPrototype,
                              "GASPrototype");
