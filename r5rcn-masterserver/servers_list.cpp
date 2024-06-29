#include "include\servers_list.hpp"
#include "include\file_system.hpp"

// 创建一个全局变量，用于存储添加的服务器数据
std::vector<json> json_data;
// 创建一个全局变量，用于存储生成的服务器token以及其服务器
std::vector<json> json_token;

// 创建一个io_context对象
boost::asio::io_context io_context;

// 创建一个io_context对象
boost::asio::io_context io_pcontext;

//我还是害怕会冲突
boost::asio::deadline_timer pglobal_timer(io_pcontext);
// 创建一个全局变量，用于存储定时器对象
// 改名为global_timer，避免与main函数中的局部变量冲突
boost::asio::deadline_timer global_timer(io_context);

// 添加私人服务器的函数
void add_private_server(http::request<http::string_body>& req, http::response<http::string_body>& res, std::string ip_address) {
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 打印json数据
    std::cout << get_time() << "Received private server data: " << data << std::endl;
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
    std::cout << get_time() << "Sendback create server result: " << response << std::endl;

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
                std::cout << get_time() << "Deleted private server data: " << *it << std::endl;
                json_token.erase(it, json_token.end());
                std::cout << json_token << std::endl;
            }
        }
        else { // 有错误发生，并打印错误信息
            std::cerr << get_time() << "Error: " << ec.message() << std::endl;
        }
        });
}



// 处理创建服务器的请求的函数
void handle_create_server_request(http::request<http::string_body>& req, http::response<http::string_body>& res, std::string ip_address) {
    string ip = ip_address;
    // 从请求中获取json数据
    json data = json::parse(req.body());
    // 打印json数据
    std::cout << get_time() << "Received server data: " << data << std::endl;
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
            add_private_server(req, res, ip);
        }
        else

            // 判断no_response是否为真，否则按照原来的逻辑设置响应内容
            if (no_response) {
                response["success"] = true;
                res.body() = response.dump();
                //        std::cout << get_time() << "Send server data: " << response << std::endl;

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
                        std::cout << get_time() << "Timer expired" << std::endl;
                        // 获取当前时间戳，使用long long类型来表示
                        long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                        // 在存储的数据中查找超过3秒没有更新过的内容，并删除
                        auto it = std::remove_if(json_data.begin(), json_data.end(), [now](const json& j) {
                            return now - j["timeStamp"].get<long long>() > 3000; // 使用timeStamp参数来判断是否超时
                            });

                        if (it != json_data.end()) {
                            std::cout << get_time() << "Deleted server data: " << *it << std::endl;
                            json_data.erase(it, json_data.end());
                            std::cout << json_data << std::endl;
                        }
                    }
                    else { // 有错误发生，并打印错误信息
                        std::cerr << get_time() << "Error: " << ec.message() << std::endl;
                    }
                    });

                response["success"] = true;
                response["token"] = nullptr; // 没有token
                // 将json对象转换为字符串，并设置为响应的body
                res.body() = response.dump();
                std::cout << get_time() << "Sendback create server result: " << response << std::endl;

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
        std::cout << get_time() << "Sendback create server result: " << response << std::endl;
    }

}

void handle_get_servers_list_request(http::request<http::string_body>& req, http::response<http::string_body>& res) {
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

    // 检查data中是否有"version"键，如果没有，直接返回一个错误信息给请求者
    if (data.contains("version")) {
        std::string version = data.at("version").get<std::string>();
        // 判断version参数是否为来自设置文件的SDK_version，如果是，则设置success为true，否则设置success为false和error信息
        if (version == SDK_version) {
            response["success"] = true;
        }
        else {
            response["success"] = false;
            response["error"] = "Your SDK version is unsupported, please update the SDK.";
        }
    }
    else {
        response["success"] = false;
        response["error"] = "Missing field.";
    }

    // 获取服务器列表
    json servers = json_data;

    // 按playerCount字段从大到小排序
    std::sort(servers.begin(), servers.end(), [](const json& a, const json& b) {
        int playerCountA = std::stoi(a["playerCount"].get<std::string>());
        int playerCountB = std::stoi(b["playerCount"].get<std::string>());
        return playerCountA > playerCountB;
        });

    // 将排序后的服务器列表存储到响应结果中
    response["servers"] = servers;
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
    std::cout << get_time() << "request token:" << token << std::endl;
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