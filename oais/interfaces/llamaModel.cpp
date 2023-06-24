
#include "oais/interfaces/llamaModel.h"
#include "oais/utils.h"

#include <cassert>
#include <algorithm>

namespace oais
{

LlamaModel::LlamaModel()
{
    if(m_threadCount == 0)
    {
        m_threadCount = 1;
    }
}

LlamaModel::~LlamaModel()
{

}

std::vector<llama_token> llamaTokenize(struct llama_context * ctx, const std::string & text, bool add_bos)
{
    // initialize to prompt numer of chars, since n_tokens <= n_prompt_chars
    std::vector<llama_token> res(text.size() + (int) add_bos);
    const int n = llama_tokenize(ctx, text.c_str(), res.data(), res.size(), add_bos);
    assert(n >= 0);
    res.resize(n);

    return res;
}

bool LlamaModel::loadModel(const std::string &modelName)
{
    m_params=llama_context_default_params();
    m_context=llama_init_from_file(modelName.c_str(), m_params);
    
    if(m_context == NULL)
    {
        fprintf(stderr, "%s: error: unable to load model\n", __func__);
        return false;
    }

    if(!m_loraAdapter.empty())
    {
        int error=llama_apply_lora_from_file(m_context,
            m_loraAdapter.c_str(), m_loraBase.empty() ? NULL : m_loraBase.c_str(),
            m_threadCount);
            
        if(error != 0)
        {
            fprintf(stderr, "%s: error: failed to apply lora adapter\n", __func__);
            return false;
        }
    }
    
    // determine newline token
    m_newLineTokens=llamaTokenize(m_context, "\n", false);
    m_lastTokens.resize(m_params.n_ctx);
    std::fill(m_lastTokens.begin(), m_lastTokens.end(), 0);

    return true;
}

bool LlamaModel::loadPrompt(const std::string &prompt)
{
    m_prompt.push_back(' ');
    m_prompt.insert(m_prompt.end(), prompt.begin(), prompt.end());
    
    std::vector<llama_token> promptTokens=llamaTokenize(m_context, m_prompt, true);

    if(m_tokensToKeep < 0)
    {
        m_tokensToKeep=promptTokens.size();
    }
    m_tokensToKeep=std::min((size_t)m_params.n_ctx-4, m_tokensToKeep);

    if(promptTokens.size() > (size_t)m_params.n_ctx)
    {
        const int left=(m_params.n_ctx-m_tokensToKeep)/2;
        const int erasedTokens=(promptTokens.size()-m_tokensToKeep-left-1);
        std::vector<llama_token> tokens(promptTokens.begin(), promptTokens.begin()+m_tokensToKeep);

        tokens.insert(tokens.end(), promptTokens.begin()+m_tokensToKeep+erasedTokens, promptTokens.end());   
        promptTokens=tokens;

        printf("input truncated, tokens: %d keep: %zu, left: %d\n", m_params.n_ctx, m_tokensToKeep, left);
    }
    else
    {
        const size_t promptSize=promptTokens.size();

        std::fill(m_lastTokens.begin(), m_lastTokens.end()-promptSize, 0);
        std::copy(promptTokens.begin(), promptTokens.end(), m_lastTokens.end()-promptSize);
    }

    m_evalPos=find_first_different(m_embeddedTokens, promptTokens);
    
    m_embeddedTokens=promptTokens;
    if(m_evalPos == promptTokens.size())
    {
        --m_evalPos;
    }
    
    printf("prompt ingested, eval pos: %zu\n", m_evalPos);

    m_hasNextToken=true;
    return true;
}

void LlamaModel::beginCompletion()
{
    m_tokensLeft = m_predictCount;

    llama_set_rng_seed(m_context, m_seed);
}

llama_token LlamaModel::nextToken()
{
    llama_token result = -1;

    if(m_embeddedTokens.size() >= (size_t)m_params.n_ctx)
    {
        m_tokensLeft = (m_params.n_ctx - m_tokensToKeep)/2;

        std::vector<llama_token> tokens(m_embeddedTokens.begin(), m_embeddedTokens.begin() + m_tokensToKeep);
        tokens.insert(tokens.end(), m_embeddedTokens.begin() - m_tokensLeft, m_embeddedTokens.end());
        m_embeddedTokens=tokens;
        m_evalPos=m_tokensToKeep;
    }

    while(m_evalPos < m_embeddedTokens.size())
    {
        size_t tokensToEval=m_embeddedTokens.size() - m_evalPos;

        if(tokensToEval > m_batchSize)
        {
            tokensToEval=m_batchSize;
        }

        if(llama_eval(m_context, m_embeddedTokens.data()+m_evalPos, tokensToEval, m_evalPos, m_threadCount))
        {
            fprintf(stderr, "%s : failed to eval\n", __func__);
            m_hasNextToken=false;
            return -1;
        }
        m_evalPos+=tokensToEval;
    }
    
    float *logits=llama_get_logits(m_context);
    int vocabularySize=llama_n_vocab(m_context);

    for(size_t i=0; i<m_logitBias.size(); ++i)
    {
        logits[i]+=m_logitBias[i];
    }

    std::vector<llama_token_data> candidates;
    candidates.reserve(vocabularySize);

    for(llama_token tokenId=0; tokenId<vocabularySize; ++tokenId)
    {
        candidates.emplace_back(llama_token_data{tokenId, logits[tokenId], 0.0f});
    }

    int32_t tokensToPenalize=m_tokensToPenalize<0?m_params.n_ctx:m_tokensToPenalize;
    llama_token_data_array candidatesPenalties={candidates.data(), candidates.size(), false};
    float newLineToken=logits[llama_token_nl()];
    
    tokensToPenalize=std::min(std::min((int32_t)m_lastTokens.size(), tokensToPenalize), m_params.n_ctx);

    llama_sample_repetition_penalty(m_context, &candidatesPenalties,
        m_lastTokens.data() + m_lastTokens.size() - tokensToPenalize,
        tokensToPenalize, m_repeatPenalty);
    llama_sample_frequency_and_presence_penalties(m_context, &candidatesPenalties,
        m_lastTokens.data()+m_lastTokens.size()-tokensToPenalize,
        tokensToPenalize, m_frequencyPenalty, m_presencePenalty);

    //reset newline if not penalized
    if(!m_penalizeNewline)
    {
        logits[llama_token_nl()]=newLineToken;
    }

    llama_token tokenId = 0;

    if(m_temperature <= 0)
    {
        //greedy sampling
        tokenId=llama_sample_token_greedy(m_context, &candidatesPenalties);
    }
    else
    {
        if(m_mirostat == 1)
        {
            float mirostat_mu=2.0f*m_mirostat_tau;
            const int mirostat_m=100;

            llama_sample_temperature(m_context, &candidatesPenalties, m_temperature);
            tokenId=llama_sample_token_mirostat(m_context, &candidatesPenalties, m_mirostat_tau, m_mirostat_eta, mirostat_m, &mirostat_mu);
        }
        else if(m_mirostat == 2)
        {
            static float mirostat_mu=2.0f*m_mirostat_tau;
            llama_sample_temperature(m_context, &candidatesPenalties, m_temperature);
            tokenId=llama_sample_token_mirostat_v2(m_context, &candidatesPenalties, m_mirostat_tau, m_mirostat_eta, &mirostat_mu);
        }
        else
        {
            // Temperature sampling
            llama_sample_tail_free(m_context, &candidatesPenalties, m_tfsZ, 1);
            llama_sample_typical(m_context, &candidatesPenalties, m_typicalP, 1);
            llama_sample_top_p(m_context, &candidatesPenalties, m_topP, 1);
            llama_sample_top_k(m_context, &candidatesPenalties, m_topK, 1);
            llama_sample_temperature(m_context, &candidatesPenalties, m_temperature);
            tokenId=llama_sample_token(m_context, &candidatesPenalties);
        }
    }

    m_lastTokens.erase(m_lastTokens.begin());
    m_lastTokens.push_back(tokenId);
    m_tokensPredicted++;

    m_embeddedTokens.push_back(tokenId);
    m_tokensLeft--;

    if(!m_embeddedTokens.empty() && m_embeddedTokens.back() == llama_token_eos())
    {
        m_hasNextToken=false;
        return tokenId;
    }
    
    m_hasNextToken=((m_tokensPredicted == -1) || (m_tokensLeft != 0));
    return tokenId;
}

std::string LlamaModel::doCompletion()
{
    llama_token token=nextToken();
    std::string tokenText;
    
    if(token != -1)
    {
        tokenText=llama_token_to_str(m_context, token);
        m_generatedText+=tokenText;
    }

    if(m_multibytePending > 0)
    {
        m_multibytePending -= tokenText.size();
    }
    else if(tokenText.size() == 1)
    {
        const char &c=tokenText[0];

        // 2-byte characters: 110xxxxx 10xxxxxx
        if((c & 0xE0) == 0xC0)
        {
            m_multibytePending = 1;
        // 3-byte characters: 1110xxxx 10xxxxxx 10xxxxxx
        }
        else if((c & 0xF0) == 0xE0)
        {
            m_multibytePending = 2;
        // 4-byte characters: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        }
        else if((c & 0xF8) == 0xF0)
        {
            m_multibytePending = 3;
        }
        else
        {
            m_multibytePending = 0;
        }
    }

    if(m_multibytePending > 0 && !m_hasNextToken)
    {
        m_hasNextToken=true;
        m_tokensLeft++;
    }

    if(!m_hasNextToken && m_tokensLeft == 0)
    {
        m_stopType=StopType::Limit;
    }

    return tokenText;
}

std::vector<float> LlamaModel::embedding(const std::string &content)
{
    std::vector<float> embeddings;
    std::string input=' '+content;
    std::vector<llama_token> tokens = llamaTokenize(m_context, input, true);

    if(tokens.size() > 0)
    {
        if(llama_eval(m_context, tokens.data(), tokens.size(), 0, m_threadCount))
        {
            fprintf(stderr, "%s : failed to eval\n", __func__);
            return embeddings;
        }
    }
    const int n_embd=llama_n_embd(m_context);
    float *embedding=llama_get_embeddings(m_context);

    embeddings.insert(embeddings.end(), embedding, embedding+n_embd);
    return embeddings;
}

void LlamaModel::reset()
{
//    m_antiPrompt.clear();
    m_generatedText="";
    m_generatedText.reserve(m_params.n_ctx);
    m_stoppingWord="";
    m_stopType=StopType::None;
    
    m_tokensPredicted=0;
}

}//namespace oais