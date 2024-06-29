#pragma once
#include "inc.hpp"

// 声明设置的变量
extern string banlist_file;
extern string eula_file;
extern string cert_file;
extern string key_file;
extern bool enable_verifi;
extern string cert_verifi;
extern string listen_address;
extern string SDK_version;
extern int listen_port;

extern string get_time();
extern void store_line(string key, string value);
extern void read_settings(string filename);
extern void create_default_settings(string filename);
extern json read_banlist();
extern json read_eula();