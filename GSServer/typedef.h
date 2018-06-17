#pragma once

class Object;
class Client;
using ObjectMap = std::unordered_map<unsigned int, std::unique_ptr<Object>>;
using ULock = std::unique_lock<std::mutex>;
