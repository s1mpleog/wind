#include "CrashHandler.hpp"

#include "Config.hpp"
#include "Types.hpp"

#include <chrono>
#include <filesystem>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

WIND_INLINE std::filesystem::path MakeCrashDirctory() WIND_NOEXCEPT
{
	auto Now = std::chrono::system_clock::now();
	auto Ts = std::format("{:%Y-%m-%d_%H-%M-%S}", Now);

	std::filesystem::path Dir = "Saved/Crashes/" + Ts;
	std::filesystem::create_directories(Dir);
	return Dir;
}

WIND_INLINE void WriteCrashLog(const std::filesystem::path &Directory, std::string_view Message) WIND_NOEXCEPT
{
	std::filesystem::path LogPath = Directory / "Crash.log";

	auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogPath.string(), true);
	auto CrashLog = std::make_shared<spdlog::logger>("crash", FileSink);

	CrashLog->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
	CrashLog->critical("{}", Message);
	CrashLog->flush();

	spdlog::critical("{}", Message);
}

void Fatal(std::string_view Expression, std::string_view Error, const char *File, uint32 Line) WIND_NOEXCEPT
{
	std::filesystem::path Directory = MakeCrashDirctory();
	std::string Message = std::format("FATAL @ {}:{}\n  expr : {}\n  error: {}", File, Line, Expression, Error);
	WriteCrashLog(Directory, Message);
	std::terminate();
}
