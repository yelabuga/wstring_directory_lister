#include "stdafx.h"
#include "func.h"

#pragma comment(lib, "WtsApi32.lib")

#include "sensitive_data.h" /* excluded from git */

namespace fs = std::filesystem;

int main() {
    boost_logger::init_logging();    

    const std::string lister_version = get_self_version();
    std::cout << "Revit server models lister v" << lister_version << "\n";
    std::string model_identity = "";

    /* т.к. у нас обработка кириллических имён файлов происходит на английской версии Windows Server, юзаем UTF-16 */
    _setmode(_fileno(stdout), _O_U16TEXT);

    try {
        /* Iterate recursively through the directory and its subdirectories */
        for (const auto& entry : fs::recursive_directory_iterator("C:\\ProgramData\\Autodesk\\Revit Server 2023\\Projects\\Projects")) {
            std::string path_string{ entry.path().u8string() };

            auto rvt_extension = path_string.substr(path_string.size() - 3);
            boost::algorithm::to_lower(rvt_extension);

            if (fs::is_directory(entry) && rvt_extension == "rvt") {                
                model_identity = get_model_identity(entry.path().wstring());
                boost::replace_all(path_string, "C:\\ProgramData\\Autodesk\\Revit Server 2023\\Projects\\Projects", "RSN://ald-vm-revit01/Projects");
                boost::replace_all(path_string, "\\", "/");

                LOG_SAVE << "INSERT INTO[dbo].[rvt_night_list]([rvt_path], [rvt_name], [guid]) VALUES ('" << path_string << "', '" << entry.path().filename().u8string() << "', '" << model_identity << "')";
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }

    std::string query_update_view = R"(
    MERGE dbo.rvt_list AS target
        USING(
            SELECT *
            FROM dbo.rvt_night_list
            WHERE TRY_CONVERT(UNIQUEIDENTIFIER, guid) IS NOT NULL
        ) AS source
        ON target.rvt_list_guid = TRY_CONVERT(UNIQUEIDENTIFIER, source.guid)

        WHEN MATCHED AND(
            ISNULL(target.rvt_path, '') < > ISNULL(source.rvt_path, '')
            OR ISNULL(target.rvt_name, '') < > ISNULL(source.rvt_name, '')
        )
        THEN UPDATE SET
        target.rvt_path = source.rvt_path,
        target.rvt_name = source.rvt_name

        WHEN NOT MATCHED BY TARGET THEN
        INSERT(
            rvt_list_guid,
            rvt_path,
            rvt_name,
            is_active
        )
        VALUES(
            TRY_CONVERT(UNIQUEIDENTIFIER, source.guid),
            source.rvt_path,
            source.rvt_name,
            1
        )

        WHEN NOT MATCHED BY SOURCE THEN
        DELETE;
	)";

    LOG_SAVE << query_update_view;

    std::wstring command_line = L"sqlcmd.exe -S " + db_host_name + L" -d " + db_name + L" -U " + db_user_name + L" -i C:\\rasa\\night_lister.sql -o C:\\rasa\\mssql.log";

    /* пароль передаётся через окружение, а не через -P: так он не виден в списке процессов и в аудите командных строк */
    SetEnvironmentVariableW(L"SQLCMDPASSWORD", db_password.c_str()); /* hash/decrypting process is in sensitive_data.h */

    STARTUPINFOW startup_info{ sizeof(startup_info) };
    PROCESS_INFORMATION process_info{};
    const BOOL is_started = CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup_info, &process_info);

    /* дочерний процесс уже получил копию окружения, у себя переменную убираем */
    SetEnvironmentVariableW(L"SQLCMDPASSWORD", nullptr);

    if (!is_started) {
        std::wcerr << L"sqlcmd.exe start error: " << GetLastError() << L'\n';
        return 1;
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(process_info.hProcess, &exit_code);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return static_cast<int>(exit_code);
}
