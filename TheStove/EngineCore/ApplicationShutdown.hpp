/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ApplicationShutdown.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares lightweight app-owned shutdown helpers so engine systems can
					request termination without owning the desktop application loop.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

 /**
  * @brief Requests an orderly application shutdown from any engine/runtime layer.
  */
void RequestApplicationShutdown();

/**
 * @brief Returns whether application shutdown has been requested.
 * @return `true` when the desktop app should stop running.
 */
bool IsApplicationShutdownRequested();
