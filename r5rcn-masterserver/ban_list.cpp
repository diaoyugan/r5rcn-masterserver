#include "include\ban_list.hpp"
#include "include\file_system.hpp"

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
    std::cout << get_time() << "Send banlist check data: " << response << std::endl;
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
    std::cout << get_time() << "Send banlist bulkCheck data: " << response << std::endl;
}