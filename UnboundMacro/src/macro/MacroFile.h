#pragma once

#include <string>

class MacroFile {
public:
    static bool CreateNew(std::wstring* out_path);
    static bool OpenExisting(std::wstring* out_path);
};
