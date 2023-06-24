#include "oais/serverOptions.h"
#include "oais/modelServer.h"

#include <cxxopts.hpp>

#include <filesystem>

void printUsage()
{

}

int main(int argc, char ** argv) 
{
    cxxopts::Options options(argv[0], " - openai api server - Runs llama.cpp (and others) as openai api server");

    options.add_options()
        ("config", "Location of server config (optional, default ./config.json)", cxxopts::value<std::string>()->default_value("./config.json"))
        ("cmd", "Run server command line", cxxopts::value<bool>())
        ("h,help", "Print usage")
    ;

    auto result = options.parse(argc, argv);

    if(result.count("help"))
    {
        printUsage();
        return 0;
    }

    oais::ServerOptions serverOptions;
    std::string configFileName=result["config"].as<std::string>();

    if(!readServerOptions(serverOptions, configFileName))
    {
        return 0;
    }

    oais::ModelServer modelServer;

    if(!modelServer.init(serverOptions))
    {
        return 0;
    }

    if(result["cmd"].as<bool>())
    {
        modelServer.runCmd();
        return 0;
    }

    modelServer.run();
    return 0;
}
