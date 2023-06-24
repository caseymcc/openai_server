#ifndef _oais_utils_h_
#define _oais_utils_h_

#include <vector>
#include <string>
#include <filesystem>

namespace oais
{

namespace fs=std::filesystem;

template <typename _Type>
size_t find_first_different(std::vector<_Type> const &vec1, std::vector<_Type> const &vec2)
{
    size_t i=0;
    for(; i<vec1.size() && i<vec2.size(); ++i) 
    {}
    return i;
};

inline bool ends_with(std::string const &value, std::string const &ending)
{
    if(ending.size() > value.size()) 
        return false;

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline std::string loadFileToString(const std::string &fileName)
{
    fs::path filePath(fileName);
    std::string buffer;
    
    if(!fs::exists(filePath))
    {
        return buffer;
    }

    std::uintmax_t fileSize=fs::file_size(filePath);

    if(fileSize == 0)
    {
        return buffer;
    }

    buffer.resize(fileSize+1);
    FILE *file=fopen(fileName.c_str(), "r");
    
    if(file == NULL)
    {
        return buffer;
    }

    size_t readSize=fread(&buffer[0], 1, fileSize, file);

    if(readSize != fileSize)
    {
        buffer.clear();
    }
    buffer[fileSize]='\0';
    return buffer;
}

}//namespace oais

#endif//_oais_utils_h_