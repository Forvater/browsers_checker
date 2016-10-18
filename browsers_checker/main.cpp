#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <list>
#include "tinyxml2.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

enum Roots { LOCAL_MACHINE, CURRENT_USER };
enum RegistryModes {KEY32, KEY64};

RegistryModes gIeRegKeyMode = RegistryModes::KEY32;
Roots gIeRoot = Roots::LOCAL_MACHINE;
std::string gIeRegPath = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";
std::string gIeAppName = "internet_explorer";
std::string gIeAppExe = "iexplore.exe";

struct RegSubPath {
    RegistryModes mode;
    std::string m_registry_sub_path;
};

struct RegPath {
    Roots root;
    std::string m_registry_path;
    std::list<RegSubPath> m_registry_subpaths;
};

struct App {
    std::string m_name;
    std::string m_exe_name;
};

struct AppOutput {
    AppOutput() :m_file_version_ms(0), m_file_version_ls(0) {}
    std::string m_name;
    std::string m_exe_name;
    std::string m_install_path;
    unsigned long m_file_version_ms;
    unsigned long m_file_version_ls;
};

std::list<std::string> gSubkeys;
std::list<RegPath> gRegPaths;
std::list<App> gApps;
std::list<AppOutput> gAppsOutput;

void parse_config(std::string aCommand_line_str)
{
    tinyxml2::XMLDocument xmlDoc;
    xmlDoc.LoadFile(aCommand_line_str.c_str());
    tinyxml2::XMLNode* pRoot = xmlDoc.FirstChild();

    tinyxml2::XMLElement* pRegistryPaths = pRoot->FirstChildElement("registry_paths");
    tinyxml2::XMLElement* pApps = pRoot->FirstChildElement("apps");

    tinyxml2::XMLElement* pCurrent = pRegistryPaths->FirstChildElement();

    while (pCurrent != nullptr) {
        std::string l_full_reg_path = pCurrent->GetText();
        std::string l_root = l_full_reg_path.substr(0, 4);
        std::string l_reg_path = l_full_reg_path.substr(6);
        RegPath l_rpath;
        if (l_full_reg_path.find("HKLM") != std::string::npos) {
            l_rpath.root = Roots::LOCAL_MACHINE;
            l_rpath.m_registry_path = l_reg_path;
        }
        else if(l_full_reg_path.find("HKCU") != std::string::npos)
        {
            l_rpath.root = Roots::CURRENT_USER;
            l_rpath.m_registry_path = l_reg_path;
        }
        gRegPaths.push_back(l_rpath);
        pCurrent = pCurrent->NextSiblingElement();
    }

    pCurrent = pApps->FirstChildElement();
    while (pCurrent != nullptr) {
        std::string l_app_name = pCurrent->FirstChildElement("name")->GetText();
        std::string l_app_exe = pCurrent->FirstChildElement("exe")->GetText();
        App l_app;
        l_app.m_name = l_app_name;
        l_app.m_exe_name = l_app_exe;
        gApps.push_back(l_app);
        pCurrent = pCurrent->NextSiblingElement();
    }
}

std::list<std::string> create_subkeys_list(HKEY hKey) {
    std::list<std::string> l_result_list;

    DWORD i, retCode;
    TCHAR    achKey[MAX_KEY_LENGTH];
    DWORD    cbName;
    TCHAR    achClass[MAX_PATH] = TEXT("");
    DWORD    cchClassName = MAX_PATH;
    DWORD    cSubKeys = 0;
    DWORD    cbMaxSubKey;
    DWORD    cchMaxClass;
    DWORD    cValues;
    DWORD    cchMaxValue;
    DWORD    cbMaxValueData;
    DWORD    cbSecurityDescriptor;
    FILETIME ftLastWriteTime;

    retCode = RegQueryInfoKey(
        hKey,
        achClass,
        &cchClassName,
        NULL,
        &cSubKeys,
        &cbMaxSubKey,
        &cchMaxClass,
        &cValues,
        &cchMaxValue,
        &cbMaxValueData,
        &cbSecurityDescriptor,
        &ftLastWriteTime);


    if (cSubKeys)
    {
        for (i = 0; i<cSubKeys; i++)
        {
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKey, i,
                achKey,
                &cbName,
                NULL,
                NULL,
                NULL,
                &ftLastWriteTime);
            if (retCode == ERROR_SUCCESS)
            {
                if (achKey[0] != '{') {
                    l_result_list.push_back(achKey);
                }
            }
        }
    }

    return l_result_list;
}



