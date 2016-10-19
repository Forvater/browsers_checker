#ifndef REGISTRY_HELPER_H
#define REGISTRY_HELPER_H

#include <windows.h>
#include <string>
#include <list>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

class RegistryHelper {
    public:
        HKEY open_key(std::string a_registry_path, unsigned int a_key_mode);
        long close_ley(HKEY a_key);
        std::list<std::string> create_subkeys_list(HKEY a_key);
        std::string get_string_value(HKEY a_key, std::string a_value);
};


#endif //REGISTRY_HELPER_H
