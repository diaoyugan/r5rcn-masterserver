#include "include\inc.hpp"
#include "include\ban_list.hpp"
#include "include\servers_list.hpp"
#include "include\file_system.hpp"


std::atomic<bool> keepRunning{ true };

void ioThread(asio::io_context& io) {
    while (keepRunning) {
        io_pcontext.run();
        io_context.run();
        cout << get_time() << "Try To Delete Timeout Server\n"; // 输出提示信息
        io.restart();
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void handle_request(asio::ssl::stream<tcp::socket>& ssl_stream, asio::yield_context yield) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;

        http::async_read(ssl_stream, buffer, req, yield);

        if (req.find(http::field::user_agent) != req.end()) {
            http::response<http::string_body> res;
            res.version(11);
            res.result(http::status::forbidden);
            res.set(http::field::content_type, "text/plain");
            res.body() = "Access forbidden";
            res.prepare_payload();

            http::async_write(ssl_stream, res, yield);
            ssl_stream.lowest_layer().shutdown(tcp::socket::shutdown_send);
            return;
        }

        http::response<http::string_body> res;
        if (req.target() == "/eula") {
            handle_check_eula(req, res);
        }
        else if (req.target() == "/servers") {
            handle_get_servers_list_request(req, res);
        }
        else if (req.target() == "/servers/add") {
            std::string ip_address = ssl_stream.lowest_layer().remote_endpoint().address().to_string();
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
            res.version(11);
            res.result(http::status::not_found);
            res.set(http::field::content_type, "text/plain");
            res.body() = "The requested resource was not found";
            res.prepare_payload();
        }

        http::async_write(ssl_stream, res, yield);
        ssl_stream.lowest_layer().shutdown(tcp::socket::shutdown_send);
    }
    catch (std::exception& e) {
        std::cerr << "Request handling error: " << e.what() << std::endl;
    }
}

void start_accept(tcp::acceptor& acceptor, asio::ssl::context& ssl_ctx, asio::io_context& io_context) {
    auto socket = std::make_shared<tcp::socket>(io_context);
    acceptor.async_accept(*socket, [&acceptor, &ssl_ctx, &io_context, socket](const boost::system::error_code& error) {
        if (!error) {
            // 为握手超时时间创建deadline timer
            auto timer = std::make_shared<asio::steady_timer>(io_context, std::chrono::seconds(10)); // 设置超时时间为十秒

            asio::spawn(io_context, [socket, &ssl_ctx, timer](asio::yield_context yield) mutable {
                asio::ssl::stream<tcp::socket> ssl_stream(std::move(*socket), ssl_ctx);

                if (enable_verifi) {
                    ssl_stream.set_verify_mode(asio::ssl::verify_peer);
                    ssl_stream.set_verify_callback(asio::ssl::rfc2818_verification(cert_verifi));
                }
                else {
                    ssl_stream.set_verify_mode(asio::ssl::verify_none);
                }

                try {
                    // Async handshake with timeout
                    std::cout << "Starting async handshake" << std::endl;
                    ssl_stream.async_handshake(asio::ssl::stream_base::server, yield);

                    // Cancel the timer as handshake is successful
                    timer->cancel();

                    std::cout << "Handshake successful" << std::endl;

                    handle_request(ssl_stream, yield);
                }
                catch (std::exception& e) {
                    std::cerr << "Handshake error: " << e.what() << std::endl;
                }
                });

            // Timer handler for handshake timeout
            timer->async_wait([socket](const boost::system::error_code& ec) {
                if (!ec) {
                    std::cerr << "Handshake timed out" << std::endl;
                    socket->close(); // Close the socket due to timeout
                }
                else if (ec != asio::error::operation_aborted) {
                    std::cerr << "Timer error: " << ec.message() << std::endl;
                }
                });
        }
        else {
            std::cerr << "Accept error: " << error.message() << std::endl;
        }

        start_accept(acceptor, ssl_ctx, io_context);
        });
}


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
    string filename = "settings.txt"; // 设置文件的名字
    ifstream test(filename); // 尝试打开文件
    if (test.good()) { // 检查文件是否存在
        test.close(); // 关闭文件
        read_settings(filename); // 从文件中读取设置
    }
    else {
        test.close(); // 关闭文件
        create_default_settings(filename); // 生成一个默认的设置文件
        cout << get_time() << "A default settings file has been created.\n"; // 输出提示信息
        read_settings(filename); // 从文件中读取设置
    }
    std::cout << "Settings read complete" << std::endl;
    std::cout << "Listening:" << listen_address <<":"<< listen_port << std::endl;
    try {
        asio::io_context io;
        asio::ssl::context ssl_ctx{ asio::ssl::context::sslv23 };

        // 加载证书文件和私钥文件，这里从全局变量获取
        ssl_ctx.use_certificate_chain_file(cert_file);
        ssl_ctx.use_private_key_file(key_file, asio::ssl::context::pem);

        asio::ip::tcp::acceptor acceptor(io, tcp::endpoint(asio::ip::make_address(listen_address), listen_port));
        start_accept(acceptor, ssl_ctx, io);



        io.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}

