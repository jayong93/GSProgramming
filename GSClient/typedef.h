#pragma once

struct Object;
struct Client;
using ObjectMap = std::unordered_map<unsigned int, std::unique_ptr<Object>>;
