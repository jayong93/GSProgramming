#pragma once

struct Object;
using ObjectMap = std::unordered_map<unsigned int, std::unique_ptr<Object>>;
