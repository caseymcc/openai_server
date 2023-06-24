#include "oais/serverOptions.h"

#include "oais/json.h"
#include "oais/utils.h"

#include <filesystem>
#include <iostream>

namespace oais
{

bool readServerOptions(ServerOptions &serverOptions, const std::string &fileName)
{
    std::filesystem::path filePath(fileName);

    if(!std::filesystem::exists(filePath))
    {
        std::cout<<"Error: server options file does not exist: "<<fileName<<std::endl;
        return false;
    }

    std::string serverOptionsBuffer=loadFileToString(fileName);

    json::Document document;
    document.Parse(serverOptionsBuffer.c_str());

    if(document.HasParseError())
    {
        std::cout<<"Error parsing server options file: "<<fileName<<std::endl;
        return false;
    }

    std::vector<std::string> requiredFields={"host", "port", "modelsDirectory", "model"};

    bool error=false;
    std::stringstream errorString;
    for(std::string &field : requiredFields)
    {
        if(!document.HasMember(field.c_str()))
        {
            errorString<<"  Missing required field: "<<field<<std::endl;
            error=true;   
        }
    }

    if(error)
    {
        std::cout<<"Error parsing server options file: "<<fileName<<std::endl;
        std::cout<<errorString.str()<<std::endl;
        return false;
    }

    serverOptions.host=json::returnValue<std::string>(document["host"]);
    serverOptions.port=(uint16_t)json::returnValue<int>(document["port"]);
    serverOptions.modelsDirectory=json::returnValue<std::string>(document["modelsDirectory"]);
    serverOptions.model=json::returnValue<std::string>(document["model"]);
    
    if(document.HasMember("models"))
    {
        json::Value &jsonModels=document["models"];

        if(!jsonModels.IsArray())
        {
            std::cout<<"Error parsing server options file: "<<fileName<<std::endl;
            std::cout<<"  models field is not an array"<<std::endl;
            return false;
        }

        for(json::Value::ConstValueIterator itr=jsonModels.Begin(); itr!=jsonModels.End(); ++itr)
        {
            serverOptions.models.push_back(json::returnValue<std::string>(*itr));
        }
    }

    return true;
}

}//namespace oais
