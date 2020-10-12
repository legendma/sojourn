#include "pch.h"
#include "Utils.h"

std::wstring Utils::StringToWide( std::string to_convert )
{
    std::wstring converted( to_convert.begin(), to_convert.end() );
    return( converted );
}
