#include "oais/modelServer.h"

#include "oais/modelDefinition.h"

#include "uuid.h"

#include "oais/json.h"

#include <cstdio>
#include <filesystem>
#include <iostream>

namespace oais
{

namespace fs=std::filesystem;

void setErrorResponse(httplib::Response &res, const char *message)
{
    json::StringBuffer stringStream;
    json::PrettyWriter<json::StringBuffer> jsonWriter(stringStream);

    jsonWriter.StartObject();
    jsonWriter.Key("status");
    jsonWriter.String("error");
    jsonWriter.Key("reason");
    jsonWriter.String(message);
    jsonWriter.EndObject();

    res.set_content(stringStream.GetString(), "application/json");
    res.status = 400;
}

void setErrorResponse(httplib::Response &res, const std::string &message)
{
    setErrorResponse(res, message.c_str());
}

std::string generateUUID()
{
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);
    uuids::uuid_random_generator gen{generator};

    return uuids::to_string(gen());
}


OpenAIOptions parseOptions(json::Value &jsonValue)
{
    OpenAIOptions options;

    options.model=json::getValue<int>(jsonValue, "model", -1);
    options.stream=json::getValue<bool>(jsonValue, "stream", false);
    options.top_p=json::getValue<float>(jsonValue, "top_p", 1.0f);
    options.temperature=json::getValue<float>(jsonValue, "temperature", 1.0f);
    options.max_tokens=json::getValue<int>(jsonValue, "max_tokens", 16);
    options.n=json::getValue<int>(jsonValue, "n", 1);

    if(jsonValue.HasMember("stop"))
    {
        json::Value &stopWords=jsonValue["stop"];

        if(stopWords.IsArray())
        {
            for(const json::Value &arrayValue:stopWords.GetArray())
            {
                options.stopWords.push_back(json::returnValue<std::string>(arrayValue));
            }
        }
        else if(stopWords.IsString())
            options.stopWords.push_back(stopWords.GetString());
        else
            fprintf(stderr, "Invalid stop words\n");
    }

    options.presencePenalty=json::getValue<float>(jsonValue, "presence_penalty", 0.0f);
    options.frequencyPenalty=json::getValue<float>(jsonValue, "frequency_penalty", 0.0f);

    return options;
}

ModelServer::ModelServer():
    m_init(false)
{

}

bool ModelServer::getModels(const std::string &modelDirectory, std::vector<std::string> &models)
{
    std::filesystem::path modelsPath(modelDirectory);

    if(!std::filesystem::exists(modelsPath))
    {
        fprintf(stderr, "Models path %s  does not exist\n", modelsPath.string().c_str());
        return false;
    }

    if(!std::filesystem::is_directory(modelsPath))
    {
        fprintf(stderr, "Models path %s is not a directory\n", modelsPath.string().c_str());
        return false;
    }
    
    std::filesystem::directory_iterator modelsDir(modelsPath);

    for(auto &modelFile:modelsDir)
    {
        if(!std::filesystem::is_regular_file(modelFile))
            continue;
        if(modelFile.path().extension() != ".json")
            continue;

        Model model;

        if(!readModelDefinition(model.definition, modelFile.path().string()))
        {
            std::cout<<"Failed to read model definition: "<<modelFile.path().string()<<std::endl;
            continue;
        }

        if(!models.empty())
        {
            auto modelIter=std::find(models.begin(), models.end(), model.definition.name);

            if(modelIter == models.end())
                continue;
        }

        model.loaded=false;

        m_models.emplace_back(model);
        m_modelMap.emplace(model.definition.name, m_models.size()-1);
    }

    modelDirectory;
    return (m_models.size()>0);
}

bool ModelServer::init(ServerOptions &serverOptions)
{
    m_options=serverOptions;

    if(!getModels(serverOptions.modelsDirectory, serverOptions.models))
    {
        std::cout<<"Failed to get models"<<std::endl;
        return false;
    }

    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);
    uuids::uuid_random_generator gen{generator};

    return true;
}

