#include "func.h"
#include "sensitive_data.h" /* excluded from git */

#pragma comment(lib, "version.lib")

void boost_logger::init_logging()
{
	boost::log::add_common_attributes();

	const auto console_sink = boost::log::add_console_log(std::clog);
	console_sink->set_formatter(&log_formatter);
	logging::core::get()->add_sink(console_sink);

	const auto fs_sink = boost::log::add_file_log(
		boost::log::keywords::file_name = "C:\\rasa\\night_lister.sql",
		keywords::format = "% Message %",
		boost::log::keywords::rotation_size = 10 * 1024 * 1024,
		boost::log::keywords::min_free_space = 30 * 1024 * 1024,
		boost::log::keywords::open_mode = std::ios_base::out | std::ios_base::trunc);

	fs_sink->locked_backend()->auto_flush(true);
	LOG_SAVE << L"USE[" + db_name + L"]";
	LOG_SAVE << "GO";
	LOG_SAVE << "TRUNCATE TABLE rvt_night_list;";
}

void boost_logger::log_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
	strm << rec[expr::smessage];
}

const std::wstring uudecode(const std::string& input) {
	std::wstring decoded;
	int len = input.length();

	for (int i = 0; i < len; i += 4) {
		unsigned int combined = 0;
		for (int j = 0; j < 4; ++j) {
			unsigned char c = input[i + j];
			c = (c == '`') ? 0 : (c - 32) & 0x3F;  // decode character
			combined = (combined << 6) | c;
		}

		decoded += (combined >> 16) & 0xFF;
		if (i + 2 < len) decoded += (combined >> 8) & 0xFF;
		if (i + 3 < len) decoded += combined & 0xFF;
	}

	return decoded;
}

std::string get_model_identity(const std::wstring& xml_path)
{
	std::ifstream ifs(xml_path + L"\\Model.rvt", std::ios::binary);
	if (!ifs.is_open())
		return "";

	/* Грузим файл целиком */
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	std::string xml = buffer.str();

	std::regex rgx(R"(<ModelIdentity>(.*?)</ModelIdentity>)", std::regex::icase);
	std::smatch match;

	if (std::regex_search(xml, match, rgx))
	{
		ifs.close();
		return match[1].str();
	}

	return "";
}

std::string get_self_version() {
	char sz_path[MAX_PATH];
	GetModuleFileNameA(nullptr, sz_path, MAX_PATH);

	DWORD dw_handle = 0;
	const DWORD dw_size = GetFileVersionInfoSizeA(sz_path, &dw_handle);

	if (dw_size == 0) return "0.0.0.1";

	std::vector<BYTE> ptr_version_info(dw_size);
	if (!GetFileVersionInfoA(sz_path, dw_handle, dw_size, ptr_version_info.data()))
		return "0.0.0.2";

	VS_FIXEDFILEINFO* ptr_file_info = nullptr;
	UINT ui_size = 0;
	if (!VerQueryValueA(ptr_version_info.data(), "\\", (LPVOID*)&ptr_file_info, &ui_size))
		return "0.0.0.3";

	return std::to_string(HIWORD(ptr_file_info->dwFileVersionMS)) + "." +
		std::to_string(LOWORD(ptr_file_info->dwFileVersionMS)) + "." +
		std::to_string(HIWORD(ptr_file_info->dwFileVersionLS)) + "." +
		std::to_string(LOWORD(ptr_file_info->dwFileVersionLS));
}