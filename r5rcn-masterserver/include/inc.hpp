// 这个版本使用SSL/TLS加密，支持https协议
#pragma once

#define _CRT_SECURE_NO_WARNINGS //VS中必须定义,否则报错
#include <atomic>
#include <iostream>
#include <fstream>
#include <vector>
#include <string> 
#include <chrono>
#include <thread>
#include <time.h>
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