void ModelServer::run()
{
    printf("Running model server on %s:%d\n", m_options.host.c_str(), m_options.port);

    m_httpServer.Get("/", std::bind(&ModelServer::rootHandler, this, std::placeholders::_1, std::placeholders::_2));
    m_httpServer.Get("/v1/models", std::bind(&ModelServer::modelsHandler, this, std::placeholders::_1, std::placeholders::_2));
    m_httpServer.Get(R"(/v1/models/[^/]+)", std::bind(&ModelServer::modelsHandler, this, std::placeholders::_1, std::placeholders::_2));
        
    std::function<void(const httplib::Request &, httplib::Response &)> completionHandlerFunc=std::bind(&ModelServer::completionHandler, this, std::placeholders::_1, std::placeholders::_2);
    m_httpServer.Post("/v1/completion", completionHandlerFunc);
    std::function<void(const httplib::Request &, httplib::Response &)> chatCompletionHandlerFunc=std::bind(&ModelServer::chatCompletionHandler, this, std::placeholders::_1, std::placeholders::_2);
    m_httpServer.Post("/v1/chat/completion", chatCompletionHandlerFunc);
    std::function<void(const httplib::Request &, httplib::Response &)> embeddingHandlerFunc=std::bind(&ModelServer::embeddingHandler, this, std::placeholders::_1, std::placeholders::_2);
    m_httpServer.Post("/v1/embedding", embeddingHandlerFunc);

    fprintf(stderr, "%s: http server Listening at http://%s:%i\n", __func__, m_options.host.c_str(), m_options.port);

    m_httpServer.listen(m_options.host, m_options.port);
}

void ModelServer::runCmd()
{
    while(true)
    {
        std::string cmd;

        std::getline(std::cin, cmd);

        if(cmd == "exit")
            break;

        if(!m_llamaModel.loadPrompt(cmd))
        {
            std::cout<<"Context too long, please be more specific"<<std::endl;
            continue;
        }

        m_llamaModel.beginCompletion();

        while(m_llamaModel.hasNextToken())
        {
            m_llamaModel.doCompletion();
        }
        std::cout<<m_llamaModel.getGeneratedText()<<std::endl;
    }
}

void ModelServer::rootHandler(const httplib::Request &req, httplib::Response &res)
{
    res.set_content("<h1>openai server works</h1>", "text/html");
}

void ModelServer::modelsHandler(const httplib::Request &req, httplib::Response &res)
{
    json::StringBuffer stringStream;
    json::PrettyWriter<json::StringBuffer> jsonWriter(stringStream);
    std::vector<std::string> models;

    jsonWriter.StartObject();

    jsonWriter.Key("models");
    jsonWriter.StartArray();
    for(size_t i=0; i<m_models.size(); ++i)
    {
        jsonWriter.StartObject();
        jsonWriter.Key("id");
        jsonWriter.String(m_models[i].definition.name.c_str());
        jsonWriter.Key("object");
        jsonWriter.String("model");
        jsonWriter.Key("owned_by");
        jsonWriter.String("llama");
        jsonWriter.Key("permission");
        jsonWriter.StartObject();
        jsonWriter.EndObject();
        jsonWriter.EndObject();
    }
    jsonWriter.EndArray();

    jsonWriter.Key("object");
    jsonWriter.String("list");

    jsonWriter.EndObject();

    res.set_content(stringStream.GetString(), "application/json");
    res.status = 400;
}

void ModelServer::modelHandler(const httplib::Request &req, httplib::Response &res)
{
    json::StringBuffer stringStream;
    json::PrettyWriter<json::StringBuffer> jsonWriter(stringStream);
    std::string modelName = req.matches[1].str();

    jsonWriter.StartObject();
    jsonWriter.Key("id");
    jsonWriter.String(modelName.c_str());
    jsonWriter.Key("object");
    jsonWriter.String("model");
    jsonWriter.Key("owned_by");
    jsonWriter.String("llama");
    jsonWriter.Key("permission");
    jsonWriter.StartObject();
    jsonWriter.EndObject();
    jsonWriter.EndObject();
    
    res.set_content(stringStream.GetString(), "application/json");
    res.status = 400;
}

