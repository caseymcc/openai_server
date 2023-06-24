#ifndef _llama_llamaServer_h_
#define _llama_llamaServer_h_

#include <string>
#include <vector>
#include <unordered_map>

namespace llama
{

struct Model
{
    std::string name;
    std::string path;

    bool loaded;
};  

struct Server
{
    std::string host;
    uint16_t port;

    std::string version;
    
    std::string modelDirectory;
    std::vector<Model> models;
    std::unordered_map<std::string, size_t> modelMap;
};

bool getModels(const std::string &modelDirectory, Server &server);

}//namespace llama

#endif//_llama_llamaServer_h_