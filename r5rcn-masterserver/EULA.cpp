#include "include\inc.hpp"
#include "include\servers_list.hpp"
#include "include\file_system.hpp"

void handle_check_eula(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 读取eula.json文件，并转换为一个json对象
    json eula = read_eula();

    // 将json对象转换为字符串，并设置为响应的body
    res.body() = eula.dump();
    std::cout << get_time() << "Send eula check data: " << eula << std::endl;
}
