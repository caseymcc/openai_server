#ifndef _oais_llamaModel_h_
#define _oais_llamaModel_h_

#include <llama.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>

namespace oais
{

enum class StopType
{
    None,
    EOS,
    Word,
    Limit,
    StopWord
};

class LlamaModel
{
public:
    LlamaModel();
    ~LlamaModel();

    bool loadModel(const std::string &modelFile);
    bool loadPrompt(const std::string &prompt);

    void beginCompletion();
    llama_token nextToken();
    std::string doCompletion();
    std::vector<float> embedding(const std::string &content);
    void reset();

    bool hasNextToken() const { return m_hasNextToken; }
    const std::string &getGeneratedText() const { return m_generatedText; }

private:
    llama_context_params m_params;
    llama_context *m_context;
    size_t m_threadCount = std::thread::hardware_concurrency();

    std::string m_model;
    std::string m_loraAdapter;
    std::string m_loraBase;

    bool m_stream = false;
    bool m_hasNextToken = false;
    StopType m_stopType = StopType::None;
//    bool m_eof=false;
    std::string m_generatedText = "";
    std::string m_stoppingWord;

    int32_t m_seed; //random seed
    int32_t m_predictCount; //number of tokens to predict
    std::vector<float> m_logitBias; //logit bias

    size_t m_tokensPredicted = 0; //number of tokens predicted
    int32_t m_tokensLeft; //number of tokens left to predict
    size_t m_tokensToKeep; //number of tokens to keep from initial prompt

    size_t m_batchSize=512; //batch size for prompt processing, (must be >=32 to use BLAS)
    std::vector<llama_token> m_embeddedTokens;
    size_t m_evalPos;

    std::string m_prompt;
    std::vector<llama_token> m_newLineTokens;
    std::vector<llama_token> m_lastTokens;
    int32_t m_multibytePending;

    // sampling parameters
    float m_temperature=1.0f; // 1.0 = disabled
    int32_t m_topK=40; // <= 0 to use vocab size
    float m_topP=0.95f; // 1.0 = disabled
    float m_tfsZ=1.00f; // 1.0 = disabled
    float m_typicalP=1.00f; // 1.0 = disabled

    int32_t m_tokensToPenalize=64; // last n tokens to penalize (0 = disable penalty, -1 = context size)
    float m_repeatPenalty=1.10f; // 1.0 = disabled
    float m_frequencyPenalty=0.00f; // 0.0 = disabled
    float m_presencePenalty=0.00f; // 0.0 = disabled
    int m_mirostat=0; // 0 = disabled, 1 = mirostat, 2 = mirostat 2.0
    float m_mirostat_tau=5.00f; // target entropy
    float m_mirostat_eta=0.10f; // learning rate

    bool m_penalizeNewline=true;  // consider newlines as a repeatable token
};

}//namespace oais

#endif//_oais_llamaModel_h_