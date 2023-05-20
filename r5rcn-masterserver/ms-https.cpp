// 这个版本使用SSL/TLS加密，支持https协议
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

// 创建一个全局变量，用于存储添加的json数据
std::vector<json> json_data;

// 创建一个io_context对象
boost::asio::io_context io_context;
// 创建一个全局变量，用于存储定时器对象
// 改名为global_timer，避免与main函数中的局部变量冲突
boost::asio::deadline_timer global_timer(io_context);

// 处理添加json数据的请求的函数
void handle_add_json_request(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 打印json数据
    std::cout << "Received json data: " << data << std::endl;
    // 检查json数据是否包含了指定的参数，并且参数的类型是否为字符串
    bool invalid = data.contains("cachedId") && data.contains("checksum") && data.contains("description") && data.contains("hidden") && data.contains("ip") && data.contains("key") && data.contains("map") && data.contains("maxPlayers") && data.contains("name") && data.contains("playerCount") && data.contains("playlist") && data.contains("port") && data.contains("publicRef") && data.contains("timeStamp") && data.contains("version");
    // 将invalid变量的值赋给一个新的变量，比如no_response
    bool no_response = invalid;
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    json response;

    // 判断no_response是否为真，否则按照原来的逻辑设置响应内容
    if (no_response) {
        response["success"] = true;
        res.body() = response.dump();
        std::cout << "Send json data: " << response << std::endl;

        // - 在存储的数据中查找是否有与接收到的数据相匹配的内容，并更新或添加

        // 定义一个标志变量，表示是否找到匹配的内容
        bool found = false;

        // 遍历存储的数据中的每个元素
        for (auto& j : json_data) {
            // 定义一个标志变量，表示是否所有指定的参数都相等
            bool equal = true;

            // 遍历指定的参数列表，并比较每个参数的值是否相等
            for (const auto& param : { "cachedId", "checksum", "description", "hidden", "ip", "key", "map", "maxPlayers", "name", "playlist", "port", "publicRef", "version" }) {
                if (j[param].dump() != data[param].dump()) { // 使用dump()方法来将值转换为字符串并比较
                    equal = false;
                    break;
                }
            }

            // 如果所有指定的参数都相等，则更新其他参数的值，并设置found为true
            if (equal) {
                j["playerCount"] = data["playerCount"];
                j["timeStamp"] = data["timeStamp"];
                j["lastUpdate"] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); // 添加lastUpdate参数，并赋值为当前时间戳
                found = true;
                break;
            }
        }

        // 如果没有找到匹配的内容，则将接收到的数据添加到存储中，并添加lastUpdate参数
        if (!found) {
            json_data.push_back(data);
        }

        // - 使用global_timer代替timer，以使用全局变量而不是局部变量

        // 取消之前的定时器，避免重复执行回调函数
        global_timer.cancel();

        // 设置定时器的超时时间为3秒，并将io_context对象作为参数传递给构造函数
        global_timer.expires_from_now(boost::posix_time::seconds(3));

        // 设置定时器到期时执行的回调函数，删除存储的数据中超过3秒没有更新过的内容
        global_timer.async_wait([data](const boost::system::error_code& ec) {
            if (!ec) { // 没有错误发生
                std::cout << "Timer expired" << std::endl;
                // 在存储的数据中查找超过3秒没有更新过的内容，并删除
                auto it = std::remove_if(json_data.begin(), json_data.end(), [data](const json& j) {
                    return j["timeStamp"].get<long long>() - data["timeStamp"].get<long long>() > 3000; // 使用long long类型来表示时间戳，并使用timeStamp参数来判断是否超时
                    });

                if (it != json_data.end()) {
                    std::cout << "Deleted json data: " << *it << std::endl;
                    json_data.erase(it, json_data.end());
                    std::cout << json_data << std::endl;
                }
            }
            else { // 有错误发生，并打印错误信息
                std::cerr << "Error: " << ec.message() << std::endl;
            }
            });

        // - 将接收到的json数据转换为字符串，并写入到一个json文件中

        // 创建一个输出文件流对象，打开一个名为data.json的文件，如果不存在则创建它，如果存在则追加内容
        std::ofstream ofs("data.json", std::ios_base::app);

        // 判断文件是否打开成功，否则打印错误信息并返回
        if (!ofs.is_open()) {
            std::cerr << "Error: failed to open file" << std::endl;
            return;
        }

        // 将json数据转换为字符串，并添加一个换行符
        std::string output = data.dump() + "\n";

        // 将字符串写入到文件中，并关闭文件流对象
        ofs << output;
        ofs.close();

        // - 将hidden参数的值转换为字符串，并替换原来的hidden属性的值

        // 调用dump()方法，将hidden参数的值转换为字符串，并赋给一个新变量hidden_str
        std::string hidden_str = data["hidden"].dump();

        // 将hidden_str作为属性值替换原来的hidden属性的值
        data["hidden"] = hidden_str;

    }
    else {
        response["success"] = false;
        response["error"] = "Missing required fields."; // 缺少需要的内容
        // 将json对象转换为字符串，并设置为响应的body
        res.body() = response.dump();
        std::cout << "Send json data: " << response << std::endl;
    }

}