std::list<RegSubPath> open_reg_path_and_return_subpaths_list(int a_root, std::string a_reg_path) {
    HKEY hTestKey;
    std::list<RegSubPath> l_result_list;
    if (a_root == Roots::LOCAL_MACHINE) {

        long reg_open_result_lm = RegOpenKeyEx(HKEY_LOCAL_MACHINE, a_reg_path.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &hTestKey);
        if (reg_open_result_lm == ERROR_SUCCESS) {
            std::list<std::string> l_temp_list = create_subkeys_list(hTestKey);
            for (auto it = l_temp_list.begin(); it != l_temp_list.end(); ++it) {
                RegSubPath l_tmp;
                l_tmp.mode = RegistryModes::KEY32;
                l_tmp.m_registry_sub_path = *it;
                l_result_list.push_back(l_tmp);
            }
            RegCloseKey(hTestKey);
        }

        reg_open_result_lm = RegOpenKeyEx(HKEY_LOCAL_MACHINE, a_reg_path.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hTestKey);
        if (reg_open_result_lm == ERROR_SUCCESS) {
            std::list<std::string> l_result_list_append = create_subkeys_list(hTestKey);
            if (l_result_list_append.size() != 0) {
                for (auto it = l_result_list_append.begin(); it != l_result_list_append.end(); ++it) {
                    RegSubPath l_tmp;
                    l_tmp.mode = RegistryModes::KEY64;
                    l_tmp.m_registry_sub_path = *it;
                    l_result_list.push_back(l_tmp);
                }
            }
            RegCloseKey(hTestKey);
        }
    }
    else if (a_root == Roots::CURRENT_USER) {

        long reg_open_result_cu = RegOpenKeyEx(HKEY_CURRENT_USER, a_reg_path.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &hTestKey);
        if (reg_open_result_cu == ERROR_SUCCESS) {
            std::list<std::string> l_result_list_temp = create_subkeys_list(hTestKey);
            if (l_result_list_temp.size() != 0) {
                for (auto it = l_result_list_temp.begin(); it != l_result_list_temp.end(); ++it) {
                    RegSubPath l_tmp;
                    l_tmp.mode = RegistryModes::KEY32;
                    l_tmp.m_registry_sub_path = *it;
                    l_result_list.push_back(l_tmp);
                }
            }
            RegCloseKey(hTestKey);
        }

        reg_open_result_cu = RegOpenKeyEx(HKEY_CURRENT_USER, a_reg_path.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hTestKey);
        if (reg_open_result_cu == ERROR_SUCCESS) {
            std::list<std::string> l_result_list_append_cu = create_subkeys_list(hTestKey);
            if (l_result_list_append_cu.size() != 0) {
                for (auto it = l_result_list_append_cu.begin(); it != l_result_list_append_cu.end(); ++it) {
                    RegSubPath l_tmp;
                    l_tmp.mode = RegistryModes::KEY64;
                    l_tmp.m_registry_sub_path = *it;
                    l_result_list.push_back(l_tmp);
                }
            }
            RegCloseKey(hTestKey);
        }
    }
    return l_result_list;
}

void open_reg_key_and_create_sub_paths() {
    for (auto it = gRegPaths.begin(); it != gRegPaths.end(); ++it) {
        it->m_registry_subpaths = open_reg_path_and_return_subpaths_list(it->root, it->m_registry_path);
    }
}


std::string lower_reg_path(std::string a_str) {
    std::string llower = "";
    for (std::string::size_type i = 0; i < a_str.length(); ++i) {
        llower.push_back(std::tolower(a_str[i]));
    }
    return llower;
}

