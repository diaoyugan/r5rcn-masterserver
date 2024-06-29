#include "include\inc.hpp"
#include "include\ban_list.hpp"
#include "include\servers_list.hpp"
#include "include\file_system.hpp"

std::atomic<bool> keepRunning{ true };

void ioThread(asio::io_context& io) {
    while (keepRunning) {
        io.restart();
        io.run();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << get_time() << "Try To Delete Timeout Server\n"; // 输出提示信息
    }
}

// 处理客户端连接的函数
void handle_client(asio::ip::tcp::socket socket, asio::ssl::context& ssl_ctx) {
    try {
        // 创建一个stream对象，用于进行SSL/TLS加密的读写操作
        asio::ssl::stream<asio::ip::tcp::socket> stream{ std::move(socket), ssl_ctx };

        if (enable_verifi) {
            stream.set_verify_mode(asio::ssl::verify_peer);
            stream.set_verify_callback(asio::ssl::rfc2818_verification(cert_verifi));
        }
        else {
            stream.set_verify_mode(asio::ssl::verify_none);
        }

        // 执行SSL/TLS握手操作
        std::cout << "Starting async handshake\n";
        stream.handshake(asio::ssl::stream_base::server);

        // 创建一个缓冲区对象，用于存储接收到的数据
        beast::flat_buffer buffer;
        // 创建一个请求对象，用于解析接收到的HTTP请求
        http::request<http::string_body> req;

        // 读取HTTP请求，并将其存储到请求对象中
        http::read(stream, buffer, req);

        std::cout << get_time() << "Received request: " << req << std::endl;

        // 创建一个响应对象，用于发送HTTP响应
        http::response<http::string_body> res;

        // 判断请求的目标并调用相应的处理函数
        if (req.target() == "/servers") {
            handle_get_servers_list_request(req, res);
        }
        else if (req.target() == "/eula") {
            handle_check_eula(req, res);
        }
        else if (req.target() == "/servers/add") {
            std::string ip_address = stream.lowest_layer().remote_endpoint().address().to_string();
            handle_create_server_request(req, res, ip_address);
        }
        else if (req.target() == "/server/byToken") {
            handle_get_server_byToken(req, res);
        }
        else if (req.target() == "/banlist/isBanned") {
            handle_banlist_check_banned(req, res);
        }
        else if (req.target() == "/banlist/bulkCheck") {
            handle_banlist_bulkCheck(req, res);
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
        if (ec) {
            std::cerr << get_time() << "Error: " << ec.message() << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << get_time() << "Exception: " << e.what() << std::endl;
    }
}

// 主函数
int main() {
    // 获取程序所在的目录
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH); // 注意使用GetModuleFileNameA

    // 从路径中提取目录部分
    std::string path(buffer);
    size_t found = path.find_last_of("\\/");
    std::string directory = path.substr(0, found);

    // 设置当前工作目录为ANSI字符串
    if (SetCurrentDirectoryA(directory.c_str())) { // 注意使用SetCurrentDirectoryA
        std::cout << "The current working directory has been set to the program's location: " << directory << std::endl;
    }
    else {
        std::cerr << "Failed to set the current working directory: " << GetLastError() << std::endl;
    }

    std::string filename = "settings.txt"; // 设置文件的名字
    std::ifstream test(filename); // 尝试打开文件
    if (test.good()) { // 检查文件是否存在
        test.close(); // 关闭文件
        read_settings(filename); // 从文件中读取设置
    }
    else {
        test.close(); // 关闭文件
        create_default_settings(filename); // 生成一个默认的设置文件
        std::cout << get_time() << "A default settings file has been created.\n"; // 输出提示信息
        read_settings(filename); // 从文件中读取设置
    }

    try {
        // 创建一个io_context对象，用于管理异步操作
        asio::io_context io;
        // 创建一个ssl_context对象，用于管理SSL/TLS加密相关的设置
        asio::ssl::context ssl_ctx{ asio::ssl::context::sslv23 };
        // 加载证书文件和私钥文件，这里从全局变量获取
        ssl_ctx.use_certificate_chain_file(cert_file);
        ssl_ctx.use_private_key_file(key_file, asio::ssl::context::pem);
        // 创建一个ip地址对象，表示监听的地址
        asio::ip::address address = asio::ip::make_address(listen_address);
        // 创建一个端口号对象，表示监听的端口
        unsigned short port = static_cast<unsigned short>(listen_port);
        // 创建一个endpoint对象，表示监听的地址和端口的组合
        asio::ip::tcp::endpoint endpoint{ address, port };
        // 创建一个acceptor对象，用于接受客户端的连接请求
        asio::ip::tcp::acceptor acceptor{ io, endpoint };
        // 等待客户端的连接请求
        std::cout << "Listening on " << endpoint << std::endl;

        std::thread io_thread(ioThread, std::ref(io));

        while (1) { // 用一个循环来处理多个请求
            try { // 使用try-catch语句来捕获可能发生的异常
                asio::ip::tcp::socket socket{ io };
                acceptor.accept(socket);
                std::cout << get_time() << "Accepted connection from " << socket.remote_endpoint() << std::endl;

                // 每个新连接启动一个新线程来处理
                std::thread(handle_client, std::move(socket), std::ref(ssl_ctx)).detach();
            }
            catch (std::exception& e) { // 在catch块中打印异常信息
                std::cerr << get_time() << "Exception: " << e.what() << std::endl;
            }
        }

        acceptor.close(); // 在服务器终止时关闭acceptor对象

    }
    catch (std::exception& e) { // 在catch块中打印异常信息
        std::cerr << get_time() << "Error: " << e.what() << std::endl;
    }
    return 0;
}
