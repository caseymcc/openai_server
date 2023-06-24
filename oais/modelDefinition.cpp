#include "oais/modelDefinition.h"

#include "oais/json.h"
#include "oais/utils.h"

#include <iostream>
#include <sstream>
#include <filesystem>

namespace oais
{

namespace fs=std::filesystem;

bool readModelDefinition(ModelDefinition &modelDef, const std::string &fileName)
{
    json::Document document;

    std::string modelBuffer=loadFileToString(fileName);
    document.Parse(modelBuffer.c_str());

    if(document.HasParseError())
    {
        std::cout<<"Error parsing model definition file: "<<fileName<<std::endl;
        return false;
    }

    std::vector<std::string> requiredFields={"name", "path"};
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
        std::cout<<"Error parsing model definition file: "<<fileName<<std::endl;
        std::cout<<errorString.str()<<std::endl;
        return false;
    }

    modelDef.name=json::returnValue<std::string>(document["name"]);
    std::string modelFile=json::returnValue<std::string>(document["path"]);
    fs::path filePath(fileName);
    
    modelDef.path=(filePath.parent_path()/modelFile).string();

    return true;
}

}//namespace oais