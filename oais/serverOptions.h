#ifndef _oais_serverOptions_h_
#define _oais_serverOptions_h_

#include <string>
#include <vector>

namespace oais
{

struct ServerOptions
{
    std::string host;
    uint16_t port;
    
    std::string modelsDirectory;
    std::string model;
    std::vector<std::string> models;
};

bool readServerOptions(ServerOptions &serverOptions, const std::string &fileName);

}//namespace oais

#endif//_oais_serverOptions_h_