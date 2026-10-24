#pragma once

/*
 * Шаблон для sensitive_data.h.
 * Для сборки необходимо скопировать этот файл в sensitive_data.h и заполнить значения.
 * sensitive_data.h исключён из git (см. .gitignore), коммитить его нельзя.
 */

#include "func.h"

/* SQL Server, на который выгружается список моделей */
const std::wstring db_host_name = L"sql-server-host";
const std::wstring db_name = L"database_name";

/* SQL-логин, под которым sqlcmd выполняет night_lister.sql */
const std::wstring db_user_name = L"sql_login";

/*
 * Пароль SQL-логина. Передаётся в sqlcmd через переменную окружения SQLCMDPASSWORD.
 * В исходниках хранится в закодированном виде, раскодируется функцией uudecode().
 * Это обфускация, а не шифрование: пароль можно извлечь из exe.
 */
const std::wstring db_password = uudecode("encoded_password");
