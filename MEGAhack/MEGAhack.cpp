#include <windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <Psapi.h>
#include <conio.h>
#include <fstream>
DWORD GetBaseAddress(const HANDLE hProcess) {
    if (hProcess == NULL)
        return NULL; // No access to the process

    HMODULE lphModule[1024]; // Array that receives the list of module handles
    DWORD lpcbNeeded(NULL); // Output of EnumProcessModules, giving the number of bytes requires to store all modules handles in the lphModule array

    if (!EnumProcessModules(hProcess, lphModule, sizeof(lphModule), &lpcbNeeded))
        return NULL; // Impossible to read modules

    TCHAR szModName[MAX_PATH];
    if (!GetModuleFileNameEx(hProcess, lphModule[0], szModName, sizeof(szModName) / sizeof(TCHAR)))
        return NULL; // Impossible to get module info

    return (DWORD)lphModule[0]; // Module 0 is apparently always the EXE itself, returning its address
}
//Читаем из реестра 
std::wstring ReadRegistryValue(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(hKeyRoot, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return L"";
    }

    DWORD dataType;
    wchar_t data[256];
    DWORD dataSize = sizeof(data);

    result = RegQueryValueEx(hKey, valueName.c_str(), nullptr, &dataType, reinterpret_cast<BYTE*>(data), &dataSize);
    if (result == ERROR_SUCCESS && dataType == REG_SZ) {
        RegCloseKey(hKey);
        return data;
    }
    else {
        RegCloseKey(hKey);
        return L"";
    }
}
// Получаем путь к игре
std::wstring GetGamePath() {
    std::wstring gamePath;
    gamePath = ReadRegistryValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\SEGA\\Dawn of War - Soulstorm", L"installlocation");
    if (gamePath.empty()) {
        gamePath = ReadRegistryValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\SEGA\\Dawn of War Soulstorm", L"installlocation");
    }
    if (gamePath.empty()) {
        gamePath = ReadRegistryValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\THQ\\Dawn of War Soulstorm", L"installlocation");
    }
    if (gamePath.empty()) {
        gamePath = ReadRegistryValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\THQ\\Dawn of War - Soulstorm", L"installlocation");
    }
    if (gamePath.empty()) {
        std::wstring steam = ReadRegistryValue(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", L"SteamPath");
        if (steam.empty()) {
            return gamePath;
        }
        gamePath = steam + L"/steamapps/common/Dawn of War Soulstorm";
    }
    return gamePath;
}
//Проверяем стимовская версия или довонлайн
int CheckGameVer(std::wstring gamePath) {
    std::string line;
    std::ifstream inputFile(gamePath + L"/warnings.log");
    if (!inputFile) {
        std::cout << "Не найден абсолютный путь, переместите приложение в папку с игрой" << std::endl;
        std::ifstream inputFile2("warnings.log");
        if (!inputFile2) {
            std::cout << "Файл не найден" << std::endl;
            return 0;
        }
        else {
            while (std::getline(inputFile2, line)) {
                if (line.find("GAME -- Warhammer, 1.3") != std::string::npos) {
                    std::cout << "Версия игры 1.3" << std::endl;
                    return 1;
                }
                if (line.find("GAME -- Warhammer, 1.2") != std::string::npos) {
                    std::cout << "Версия игры 1.2" << std::endl;
                    return 2;
                }
            }
            inputFile2.close();
            std::cout << "Версия игры не найдена" << std::endl;
            return 0;
        }
    }
    while (std::getline(inputFile, line)) {
        if (line.find("GAME -- Warhammer, 1.3") != std::string::npos) {
            std::cout << "Версия игры 1.3" << std::endl;
            return 1;
        }
        if (line.find("GAME -- Warhammer, 1.2") != std::string::npos) {
            std::cout << "Версия игры 1.2" << std::endl;
            return 2;
        }
    }
    inputFile.close();
    std::cout << "Версия игры не найдена" << std::endl;
    return 0;
}

template <typename T>
T ReadMemory(HANDLE hProcess, DWORD address) {
    T value;
    ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
    return value;
}

template <typename T>
bool WriteMemory(HANDLE hProcess, DWORD address, const T& value) {
    return WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr);
}
//Отключаем туман войны, туман и включаем показ всех хпбаров
int maphack(DWORD offset) {
    HWND hWnd = FindWindow(NULL, L"Dawn of War: Soulstorm");
    if (hWnd == nullptr) {
        std::cout << "Hwbd error" << std::endl;
        return 1;
    }
    DWORD PID = 0;
    GetWindowThreadProcessId(hWnd, &PID);
    if (PID == 0) {
        std::cout << "Pid error" << std::endl;
        return 1;
    }
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, 0, PID);
    if (hProcess == nullptr) {
        std::cout << "Open process error" << std::endl;
        return 1;
    }
    DWORD fog = 0x008282F0;
    DWORD hp = 0x00956596;
    BYTE fog_toggle[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
    BYTE hp_toggle[4] = { 0x90, 0x90, 0x90, 0x90 };
    WriteMemory(hProcess, fog, fog_toggle);
    WriteMemory(hProcess, hp, hp_toggle);
    DWORD baseAddress = GetBaseAddress(hProcess);
    DWORD pointerAddress1 = baseAddress + offset;
    DWORD pointerValue1 = ReadMemory<DWORD>(hProcess, pointerAddress1);
    DWORD pointerAddress2 = pointerValue1 + 0x24;
    DWORD pointerValue2 = ReadMemory<DWORD>(hProcess, pointerAddress2);
    DWORD finalAddress = pointerValue2 + 0xC58;
    BYTE fow = 0x01;
    if (WriteMemory(hProcess, finalAddress, fow)) {
        std::cout << "Good" << std::endl;
    }
    else {
        std::cout << "Error" << std::endl;
    }

    CloseHandle(hProcess);
    return 0;
}
int main() {
    setlocale(LC_ALL, "Russian");
    std::wstring gamePath;
    gamePath = GetGamePath();
    if (gamePath.empty()) {
        std::cout << "Путь к игре не найден" << std::endl;     
    }
    else {
        std::wcout << L"Путь: " + gamePath << std::endl;
    }
    std::cout << "Нажмите F1 что бы включить мапхак" << std::endl;
    while (true) {
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            int v = CheckGameVer(gamePath);
            if (v == 1) {
                maphack(0x0083F24C);
            }
            if (v == 2) {
                maphack(0x0078A830);
            }
        }
        Sleep(100);
    }
    return 0;
}