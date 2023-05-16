// 一个简单的c++程序，使用boost库和json库，能够接受两种不同的post请求，一种是获取json数据，一种是添加json数据，并且能够将添加的json数据存储起来，供获取的请求使用
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

// 创建一个全局变量，用于存储添加的json数据
std::vector<json> json_data;

// 处理获取json数据的请求的函数
void handle_get_json_request(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 创建一个json对象，作为响应内容
    json response;
    // 从请求中获取version参数的值
    std::string version = req["version"];
    // 判断version参数是否为VGameSDK008，如果是，则设置success为true，否则设置success为false和error信息
    if (version == "VGameSDK008") {
        response["success"] = true;
    }
    else {
        response["success"] = false;
        response["error"] = "Your SDK version is unsupported, please update the SDK.";
    }
    // 将存储的json数据作为servers字段的值
    response["servers"] = json_data;
    // 将json对象转换为字符串，并设置为响应的body
    res.body() = response.dump();
}

// 处理添加json数据的请求的函数
void handle_add_json_request(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 打印json数据
    std::cout << "Received json data: " << data << std::endl;
    // 将json数据添加到存储中
    json_data.push_back(data);
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 创建一个json对象，作为响应内容
    json response;
    response["message"] = "Added json data successfully";
    response["data"] = data; // 将添加的json数据作为响应内容
    // 将json对象转换为字符串，并设置为响应的body
    res.body() = response.dump();
}

// 主函数
int main() {
    try {
        // 创建一个io_context对象，用于管理异步操作
        asio::io_context ioc;
        // 创建一个ip地址对象，表示监听的地址，这里使用ipv4的回环地址
        asio::ip::address address = asio::ip::make_address("127.0.0.1");
        // 创建一个端口号对象，表示监听的端口，这里使用8080端口
        unsigned short port = static_cast<unsigned short>(8080);
        // 创建一个endpoint对象，表示监听的地址和端口的组合
        asio::ip::tcp::endpoint endpoint{ address, port };
        // 创建一个acceptor对象，用于接受客户端的连接请求
        asio::ip::tcp::acceptor acceptor{ ioc, endpoint };
        // 等待客户端的连接请求
        std::cout << "Listening on " << endpoint << std::endl;
        asio::ip::tcp::socket socket{ ioc };
        acceptor.accept(socket);
        std::cout << "Accepted connection from " << socket.remote_endpoint() << std::endl;
        // 创建一个缓冲区对象，用于存储接收到的数据
        beast::flat_buffer buffer;
        // 创建一个请求对象，用于解析接收到的HTTP请求
        http::request<http::string_body> req;
        // 读取HTTP请求，并将其存储到请求对象中
        http::read(socket, buffer, req);
        std::cout << "Received request: " << req << std::endl;
        // 创建一个响应对象，用于发送HTTP响应
        http::response<http::string_body> res;
        // 判断请求的目标是否为/servers，如果是，则调用处理获取json数据的请求的函数，否则继续判断
        if (req.target() == "/servers") {
            handle_get_json_request(req, res);
            // 判断请求的目标是否为/servers/add，如果是，则调用处理添加json数据的请求的函数，否则返回404 Not Found错误
        }
        else if (req.target() == "/servers/add") {
            handle_add_json_request(req, res);
        }
        else {
            res.version(11); // HTTP 1.1
            res.result(http::status::not_found); // 404 Not Found
            res.set(http::field::content_type, "text/plain"); // 设置响应内容类型为纯文本
            res.body() = "The requested resource was not found"; // 设置响应内容为错误信息
        }
        // 发送HTTP响应，并关闭连接
        http::write(socket, res);
        socket.shutdown(asio::ip::tcp::socket::shutdown_send);
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}

