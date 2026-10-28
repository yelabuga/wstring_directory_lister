#pragma once

#include "stdafx.h"

#define LOG_SAVE BOOST_LOG_SEV(boost::log::trivial::logger::get(), boost::log::trivial::severity_level::trace)	\
	<< boost::log::add_value("Line", __LINE__)

const std::wstring uudecode(const std::string& input);
std::string get_model_identity(const std::wstring& xml_path);
std::string get_self_version();

namespace boost_logger
{
	void init_logging();
	void log_formatter(logging::record_view const& rec, logging::formatting_ostream& strm);	
};