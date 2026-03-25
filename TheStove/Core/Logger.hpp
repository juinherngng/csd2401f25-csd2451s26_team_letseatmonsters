/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Logger.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Provides a lightweight centralized logging utility with debug/info/warn/error
					levels, a shared output path, and helper macros so engine and game code can
					emit consistent diagnostics without duplicating console-print logic.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace CoreFramework::Logging {
	enum class Level {
		Debug = 0,
		Info,
		Warn,
		Error
	};

	inline Level DefaultLevel() {
#ifdef NDEBUG
		return Level::Info;
#else
		return Level::Debug;
#endif
	}

	inline Level& MinLevelStorage() {
		static Level minLevel = DefaultLevel();
		return minLevel;
	}

	inline std::mutex& OutputMutex() {
		static std::mutex mutex;
		return mutex;
	}

	inline void SetMinLevel(Level level) {
		MinLevelStorage() = level;
	}

	inline Level GetMinLevel() {
		return MinLevelStorage();
	}

	inline bool ShouldLog(Level level) {
		return static_cast<int>(level) >= static_cast<int>(GetMinLevel());
	}

	inline const char* Prefix(Level level) {
		switch (level) {
		case Level::Debug: return "[DEBUG] ";
		case Level::Info: return "[INFO] ";
		case Level::Warn: return "[WARN] ";
		case Level::Error: return "[ERROR] ";
		default: return "";
		}
	}

	inline std::ostream& Stream(Level level) {
		return (level == Level::Warn || level == Level::Error) ? std::cerr : std::cout;
	}

	inline void Write(Level level, const std::string& message) {
		if (!ShouldLog(level)) {
			return;
		}

		std::lock_guard<std::mutex> lock(OutputMutex());
		Stream(level) << Prefix(level) << message << std::endl;
	}
}

#define TS_LOG_AT(level, expr)                                                      \
	do {                                                                            \
		if (CoreFramework::Logging::ShouldLog(level)) {                             \
			std::ostringstream ts_log_stream__;                                      \
			ts_log_stream__ << expr;                                                 \
			CoreFramework::Logging::Write(level, ts_log_stream__.str());             \
		}                                                                           \
	} while (0)

#define TS_LOG_DEBUG(expr) TS_LOG_AT(CoreFramework::Logging::Level::Debug, expr)
#define TS_LOG_INFO(expr) TS_LOG_AT(CoreFramework::Logging::Level::Info, expr)
#define TS_LOG_WARN(expr) TS_LOG_AT(CoreFramework::Logging::Level::Warn, expr)
#define TS_LOG_ERROR(expr) TS_LOG_AT(CoreFramework::Logging::Level::Error, expr)

