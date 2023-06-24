// 这个版本使用SSL/TLS加密，支持https协议


#include <iostream>
#include <fstream>
#include <vector>
#include <string> 
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>


namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;
using namespace std;

// 声明设置的变量
string banlist_file;
string cret_file;
string key_file;
bool enable_verifi;
string cert_verifi;
string listen_address;
int listen_port;

// 创建一个全局变量，用于存储添加的服务器数据
std::vector<json> json_data;
// 创建一个全局变量，用于存储生成的服务器token以及其服务器
std::vector<json> json_token;

// 创建一个io_context对象
boost::asio::io_context io_context;
// 创建一个全局变量，用于存储定时器对象
// 改名为global_timer，避免与main函数中的局部变量冲突
boost::asio::deadline_timer global_timer(io_context);

// 创建一个io_context对象
boost::asio::io_context io_pcontext;
//我还是害怕会冲突
boost::asio::deadline_timer pglobal_timer(io_pcontext);

// 定义一个函数，根据key和value来给变量赋值
void store_line(string key, string value) {
    if (key == "banlist_file") {
        banlist_file = value;
    }
    else if (key == "cret_file") {
        cret_file = value;
    }
    else if (key == "key_file") {
        key_file = value;
    }
    else if (key == "enable_verifi") {
        enable_verifi = (value == "true");
    }
    else if (key == "cert_verifi") {
        cert_verifi = value;
    }
    else if (key == "listen_address") {
        listen_address = value;
    }
    else if (key == "listen_port") {
        listen_port = stoi(value);
    }
    else {
        cout << "Unknown setting: " << key << endl;
    }
}

// 定义一个函数，从文件中读取设置
void read_settings(string filename) {
    ifstream file(filename); // 创建一个文件流对象
    if (file.is_open()) { // 检查文件是否打开成功
        string line; // 用来存储每一行的内容
        while (getline(file, line)) { // 循环读取每一行
            istringstream is_line(line); // 创建一个字符串流对象
            string key; // 用来存储键名
            if (getline(is_line, key, '=')) { // 用等号作为分隔符，读取键名
                string value; // 用来存储键值
                if (getline(is_line, value)) { // 读取键值
                    if (!key.empty() && key[0] != '#') { // 检查键名是否为空或以#开头，如果是则忽略这一行
                        store_line(key, value); // 调用函数，给变量赋值
                    }
                }
            }
        }
        file.close(); // 关闭文件
    }
    else {
        cout << "Unable to open file: " << filename << endl;
    }
}

// 定义一个函数，生成一个默认的设置文件
void create_default_settings(string filename) {
    ofstream file(filename); // 创建一个文件流对象
    if (file.is_open()) { // 检查文件是否打开成功
        file << "#如果要使用反斜线 请务必打两个 不然你懂的 对了 可以用绝对路径或者相对路径\n";
        file << "\n";
        file << "\n";
        file << "#封禁列表文件\n";
        file << "banlist_file=banlist.json\n"; 
        file << "\n";
        file << "#证书文件\n";
        file << "cret_file=cret.crt\n";
        file << "\n";
        file << "#证书私钥\n";
        file << "key_file=cret.key\n";
        file << "\n";
        file << "#是否开启证书验证 true为开 false为关\n";
        file << "enable_verifi=true\n";
        file << "\n";
        file << "#验证证书的地址（证书的地址） 如果你没开验证 可以忽略这一行\n";
        file << "cert_verifi=ms.example.com\n";
        file << "\n";
        file << "#监听ip 一般不用改\n";
        file << "listen_address=0.0.0.0\n";
        file << "\n";
        file << "#监听端口 随你 我推荐443 不然sdk那边可不好搞\n";
        file << "listen_port=443\n";
        file.close(); // 关闭文件
    }
    else {
        cout << "Unable to create file: " << filename << endl;
    }
}

