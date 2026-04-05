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

#include <cctype>
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

	/**
	 * @brief Returns the default minimum log level for the current build type.
	 * @return Default log level for debug or release builds.
	 */
	inline Level DefaultLevel() {
#ifdef NDEBUG
		// Release builds default to quieter logging.
		return Level::Info;
#else
		// Debug builds surface verbose diagnostics by default.
		return Level::Debug;
#endif
	}

	/**
	 * @brief Returns the global storage slot for the active minimum log level.
	 * @return Reference to the mutable minimum log level.
	 */
	inline Level& MinLevelStorage() {
		static Level minLevel = DefaultLevel();
		// Keep one shared minimum-level value for the whole process.
		return minLevel;
	}

	/**
	 * @brief Returns the mutex guarding logger output streams.
	 * @return Reference to the shared output mutex.
	 */
	inline std::mutex& OutputMutex() {
		static std::mutex mutex;
		// Serialize log writes so multi-threaded logging does not interleave messages.
		return mutex;
	}

	/**
	 * @brief Sets the minimum log level that will be emitted.
	 * @param level New minimum log level.
	 */
	inline void SetMinLevel(Level level) {
		// Update the shared threshold used by all later log calls.
		MinLevelStorage() = level;
	}

	/**
	 * @brief Returns the currently active minimum log level.
	 * @return Active minimum log level.
	 */
	inline Level GetMinLevel() {
		// Read the shared threshold used to filter log output.
		return MinLevelStorage();
	}

	/**
	 * @brief Checks whether a message at the given level should be logged.
	 * @param level Level of the message being considered.
	 * @return True if the message passes the current minimum-level filter.
	 */
	inline bool ShouldLog(Level level) {
		// Higher-severity levels always pass once the threshold is met.
		return static_cast<int>(level) >= static_cast<int>(GetMinLevel());
	}

	/**
	 * @brief Returns the text prefix used for a given log level.
	 * @param level Log level to describe.
	 * @return Prefix string such as `"[INFO] "`.
	 */
	inline const char* Prefix(Level level) {
		switch (level) {
		case Level::Debug: return "[DEBUG] ";
		case Level::Info: return "[INFO] ";
		case Level::Warn: return "[WARN] ";
		case Level::Error: return "[ERROR] ";
		default: return "";
		}
	}

	/**
	 * @brief Returns the output stream used for a log level.
	 * @param level Log level to route.
	 * @return Output stream for the message.
	 */
	inline std::ostream& Stream(Level level) {
		// Route warnings and errors to stderr while keeping normal diagnostics on stdout.
		return (level == Level::Warn || level == Level::Error) ? std::cerr : std::cout;
	}

	/**
	 * @brief Trims leading horizontal whitespace from a log message.
	 * @param message Raw log message string.
	 * @return Message with leading spaces and tabs removed.
	 */
	inline std::string TrimLeadingWhitespace(const std::string& message) {
		size_t index = 0;
		while (index < message.size() &&
			std::isspace(static_cast<unsigned char>(message[index])) != 0 &&
			message[index] != '\n' &&
			message[index] != '\r') {
			++index;
		}
		// Preserve intentional newlines while removing accidental leading indentation from macro call sites.
		return message.substr(index);
	}

	/**
	 * @brief Writes a formatted log message if it passes the level filter.
	 * @param level Severity level of the message.
	 * @param message Message text to emit.
	 */
	inline void Write(Level level, const std::string& message) {
		if (!ShouldLog(level)) {
			return;
		}

		// Hold the output mutex while writing so messages remain intact across threads.
		std::lock_guard<std::mutex> lock(OutputMutex());
		Stream(level) << Prefix(level) << TrimLeadingWhitespace(message) << std::endl;
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
