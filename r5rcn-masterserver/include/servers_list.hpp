#pragma once
#include "inc.hpp"

// 创建一个全局变量，用于存储添加的服务器数据
extern std::vector<json> json_data;
// 创建一个全局变量，用于存储生成的服务器token以及其服务器
extern std::vector<json> json_token;

// 创建一个io_context对象
extern boost::asio::io_context io_context;

// 创建一个io_context对象
extern boost::asio::io_context io_pcontext;

//声明定时器变量，不要在头文件中定义
extern boost::asio::deadline_timer pglobal_timer;
extern boost::asio::deadline_timer global_timer;

extern void add_private_server(http::request<http::string_body>& req, http::response<http::string_body>& res, std::string ip_address);
extern void handle_create_server_request(http::request<http::string_body>& req, http::response<http::string_body>& res, std::string ip_address);
extern void handle_get_servers_list_request(http::request<http::string_body>& req, http::response<http::string_body>& res);
extern void handle_get_server_byToken(http::request<http::string_body>& req, http::response<http::string_body>& res);
extern void handle_check_eula(http::request<http::string_body>& req, http::response<http::string_body>& res);
