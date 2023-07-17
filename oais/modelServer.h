#ifndef _oais_modelServer_h_
#define _oais_modelServer_h_

#include "oais/interfaces/llamaModel.h"
#include "oais/serverOptions.h"
#include "oais/modelDefinition.h"

#include <httplib.h>

#include <string>
#include <vector>

namespace oais
{

constexpr size_t InvalidIndex=std::numeric_limits<size_t>::max();

struct ModelInterface
{
    std::string name;
    std::vector<std::string> models;

};

struct Model
{
    ModelDefinition definition;

    bool loaded;
};

struct OpenAIOptions
{
    int model;
    bool stream;
    float top_p;
    float temperature;
    int max_tokens;
    int n;

    float presencePenalty;
    float frequencyPenalty;
    std::vector<std::string> stopWords;
};

class ModelServer
{
public:
    ModelServer();

    bool init(ServerOptions &options);
    void run();
    void runCmd();

    void rootHandler(const httplib::Request &req, httplib::Response &res);
    void modelsHandler(const httplib::Request &req, httplib::Response &res);
    void modelHandler(const httplib::Request &req, httplib::Response &res);
    void completionHandler(const httplib::Request &req, httplib::Response &res);
    void chatCompletionHandler(const httplib::Request &req, httplib::Response &res);
    void embeddingHandler(const httplib::Request &req, httplib::Response &res);

private:
    bool getModels(const std::string &modelDirectory, std::vector<std::string> &models);
    size_t getModelIndex(std::string modelName);

    ServerOptions m_options;

    httplib::Server m_httpServer;

    std::string m_version;

    bool m_init;
    
    std::string m_modelDirectory;
    std::vector<Model> m_models;
    std::unordered_map<std::string, size_t> m_modelMap;

    std::string m_chatModel;
    std::string m_imageModel;

    bool m_embedding;
    int m_currentModel;

    size_t m_tokensRead;
    size_t m_tokensLeft;
    size_t m_tokensPredicted;
    
    LlamaModel m_llamaModel;
};

}//namespace oais

#endif//_oais_modelServer_h_