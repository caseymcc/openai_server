#ifndef _oais_json_h_
#define _oais_json_h_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wsuggest-destructor-override"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#pragma GCC diagnostic pop

//#include "oais/json/bulder.h"

#include <cstdio>
#include <vector>
#include <functional>

namespace rapidjson
{

template<typename _Type> 
_Type returnValue(const Value &value)=delete;

template<> 
inline bool returnValue<bool>(const Value &value)
{
    if(!value.IsBool())
    {
        printf("is not a bool");
        return "";
    }
    return value.GetBool(); 
}

template<> 
inline std::string returnValue<std::string>(const Value &value)
{
    if(!value.IsString())
    {
        printf("is not a string");
        return "";
    }
    
    std::string retValue=value.GetString();

    return retValue; 
}

template<> 
inline size_t returnValue<size_t>(const Value &value)
{ 
    if(!value.IsNumber())
    {
        printf("is not a number");
        return 0;
    }
    return value.GetUint(); 
}

template<> 
inline int returnValue<int>(const Value &value)
{ 
    if(!value.IsNumber())
    {
        printf("is not a number");
        return 0;
    }
    return value.GetInt(); 
}

template<> 
inline float returnValue<float>(const Value &value)
{ 
    if(!value.IsNumber())
    {
        printf("is not a number");
        return 0;
    }
    return value.GetFloat(); 
}

template<> 
inline double returnValue<double>(const Value &value)
{ 
    if(!value.IsNumber())
    {
        printf("is not a number");
        return 0;
    }
    return value.GetDouble(); 
}

template<> 
inline const Value &returnValue<const Value &>(const Value &value)
{
    return value; 
}

template<typename _Type> 
_Type returnMemberValue(const Value &value, const char *name)
{
    const Value &retValue=value[name];

    return returnValue<_Type>(retValue);
}
template<typename _Type> 
_Type returnMemberValue(const Value &value, const std::string &name)
{
    return returnMemberValue<_Type>(value, name.c_str());
}

template<typename _Type> 
_Type defaultValue(const Value &value)=delete;

template<> 
inline bool defaultValue<bool>(const Value &value)
{
    return false;
}

template<> 
inline std::string defaultValue<std::string>(const Value &value)
{
    return "";
}

template<> 
inline size_t defaultValue<size_t>(const Value &value)
{
    return 0;
}

template<> 
inline float defaultValue<float>(const Value &value)
{
    return 0.0f;
}

template<> 
inline double defaultValue<double>(const Value &value)
{
    return 0.0;
}

template<> 
inline const Value &defaultValue<const Value &>(const Value &value)
{
    return value;
}

template<typename _Type> 
_Type getValue(const Value &value, const char *name)
{
    if(!value.HasMember(name))
    {
        _Type retValue=defaultValue<_Type>(value);

        printf("\"%s\" member missing", name);
        return retValue;
    }

    return returnMemberValue<_Type>(value, name);
}
template<typename _Type> 
_Type getValue(const Value &value, const std::string &name)
{
    return getValue<_Type>(value, name.c_str());
}

template<typename _Type> 
_Type getValue(const Value &value, const char *name, _Type defaultValue)
{
    if(!value.HasMember(name))
        return defaultValue;
    return returnMemberValue<_Type>(value, name);
}

template<typename _Type> 
_Type getValue(const Value &value, const std::string &name, _Type defaultValue)
{
    return getValue<_Type>(value, name.c_str(), defaultValue);
}

template<typename _Type>
std::vector<_Type> getVector(const Value &value, const char *name)
{
    std::vector<_Type> retValues;

    if(!value.HasMember(name))
    {
        printf("\"%s\" member missing", name);
        return retValues;
    }

    const Value &nameValue = value[name];

    if(!nameValue.IsArray())
    {
        printf("\"%s\" member is not an array", name);
        return retValues;
    }

    for(const Value &arrayValue:nameValue.GetArray())
    {
        retValues.push_back(returnValue<_Type>(arrayValue));
    }

    return retValues;
}
template<typename _Type>
std::vector<_Type> getVector(const Value &value, const std::string &name)
{
    return getVector<_Type>(value, name.c_str());
}

template<typename _Type>
bool fillVector(const Value &jsonValue, std::string name, std::vector<_Type> &values, std::function<bool(const Value&, _Type&)> func)
{
    if(!jsonValue.HasMember(name.c_str()))
    {
        fprintf(stderr, "Failed to load %s", name.c_str());
        return false;
    }

    const Value &jsonValues=jsonValue[name.c_str()];

    if(!jsonValues.IsArray())
        return false;

    size_t count=jsonValues.Size();
    values.resize(count);

    bool success=true;
    for(size_t i=0; i<count; ++i)
    {
        if(!func(jsonValues[i], values[i]))
        {
            success=false;
            break;
        }
    }

    return success;
}

template<typename _Type>
bool fillVector(const Value &jsonValue, std::string name, std::vector<_Type> &values, std::function<bool(const Value&, std::vector<_Type> &values, _Type&)> func)
{
    if(!jsonValue.HasMember(name.c_str()))
    {
        fprintf(stderr, "Failed to load %s", name.c_str());
        return false;
    }

    const Value &jsonValues=jsonValue[name.c_str()];

    if(!jsonValues.IsArray())
        return false;

    size_t count=jsonValues.Size();
    values.resize(count);

    bool success=true;
    for(size_t i=0; i<count; ++i)
    {
        if(!func(jsonValues[i], values[i]))
        {
            success=false;
            break;
        }
    }

    return success;
}

}//namespace rapidjson

namespace oais
{

namespace json=rapidjson;

}//namespace oais

#endif//_oais_json_h_