std::string get_install_path(Roots a_root, std::string a_path, std::string a_subpath, RegistryModes a_key_mode) {

    HKEY hTestKey;
    std::string l_full_path = a_path + "\\" + a_subpath;
    
    if (a_root == Roots::LOCAL_MACHINE) {
        if (a_key_mode == RegistryModes::KEY32) {
            long reg_open_result_lm = RegOpenKeyEx(HKEY_LOCAL_MACHINE, l_full_path.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &hTestKey);
            if (reg_open_result_lm == ERROR_SUCCESS) {
                TCHAR szBuffer[512];
                DWORD dwBufferSize = sizeof(szBuffer);
                DWORD nError;
                std::string val("InstallLocation");
                nError = RegQueryValueEx(hTestKey, val.c_str() , 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
                std::string l_res(szBuffer);
                RegCloseKey(hTestKey);
                if (nError == ERROR_SUCCESS) {
                    return l_res;
                }
            }
        }
        else if (a_key_mode == RegistryModes::KEY64) {
            long reg_open_result_lm = RegOpenKeyEx(HKEY_LOCAL_MACHINE, l_full_path.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hTestKey);
            if (reg_open_result_lm == ERROR_SUCCESS) {
                TCHAR szBuffer[512];
                DWORD dwBufferSize = sizeof(szBuffer);
                DWORD nError;
                std::string val("InstallLocation");
                nError = RegQueryValueEx(hTestKey, val.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
                std::string l_res(szBuffer);
                RegCloseKey(hTestKey);
                if (nError == ERROR_SUCCESS) {
                    return l_res;
                }
            }
        }
        else {
            return "";
        }
    }
    else if (a_root == Roots::CURRENT_USER) {
        if (a_key_mode == RegistryModes::KEY32) {
            long reg_open_result_lm = RegOpenKeyEx(HKEY_CURRENT_USER, l_full_path.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &hTestKey);
            if (reg_open_result_lm == ERROR_SUCCESS) {
                TCHAR szBuffer[512];
                DWORD dwBufferSize = sizeof(szBuffer);
                DWORD nError;
                std::string val("InstallLocation");
                nError = RegQueryValueEx(hTestKey, val.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
                std::string l_res(szBuffer);
                RegCloseKey(hTestKey);
                if (nError == ERROR_SUCCESS) {
                    return l_res;
                }
            }
        }
        else if (a_key_mode == RegistryModes::KEY64) {
            long reg_open_result_lm = RegOpenKeyEx(HKEY_CURRENT_USER, l_full_path.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hTestKey);
            if (reg_open_result_lm == ERROR_SUCCESS) {
                TCHAR szBuffer[512];
                DWORD dwBufferSize = sizeof(szBuffer);
                DWORD nError;
                std::string val("InstallLocation");
                nError = RegQueryValueEx(hTestKey, val.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
                std::string l_res(szBuffer);
                RegCloseKey(hTestKey);
                if (nError == ERROR_SUCCESS) {
                    return l_res;
                }
            }
        }
        else {
            return "";
        }
    }
    else {
        return "";
    }
}


std::string match_app_name(std::string a_name) {
    for (auto it = gRegPaths.begin(); it != gRegPaths.end(); ++it) {
        auto internal_it_begin = it->m_registry_subpaths.begin();
        auto internal_it_end = it->m_registry_subpaths.end();
        for (auto it1 = internal_it_begin; it1 != internal_it_end; ++it1) {
            std::string l_lower_path = lower_reg_path(it1->m_registry_sub_path);
            size_t found = l_lower_path.find(a_name);
            if (found != std::string::npos) {
                std::string l_install_path = get_install_path(it->root, it->m_registry_path, it1->m_registry_sub_path, it1->mode);
                return l_install_path;
            }
        }
    }
    return "";
}

void find_apps_install_path() {
    for (auto it = gApps.begin(); it != gApps.end(); ++it) {
        std::string l_install_path = match_app_name(it->m_name);
        if (l_install_path != "") {
            AppOutput l_app_temp;
            l_app_temp.m_name = it->m_name;
            l_app_temp.m_exe_name = it->m_exe_name;
            l_app_temp.m_install_path = l_install_path;
            gAppsOutput.push_back(l_app_temp);
        }
    }
}

void get_version_info(AppOutput& a_app_output) {
    DWORD  verHandle = NULL;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    std::string l_path = a_app_output.m_install_path + "\\" + a_app_output.m_exe_name;
    DWORD  verSize = GetFileVersionInfoSize(l_path.c_str(), &verHandle);
    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];
        if (GetFileVersionInfo(l_path.c_str(), verHandle, verSize, verData)) {
            if (VerQueryValue(verData, "\\", (VOID FAR* FAR*)&lpBuffer, &size)) {
                if (size) {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                    int a = 0;
                    if (verInfo->dwSignature == 0xfeef04bd){
                        a_app_output.m_file_version_ms = verInfo->dwFileVersionMS;
                        a_app_output.m_file_version_ls = verInfo->dwFileVersionLS;
                    }
                }
            }
        }
        delete[] verData;
    }
}

void version_info() {
    for (auto it = gAppsOutput.begin(); it != gAppsOutput.end(); ++it) {
        get_version_info(*it);
    }
}

void save_app_output() {
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLNode * pRoot = xmlDoc.NewElement("output");
    xmlDoc.InsertFirstChild(pRoot);
    tinyxml2::XMLElement * pAppsElement = xmlDoc.NewElement("apps");

    pRoot->InsertEndChild(pAppsElement);

    for (auto it = gAppsOutput.begin(); it != gAppsOutput.end(); ++it) {
        tinyxml2::XMLElement * pAppElement = xmlDoc.NewElement("app");

        tinyxml2::XMLElement * pNameElement = xmlDoc.NewElement("name");
        pNameElement->SetText(it->m_name.c_str());
        pAppElement->InsertEndChild(pNameElement);

        tinyxml2::XMLElement * pExeElement = xmlDoc.NewElement("exe");
        pExeElement->SetText(it->m_exe_name.c_str());
        pAppElement->InsertEndChild(pExeElement);

        tinyxml2::XMLElement * pInstallPathElement = xmlDoc.NewElement("install_path");
        pInstallPathElement->SetText(it->m_install_path.c_str());
        pAppElement->InsertEndChild(pInstallPathElement);

        tinyxml2::XMLElement * pFileVersionMsElement = xmlDoc.NewElement("file_version_ms");
        pFileVersionMsElement->SetText((unsigned int)it->m_file_version_ms);
        pAppElement->InsertEndChild(pFileVersionMsElement);

        tinyxml2::XMLElement * pFileVersionLsElement = xmlDoc.NewElement("file_version_ls");
        pFileVersionLsElement->SetText((unsigned int)it->m_file_version_ls);
        pAppElement->InsertEndChild(pFileVersionLsElement);

        pAppsElement->InsertEndChild(pAppElement);
    }

    tinyxml2::XMLError eResult = xmlDoc.SaveFile("output.xml");
}

void parse_output(std::string aCommand_line_str) {
    tinyxml2::XMLDocument xmlDoc;
    xmlDoc.LoadFile(aCommand_line_str.c_str());
    tinyxml2::XMLNode* pRoot = xmlDoc.FirstChild();

    tinyxml2::XMLElement* pApps = pRoot->FirstChildElement("apps");

    tinyxml2::XMLElement* pCurrent = pApps->FirstChildElement();

    while (pCurrent != nullptr) {
        std::string l_app_name = pCurrent->FirstChildElement("name")->GetText();
        std::string l_app_exe = pCurrent->FirstChildElement("exe")->GetText();
        std::string l_app_install_path = pCurrent->FirstChildElement("install_path")->GetText();
        std::string l_app_file_version_ms = pCurrent->FirstChildElement("file_version_ms")->GetText();
        std::string l_app_file_version_ls = pCurrent->FirstChildElement("file_version_ls")->GetText();
        unsigned long l_app_file_version_ms_ul = std::stoul(l_app_file_version_ms, nullptr, 0);
        unsigned long l_app_file_version_ls_ul = std::stoul(l_app_file_version_ls, nullptr, 0);
        AppOutput l_app_output;
        l_app_output.m_name = l_app_name;
        l_app_output.m_exe_name = l_app_exe;
        l_app_output.m_install_path = l_app_install_path;
        l_app_output.m_file_version_ms = l_app_file_version_ms_ul;
        l_app_output.m_file_version_ls = l_app_file_version_ls_ul;
        gAppsOutput.push_back(l_app_output);
        pCurrent = pCurrent->NextSiblingElement();
    }
}

void print_output() {
    for (auto app_output : gAppsOutput) {
        std::cout << app_output.m_name << " Version: "
                  << ((app_output.m_file_version_ms >> 16) & 0xffff) << "."
                  << ((app_output.m_file_version_ms >> 0) & 0xffff) << "."
                  << ((app_output.m_file_version_ls >> 16) & 0xffff) << "."
                  << ((app_output.m_file_version_ls >> 0) & 0xffff) << std::endl;
    }
}

std::string get_ie_install_path() {
    HKEY hTestKey;
    long reg_open_result_lm = RegOpenKeyEx(HKEY_LOCAL_MACHINE, gIeRegPath.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &hTestKey);
    if (reg_open_result_lm == ERROR_SUCCESS) {
        TCHAR szBuffer[512];
        DWORD dwBufferSize = sizeof(szBuffer);
        DWORD nError;
        std::string val("Path");
        nError = RegQueryValueEx(hTestKey, val.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
        std::string l_res(szBuffer);
        RegCloseKey(hTestKey);
        if (nError == ERROR_SUCCESS) {
            return l_res;
        }
    }
    return "";
}

void get_ie_info() {
    std::string l_path = get_ie_install_path();
    std::string l_path_no_semicolon = l_path.substr(0, l_path.length() - 1);
    std::string l_ie_exe_path = l_path_no_semicolon + "\\" + gIeAppExe;
    AppOutput l_ie_app;
    l_ie_app.m_name = gIeAppName;
    l_ie_app.m_exe_name = gIeAppExe;
    l_ie_app.m_install_path = l_path_no_semicolon;
    get_version_info(l_ie_app);
    gAppsOutput.push_back(l_ie_app);
}


int main(int argc, char* argv[])
{

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " -r"<<" [full/path/to/config]" << std::endl;
        std::cerr << "Usage: " << argv[0] << " -l" << " [full/path/to/output]" << std::endl;

        return 1;
    }

    if (std::string(argv[1]) == "-r") {
        parse_config(argv[2]);
        open_reg_key_and_create_sub_paths();
        find_apps_install_path();
        version_info();
        get_ie_info();
        save_app_output();
    }
    else if (std::string(argv[1]) == "-l") {
        parse_output(argv[2]);
        print_output();
    }

    return 0;
}