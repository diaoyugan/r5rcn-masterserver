#include "include\file_system.hpp"

std::string banlist_file;
std::string eula_file;
std::string cert_file;
std::string key_file;
bool enable_verifi;
std::string cert_verifi;
std::string listen_address;
std::string SDK_version;
int listen_port;

string get_time() {
    time_t timep;
    time(&timep); //获取time_t类型的当前时间
    char tmp[64];
    strftime(tmp, sizeof(tmp), "[%Y-%m-%d %H:%M:%S]  ", localtime(&timep));//对日期和时间进行格式化
    return tmp;
}

// 定义一个函数，根据key和value来给变量赋值
void store_line(string key, string value) {
    if (key == "banlist_file") {
        banlist_file = value;
    }
    else if (key == "eula_file") {
        eula_file = value;
    }
    else if (key == "cert_file") {
        cert_file = value;
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
    else if (key == "SDK_version") {
        SDK_version = value;
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
        cout << get_time() << "Unable to open file: " << filename << endl;
    }
}

// 定义一个函数，生成一个默认的设置文件
void create_default_settings(string filename) {
    ofstream file(filename); // 创建一个文件流对象
    if (file.is_open()) { // 检查文件是否打开成功
        file << "#如果你在windows系统上要使用反斜线，请务必打两个，不然你懂的 对了，可以用绝对路径或者相对路径\n";
        file << "#本配置文件只会在ms的工作目录生成和读取 请留意\n";
        file << "#banlist.json文件会在第一次需要使用时生成，你可以自己创建 和本文件相同 在工作目录生成和读取\n";
        file << " #eula不会自己生成 务必注意！！！\n";
        file << "\n";
        file << "\n";
        file << "#请求者的SDK版本 用于判断对方客户端或服务端是否需要更新\n";
        file << "SDK_version=VGameSDK009\n";
        file << "\n";
        file << "#封禁列表文件\n";
        file << "banlist_file=banlist.json\n";
        file << "\n";
        file << "#EULA文件\n";
        file << "eula_file=eula.json\n";
        file << "\n";
        file << "#证书文件\n";
        file << "cert_file=cert.crt\n";
        file << "\n";
        file << "#证书私钥\n";
        file << "key_file=cert.key\n";
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
        cout << get_time() << "Unable to create file: " << filename << endl;
    }
}

// 定义一个函数，用于读取banlist文件并返回一个json对象
json read_banlist() {
    ifstream input(banlist_file); // 打开文件
    if (!input) { // 检查文件是否存在
        cerr << get_time() << "Error: banlist file: " << banlist_file << " not found" << endl;
        cerr << get_time() << "Createing " << banlist_file << endl;
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

// 定义一个函数，用于读取eula文件并返回一个json对象
json read_eula() {
    ifstream input(eula_file); // 打开文件
    if (!input) { // 检查文件是否存在
        cerr << get_time() << "Error: eula file: " << eula_file << " not found" << endl;
        cerr << get_time() << "Createing " << eula_file << endl;
        ofstream file(eula_file); // 创建一个文件流对象
        if (file.is_open()) { // 检查文件是否打开成功
            file << "{";
            file << "}";
            file.close(); // 关闭文件
        }
    }
    json eula; // 创建一个json对象
    input >> eula; // 从文件中读取json数据
    input.close(); // 关闭文件
    return eula; // 返回json对象
}