void ModelServer::completionHandler(const httplib::Request &req, httplib::Response &res)
{
    json::Document jsonDoc;
    
    jsonDoc.Parse(req.body.c_str());
    
    OpenAIOptions options=parseOptions(jsonDoc);

    if(m_currentModel < 0)
    {
        if((options.model < 0) || (options.model >= m_models.size()))
        {
            setErrorResponse(res, "Model does not exist");
            return;
        }

        m_llamaModel.loadModel(m_models[options.model].definition.path.c_str());
        m_currentModel=options.model;
    }

    if(options.model != m_currentModel)
    {
        setErrorResponse(res, "Another model is already loaded, can only handle 1 at a time");
        return;
    }
    
    if(!jsonDoc.HasMember("prompt"))
    {
        setErrorResponse(res, "Request does not have a prompt");
        return;
    }

    m_llamaModel.reset();

    if(!m_llamaModel.loadPrompt(jsonDoc["prompt"].GetString()))
    {
        setErrorResponse(res, "Context too long, please be more specific");
        return;
    }

    size_t tokensPredicted=0;
    std::vector<std::string> choices;

    json::StringBuffer stringStream;
    json::PrettyWriter<json::StringBuffer> jsonWriter(stringStream);

    jsonWriter.StartObject();

    jsonWriter.Key("choices");
    jsonWriter.StartArray();
    for(uint32_t i=0; i<options.n; ++i)
    {
        m_llamaModel.beginCompletion();

        // loop inference until finish completion
        while(m_llamaModel.hasNextToken())
        {
            m_llamaModel.doCompletion();
        }

        jsonWriter.StartObject();
        jsonWriter.Key("text");
        jsonWriter.String(m_llamaModel.getGeneratedText().c_str());
        jsonWriter.Key("index");
        jsonWriter.Uint(i);
        jsonWriter.Key("logprobs");
        jsonWriter.StartObject();
        jsonWriter.EndObject();
        jsonWriter.Key("finish_reason");
        jsonWriter.String("length");
        jsonWriter.EndObject();
        
        tokensPredicted++;
    }
    jsonWriter.EndArray();
    
    jsonWriter.Key("id");
    jsonWriter.String(generateUUID().c_str());
    jsonWriter.Key("object");
    jsonWriter.String("text_completion");
    jsonWriter.Key("created");
    jsonWriter.Uint64(std::chrono::system_clock::now().time_since_epoch().count());
    jsonWriter.Key("model");
    jsonWriter.Uint(options.model);

    jsonWriter.EndObject();

    return res.set_content(stringStream.GetString(), "application/json");
}

void ModelServer::chatCompletionHandler(const httplib::Request &req, httplib::Response &res)
{
//    json::Document jsonDoc;
//    
//    jsonDoc.Parse(req.body);
//    
//    OpenAIOptions options=parseOptions(jsonDoc);
//
//    m_llamaModel.interactive = true;
//    m_llamaModel.no_show_words=
//    {
//        "user:",
//        "\nuser",
//        "system:",
//        "\nsystem",
//        "\n\n\n"
//    };
//
//    req.body.messages;
}

void ModelServer::embeddingHandler(const httplib::Request &req, httplib::Response &res)
{
//    if(!m_llamaModel.m_params.embedding)
//    {
//        std::vector<float> empty;
//        std::string data=json::build(
//            {
//                {"embedding", json::array(empty)}
//            }
//        );
//
//        fprintf(stderr, "[llama-server] : You need enable embedding mode adding: --embedding option\n");
//        return res.set_content(data, "application/json");
//    }

    json::Document jsonDoc;
    
    jsonDoc.Parse(req.body.c_str());
    
    OpenAIOptions options=parseOptions(jsonDoc);
    std::vector<std::string> input;

    if(jsonDoc.HasMember("input"))
    {
        json::Value &jsonInput=jsonDoc["input"];

        if(jsonInput.IsArray())
        {
            for(const json::Value &arrayValue:jsonInput.GetArray())
            {
                input.push_back(json::returnValue<std::string>(arrayValue));
            }
        }
        else if(jsonInput.IsString())
        {
            input.push_back(jsonInput.GetString());
        }
        else
        {
            setErrorResponse(res, "Request does not have input");
            return;
        }
    }

    json::StringBuffer stringStream;
    json::PrettyWriter<json::StringBuffer> jsonWriter(stringStream);

    jsonWriter.StartObject();
    jsonWriter.Key("object");
    jsonWriter.String("list");
    jsonWriter.Key("data");
    jsonWriter.StartArray();

    for(size_t i=0; i<input.size(); ++i)
    {
        jsonWriter.StartObject();

        jsonWriter.Key("object");
        jsonWriter.String("embedding");
        jsonWriter.Key("ebmedding");
        jsonWriter.StartArray();

        std::vector<float> embedding=m_llamaModel.embedding(input[i]);

        for(size_t j=0; j<embedding.size(); ++j)
        {
            jsonWriter.Double(embedding[j]);
        }
        jsonWriter.EndArray();
        jsonWriter.Key("index");
        jsonWriter.Uint(i);

        jsonWriter.EndObject();        
    }

    jsonWriter.EndArray();

    jsonWriter.Key("model");
    jsonWriter.Uint(options.model);

    jsonWriter.EndObject();

    return res.set_content(stringStream.GetString(), "application/json");
}

}//namespace oais