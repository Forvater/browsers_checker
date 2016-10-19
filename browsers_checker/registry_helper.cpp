#include "registry_helper.h"

//TODO::ADD HKEY_USERS
HKEY RegistryHelper::open_key(std::string a_registry_path, unsigned int a_key_mode) {
    HKEY hResultKey;
    HKEY hRootKey;
    std::string l_root = a_registry_path.substr(0, 4);
    std::string l_reg_path = a_registry_path.substr(6);
    if (l_root == "HKLM") {
        hRootKey = HKEY_LOCAL_MACHINE;
    }
    else if (l_root == "HKCU") {
        hRootKey = HKEY_CURRENT_USER;
    }
    else {
        return nullptr;
    }
    long reg_open_result_lm = RegOpenKeyEx(hRootKey, l_reg_path.c_str(), 0, KEY_READ | a_key_mode, &hResultKey);
    if (reg_open_result_lm == ERROR_SUCCESS) {
        return hResultKey;
    }
    else {
        return nullptr;
    }
}

long RegistryHelper::close_ley(HKEY a_key) {
    return RegCloseKey(a_key);
}

std::list<std::string> RegistryHelper::create_subkeys_list(HKEY a_key) {
    std::list<std::string> l_result_list;
    DWORD i, retCode;
    TCHAR    achKey[MAX_KEY_LENGTH];
    DWORD    cbName = MAX_KEY_LENGTH;
    DWORD    cSubKeys = 0;
    RegQueryInfoKey(a_key,
                    NULL,
                    NULL,
                    NULL,
                    &cSubKeys,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
    if (cSubKeys) {
        for (i = 0; i < cSubKeys; i++) {
            retCode = RegEnumKeyEx(a_key,
                                   i,
                                   achKey,
                                   &cbName,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
            if (retCode == ERROR_SUCCESS) {
                if (achKey[0] != '{') {
                    l_result_list.push_back(achKey);
                }
            }
        }
    }
    return l_result_list;
}

std::string RegistryHelper::get_string_value(HKEY a_key, std::string a_value) {
    TCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    DWORD nError;
    //a_value("InstallLocation");
    nError = RegQueryValueEx(a_key, a_value.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (nError == ERROR_SUCCESS) {
        return std::string(szBuffer);
    }
    else {
        return "";
    }
}
