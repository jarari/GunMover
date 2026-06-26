#pragma once
#define _AMD64_

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

#include <format>
#include <source_location>
#include <utility>

#pragma warning(push)
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <REX/REX.h>
#include <windows.h>
#pragma warning(pop)
#pragma warning(disable: 4100);

#define DLLEXPORT __declspec(dllexport)

namespace logger
{
	inline void info(std::string_view a_msg) { REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Info, a_msg); }
	inline void warn(std::string_view a_msg) { REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Warning, a_msg); }
	inline void error(std::string_view a_msg) { REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Error, a_msg); }
	inline void critical(std::string_view a_msg) { REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Critical, a_msg); }

	template <class... Args>
	void info(std::format_string<Args...> a_fmt, Args&&... a_args)
	{
		REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Info, a_fmt, std::forward<Args>(a_args)...);
	}

	template <class... Args>
	void warn(std::format_string<Args...> a_fmt, Args&&... a_args)
	{
		REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Warning, a_fmt, std::forward<Args>(a_args)...);
	}

	template <class... Args>
	void error(std::format_string<Args...> a_fmt, Args&&... a_args)
	{
		REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Error, a_fmt, std::forward<Args>(a_args)...);
	}

	template <class... Args>
	void critical(std::format_string<Args...> a_fmt, Args&&... a_args)
	{
		REX::Impl::Log(std::source_location::current(), REX::ELogLevel::Critical, a_fmt, std::forward<Args>(a_args)...);
	}
}

using namespace std::literals;

#include "version.h"