// 处理获取json数据的请求的函数
void handle_get_json_request(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 创建一个json对象，作为响应内容
    json response;
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 从json数据中获取version参数，并转换为字符串类型
    std::string version = data.at("version").get<std::string>();

    // 从json数据中获取version参数，并转换为字符串类型
    // 检查data中是否有"version"键，如果没有，直接返回一个错误信息给请求者
    if (data.contains("version")) {
        std::string version = data.at("version").get<std::string>();
        // 判断version参数是否为VGameSDK008，如果是，则设置success为true，否则设置success为false和error信息
        if (version == "VGameSDK008") {
            response["success"] = true;
            io_context.run();
            std::cout << json_data << std::endl;
        }
        else {
            response["success"] = false;
            response["error"] = "Your SDK version is unsupported, please update the SDK.";
            io_context.run();
            std::cout << json_data << std::endl;
        }
    }
    else {
        response["success"] = false;
        response["error"] = "Missing field.";
        io_context.run();
        std::cout << json_data << std::endl;
    }


    // 将存储的json数据作为servers字段的值
    response["servers"] = json_data;
    // 遍历返回结果中的servers字段的每个元素
    for (auto& j : response["servers"]) {
        // 调用dump()方法，将hidden参数的值转换为字符串，并赋给一个新变量hidden_str
        std::string hidden_str = j["hidden"].dump();
        // 将hidden_str作为属性值替换原来的hidden属性的值
        j["hidden"] = hidden_str;
    }
    // 将json对象转换为字符串，并设置为响应的body
    res.body() = response.dump();
}

//主函数
int main() {
    try {
        // 创建一个io_context对象，用于管理异步操作
        asio::io_context io;
        // 创建一个ssl_context对象，用于管理SSL/TLS加密相关的设置
        asio::ssl::context ssl_ctx{ asio::ssl::context::sslv23 };
        // 加载证书文件和私钥文件，这里假设文件名分别为Test.crt和Test.key
        ssl_ctx.use_certificate_chain_file("C:\\Users\\95741\\source\\repos\\diaoyugan\\r5rcn_master-server\\r5rcn-masterserver\\x64\\Debug\\Test.crt");
        ssl_ctx.use_private_key_file("C:\\Users\\95741\\source\\repos\\diaoyugan\\r5rcn_master-server\\r5rcn-masterserver\\x64\\Debug\\Test.key", asio::ssl::context::pem);
        // 创建一个ip地址对象，表示监听的地址，这里使用ipv4的回环地址
        asio::ip::address address = asio::ip::make_address("127.0.0.1");
        // 创建一个端口号对象，表示监听的端口，这里使用37020端口
        unsigned short port = static_cast<unsigned short>(37020);
        // 创建一个endpoint对象，表示监听的地址和端口的组合
        asio::ip::tcp::endpoint endpoint{ address, port };
        // 创建一个acceptor对象，用于接受客户端的连接请求
        asio::ip::tcp::acceptor acceptor{ io, endpoint };
        // 等待客户端的连接请求
        std::cout << "Listening on " << endpoint << std::endl;
        while (1) // 用一个循环来处理多个请求
        {
            try { // 使用try-catch语句来捕获可能发生的异常
                asio::ip::tcp::socket socket{ io };
                acceptor.accept(socket);
                std::cout << "Accepted connection from " << socket.remote_endpoint() << std::endl;
                // 创建一个stream对象，用于进行SSL/TLS加密的读写操作
                asio::ssl::stream<asio::ip::tcp::socket> stream{ std::move(socket), ssl_ctx };
                // 设置SSL/TLS握手的验证模式和回调函数，以处理可能的证书错误
                //这边暂时禁用了证书验证
                stream.set_verify_mode(asio::ssl::verify_none);
                //                stream.set_verify_callback(asio::ssl::rfc2818_verification("www.google.de"));
                                // 执行SSL/TLS握手操作
                stream.handshake(asio::ssl::stream_base::server);
                // 创建一个缓冲区对象，用于存储接收到的数据
                beast::flat_buffer buffer;
                // 创建一个请求对象，用于解析接收到的HTTP请求
                http::request<http::string_body> req;
                // 读取HTTP请求，并将其存储到请求对象中
                http::read(stream, buffer, req);
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
                http::write(stream, res);
                beast::error_code ec;
                stream.shutdown(ec);
                if (ec) { // 检查是否有错误发生，并打印错误信息
                    std::cerr << "Error: " << ec.message() << std::endl;
                }
                socket.close(ec); // 关闭socket对象，释放文件描述符，并检查是否有错误发生，并打印错误信息
                if (ec) {
                    std::cerr << "Error: " << ec.message() << std::endl;
                }
            }
            catch (std::exception& e) { // 在catch块中打印异常信息
                std::cerr << "Exception: " << e.what() << std::endl;
            }
            io.run(); // 运行io_context对象直到所有异步操作完成
            io_context.run();

        }
        acceptor.close(); // 在服务器终止时关闭acceptor对象

    }
    catch (std::exception& e) { // 在catch块中打印异常信息
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
