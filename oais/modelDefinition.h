#ifndef _oais_modelDefinition_h_
#define _oais_modelDefinition_h_

#include <string>

namespace oais
{

struct ModelDefinition
{
    std::string name;
    std::string path;
};

bool readModelDefinition(ModelDefinition &modelDef, const std::string &fileName);

}//namespace oais

#endif//_oais_modelDefinition_h_