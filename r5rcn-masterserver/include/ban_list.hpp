#pragma once
#include "inc.hpp"

extern void handle_banlist_check_banned(http::request<http::string_body>& req, http::response<http::string_body>& res);
extern void handle_banlist_bulkCheck(http::request<http::string_body>& req, http::response<http::string_body>& res);