// 添加私人服务器的函数
void add_private_server(http::request<http::string_body>& req, http::response<http::string_body>& res, std::string ip_address) {
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 打印json数据
    std::cout << "Received private server data: " << data << std::endl;
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    json response;

    // 使用boost::uuids库来生成唯一的token，并将其转换为字符串
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::string token = boost::uuids::to_string(uuid);

    // - 在存储的数据中查找是否有与接收到的数据相匹配的内容，并更新或添加

// 定义一个标志变量，表示是否找到匹配的内容
    bool found = false;
    //声明一下后面要使用的restoken
    std::string restoken = "";

    // 遍历存储的数据中的每个元素
    for (auto& j : json_token) {
        // 定义一个标志变量，表示是否所有指定的参数都相等
        bool equal = true;

        // 遍历指定的参数列表，并比较每个参数的值是否相等
        for (const auto& param : { "cachedId", "checksum", "description", "hidden", "key", "map", "maxPlayers", "name", "playlist", "port", "publicRef", "version" }) {
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
            j["ip"] = ip_address;
            restoken = j["token"];
            found = true;
            break;
            std::cout << json_token << std::endl;
        }
    }

    // 如果没有找到匹配的内容，则将接收到的数据添加到存储中
    if (!found) {
        data["token"] = token;
        json_token.push_back(data);
        std::cout << json_token << std::endl;
    }

    // 将token作为响应的内容返回给客户端
    response["success"] = true;
    response["token"] = restoken;
    res.body() = response.dump();
    std::cout << "Sendback create server result: " << response << std::endl;

    // 设置定时器的超时时间为1秒，并将io_context对象作为参数传递给构造函数
    pglobal_timer.expires_from_now(boost::posix_time::seconds(1));

    // 设置定时器到期时执行的回调函数，删除存储的数据中超过3秒没有更新过的内容
    pglobal_timer.async_wait([](const boost::system::error_code& ec) {
        if (!ec) { // 没有错误发生
            std::cout << "Timer expired" << std::endl;
            // 获取当前时间戳，使用long long类型来表示
            long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); 
            //在存储的数据中查找超过3秒没有更新过的内容，并删除
            auto it = std::remove_if(json_token.begin(), json_token.end(), [now](const json& j) {
                return now - j["timeStamp"].get<long long>() > 3000; // 使用timeStamp参数来判断是否超时
                });

            if (it != json_token.end()) {
                std::cout << "Deleted json data: " << *it << std::endl;
                json_token.erase(it, json_token.end());
                std::cout << json_token << std::endl;
            }
        }
        else { // 有错误发生，并打印错误信息
            std::cerr << "Error: " << ec.message() << std::endl;
        }
        });
}



