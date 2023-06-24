#include "llamaServer.h"

#include <filesystem>
#include <iostream>

bool getModels(const std::string &modelDirectory, LLamaServer &server)
{
    std::filesystem::path modelsPath(modelDirectory);

    if(!std::filesystem::exists(modelsPath))
    {
        std::cerr << "Models path " << modelsPath << " does not exist" << std::endl;
        return false;
    }

    if(!std::filesystem::is_directory(modelsPath))
    {
        std::cerr << "Models path " << modelsPath << " is not a directory" << std::endl;
        return false;
    }
    
    std::filesystem::directory_iterator modelsDir(modelsPath);

    for(auto &modelFile : modelsDir)
    {
        LLamaModel model;

        model.name=modelFile.path().filename().string();
        model.path=modelFile.path().string();
        model.loaded=false;

        server.models.emplace_back(model);
        server.modelMap.emplace(model.name, server.models.size()-1);
    }

    return (server.models.size()>0);
}