// 处理添加json数据的请求的函数
void handle_add_json_request(http::request<http::string_body>& req, http::response<http::string_body>& res, std::string ip_address) {
    string ip = ip_address;
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

    // 从json数据中获取hidden参数
// 检查data中是否有"hidden"键
    if (data.contains("hidden")) {
        bool hidden = data.at("hidden");
        // 判断hidden参数是否为true，如果是，则使其添加到私人服务器系统，否则添加到原本的服务器列表
        if (hidden == true) {
            add_private_server(req, res,ip);
        }
        else
  
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
            for (const auto& param : { "cachedId", "checksum", "description", "hidden", "key", "map", "maxPlayers", "name", "playlist", "port", "publicRef", "version" }) {
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
                j["ip"] = ip;
                found = true;
                break;
            }
        }

        // 如果没有找到匹配的内容，则将接收到的数据添加到存储中
        if (!found) {
            json_data.push_back(data);
        }

        // - 使用global_timer代替timer，以使用全局变量而不是局部变量

        // 取消之前的定时器，避免重复执行回调函数
//        global_timer.cancel();

        // 设置定时器的超时时间为1秒，并将io_context对象作为参数传递给构造函数
        global_timer.expires_from_now(boost::posix_time::seconds(1));

        // 设置定时器到期时执行的回调函数，删除存储的数据中超过3秒没有更新过的内容
        global_timer.async_wait([](const boost::system::error_code& ec) {
            if (!ec) { // 没有错误发生
                std::cout << "Timer expired" << std::endl;
                // 获取当前时间戳，使用long long类型来表示
                long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                // 在存储的数据中查找超过3秒没有更新过的内容，并删除
                auto it = std::remove_if(json_data.begin(), json_data.end(), [now](const json& j) {
                    return now - j["timeStamp"].get<long long>() > 3000; // 使用timeStamp参数来判断是否超时
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

        response["success"] = true;
        response["token"] = nullptr; // 没有token
        // 将json对象转换为字符串，并设置为响应的body
        res.body() = response.dump();
        std::cout << "Sendback create server result: " << response << std::endl;

        // - 将hidden参数的值转换为字符串，并替换原来的hidden属性的值

        // 调用dump()方法，将hidden参数的值转换为字符串，并赋给一个新变量hidden_str
        std::string hidden_str = data["hidden"].dump();

        // 将hidden_str作为属性值替换原来的hidden属性的值
        data["hidden"] = hidden_str;

    }
    }
    else {
        response["success"] = false;
        response["error"] = "Missing required fields."; // 缺少需要的内容
        // 将json对象转换为字符串，并设置为响应的body
        res.body() = response.dump();
        std::cout << "Sendback create server result: " << response << std::endl;
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
            std::cout << json_data << std::endl;
        }
        else {
            response["success"] = false;
            response["error"] = "Your SDK version is unsupported, please update the SDK.";
            std::cout << json_data << std::endl;
        }
    }
    else {
        response["success"] = false;
        response["error"] = "Missing field.";
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


// 定义一个函数，用于读取banlist文件并返回一个json对象
json read_banlist() {
    ifstream input(banlist_file); // 打开文件
    if (!input) { // 检查文件是否存在
        cerr << "Error: cannot open " << banlist_file << endl;
        ofstream file(banlist_file); // 创建一个文件流对象
        if (file.is_open()) { // 检查文件是否打开成功
            file << "{";
            file << "}";
            file.close(); // 关闭文件
        }
    }
    json banlist; // 创建一个json对象
    input >> banlist; // 从文件中读取json数据
    input.close(); // 关闭文件
    return banlist; // 返回json对象
}


//处理访问管理封禁系统的函数
void handle_banlist_system_access(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 创建一个json对象，作为响应内容
    json response;
    // 从请求中获取json数据
    json data = json::parse(req.body());
}

//检查玩家是否被封禁的函数
void handle_banlist_check_banned(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 创建一个json对象，作为响应内容
    json response;
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 从json数据中获取id参数，并转换为字符串类型
    std::string id = data.at("id").dump();
    // 从json数据中获取id参数
// 读取banlist.json文件，并转换为一个json对象
    json banlist = read_banlist();

    // 检查data中是否有id键，如果没有，直接返回一个错误信息给请求者
    if (data.contains("id")) {
        std::string id = data.at("id").dump();
        // 判断uid参数是否在封禁列表中存在，如果存在id参数且和列表匹配 返回success为true 且banned为true 追加reason参数(从封禁列表读取)
        if (banlist.contains(id)) { // 如果banlist中有与id相同的键
            response["success"] = true; // 设置success为true
            response["banned"] = true; // 设置banned为true
            response["reason"] = banlist.at(id); // 设置reason为banlist中对应的值
        }
        else { // 如果banlist中没有与uid相同的键
            response["success"] = true; // 设置success为true
            response["banned"] = false; // 设置banned为false
        }
    }
    else {
        response["success"] = false;
        response["error"] = "Missing player uid.";
    }
    // 将json对象转换为字符串，并设置为响应的body
    res.body() = response.dump();
    std::cout << "Send banlist check data: " << response << std::endl;
}

//批量检查玩家是否被封禁的函数
void handle_banlist_bulkCheck(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.result(http::status::ok); // 200 OK
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 创建一个json对象，作为响应内容
    json response;
    // 从请求中获取json数据
    json data = json::parse(req.body());
    json players = data.at("players");
    // 创建一个新的json数组，用于存储被封禁玩家信息
    // 使用json::array()函数来指定数组类型
    json banned_players = json::array(); 
    // 遍历数组中的每个元素，它们应该是对象类型的值，包含id和ip两个键
    for (auto& player : players) {
        // 从每个对象中获取id键对应的值，并转换为字符串类型
        std::string id = player.at("id").dump();
        // 读取banlist.json文件，并转换为一个json对象
        json banlist = read_banlist();
        // 检查banlist.json对象中是否有与id相同的键，如果有，就将该键和对应的值添加到新的json数组中
        if (banlist.contains(id)) {
            long long id_longlong = std::stoll(id);
            banned_players.push_back({ {"id", id_longlong}, {"reason", banlist.at(id)} });
        }
    }
    // 设置success键为true，并设置bannedPlayers键为新的json数组
    response["success"] = true;
    response["bannedPlayers"] = banned_players;

    // 将json对象转换为字符串，并设置为响应的body
    res.body() = response.dump();
    std::cout << "Send banlist bulkCheck data: " << response << std::endl;
}

// 根据token参数来查询私人服务器的函数
void handle_get_server_byToken(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // 创建一个响应对象
    res.version(11); // HTTP 1.1
    res.set(http::field::content_type, "application/json"); // 设置响应内容类型为json
    // 创建一个json对象，作为响应内容
    json response;
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 从json数据中获取token参数，并转换为字符串类型
    std::string token = data.at("token").dump();
    std::cout << "reqtoken:" << token << std::endl;
    std::cout << json_token << std::endl;
    // 在json_token中查找是否有与token参数相匹配的内容，并返回给客户端
    auto it = std::find_if(json_token.begin(), json_token.end(), [token](const json& j) {
        return j["token"].dump() == token; // 使用"token"作为属性名，并比较属性值是否相等
        });


    if (it != json_token.end()) { // 如果找到了匹配的内容
        res.result(http::status::ok); // 200 OK
        response["success"] = true;
        response["server"] = *it; // 将匹配的内容作为响应内容的server属性
        response["server"]["hidden"] = response["server"]["hidden"].dump(); // 将hidden属性的值转换为字符串类型
    }
    else { // 如果没有找到匹配的内容
        res.result(http::status::not_found); // 404 Not Found
        response["success"] = false;
        response["error"] = "No server found with the given token"; // 设置错误信息
    }

    res.body() = response.dump(); // 将响应内容转换为字符串并设置为响应体
}



//主函数
int main() {
    string filename = "settings.txt"; // 设置文件的名字
    ifstream test(filename); // 尝试打开文件
    if (test.good()) { // 检查文件是否存在
        test.close(); // 关闭文件
        read_settings(filename); // 从文件中读取设置
    }
    else {
        test.close(); // 关闭文件
        create_default_settings(filename); // 生成一个默认的设置文件
        cout << "A default settings file has been created.\n"; // 输出提示信息
        read_settings(filename); // 从文件中读取设置
    }
    try {
        // 创建一个io_context对象，用于管理异步操作
        asio::io_context io;
        // 创建一个ssl_context对象，用于管理SSL/TLS加密相关的设置
        asio::ssl::context ssl_ctx{ asio::ssl::context::sslv23 };
        // 加载证书文件和私钥文件，这里从全局变量获取
        ssl_ctx.use_certificate_chain_file(cret_file);
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

        while (1) // 用一个循环来处理多个请求
        {
            try { // 使用try-catch语句来捕获可能发生的异常
                asio::ip::tcp::socket socket{ io };
                acceptor.accept(socket);
                std::cout << "Accepted connection from " << socket.remote_endpoint() << std::endl;
                // 创建一个stream对象，用于进行SSL/TLS加密的读写操作
                asio::ssl::stream<asio::ip::tcp::socket> stream{ std::move(socket), ssl_ctx };
                // 设置SSL/TLS握手的验证模式和回调函数，以处理可能的证书错误

                if (enable_verifi){                //启用证书验证
                               stream.set_verify_mode(asio::ssl::verify_peer);
                               stream.set_verify_callback(asio::ssl::rfc2818_verification(cert_verifi));
                }
                else {
                    //禁用证书验证
                    stream.set_verify_mode(asio::ssl::verify_none);
                }


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
                }
                // 判断请求的目标是否为/servers/add，如果是，则调用处理添加json数据的请求的函数，否则继续判断
                else if (req.target() == "/servers/add") {
                    // 从请求中获取远程端点的地址和端口，并转换为字符串
                    std::string ip_address = stream.lowest_layer().remote_endpoint().address().to_string();
                    // 将ip_address作为参数传递给handle_add_json_request函数
                    handle_add_json_request(req, res, ip_address);
                }
                // 判断请求的目标是否为/banlist，如果是，则调用处理访问管理封禁系统的函数，否则继续判断
                //else if (req.target() == "/banlist") {
                //    handle_banlist_system_access(req, res);
                //}
                 // 判断请求的目标是否为/server/byToken，如果是，则调用处理访问通过token获取服务器的函数，否则继续判断
                else if (req.target() == "/server/byToken") {
                    handle_get_server_byToken(req, res);
                }
                // 判断请求的目标是否为/banlist/isBanned，如果是，则调用处理检查玩家是否被封禁的函数，否则继续判断
                else if (req.target() == "/banlist/isBanned") {
                    handle_banlist_check_banned(req, res);
                }
                // 判断请求的目标是否为/banlist/bulkCheck，如果是，则调用处理批量检查玩家是否被封禁的函数，否则返回404 Not Found错误
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
            io_pcontext.run();
        }

        // 创建一个thread对象，用于执行io_context.run()函数
        std::thread t{[&io]() { io_context.run(); }};

        // 定义一个尾递归的lambda表达式，用于每三秒调用一次io_context.run()和io_pcontext.run();函数
        std::function<void(std::chrono::time_point<std::chrono::steady_clock>)> run_io;
        run_io = [&](std::chrono::time_point<std::chrono::steady_clock> next_time) {
            // 调用io_context.run()函数
            io_context.run();
            // 调用io_pcontext.run()函数
            io_pcontext.run();
            // 使用std::this_thread::sleep_until函数，让当前线程休眠到下一次调用的时间
            std::this_thread::sleep_until(next_time);
            // 递归调用自身，并更新下一次调用的时间为三秒后，实现循环执行
            run_io(next_time + std::chrono::seconds(3));
        };

        // 调用一次lambda表达式，并传入当前时间作为参数，启动定时器
        run_io(std::chrono::steady_clock::now());

        // 定义一个互斥锁对象，用于保护条件变量
        std::mutex mtx;
        // 定义一个条件变量对象，用于让线程等待通知
        std::condition_variable cv;
        // 定义一个布尔变量，用于表示线程是否应该继续运行
        bool running = true;

        // 使用一个无限循环来保持线程运行
        while (true) {
            // 在这里可以做一些其他的事情，或者什么都不做
            // ...

            // 使用互斥锁和条件变量来让线程等待通知，或者直到running变为false
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&running]() { return !running; });
            if (!running) {
                break; // 如果running为false，跳出循环，结束线程
            }
        }


        acceptor.close(); // 在服务器终止时关闭acceptor对象

    }
    catch (std::exception& e) { // 在catch块中打印异常信息
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
