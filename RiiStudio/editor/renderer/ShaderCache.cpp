#include "ShaderCache.hpp"


std::map<std::pair<std::string, std::string>, std::unique_ptr<ShaderProgram>> ShaderCache::mShaders;


ShaderProgram& ShaderCache::compile(const std::string& vert, const std::string& frag)
{
    const auto key = std::pair<std::string, std::string>(vert, frag);
    const auto found = mShaders.find(key);
    if (found != mShaders.end())
        return *found->second.get();
    auto new_program = std::make_unique<ShaderProgram>(vert, frag);
    ShaderProgram& ref = *new_program.get();
    mShaders.emplace(key, std::move(new_program));
    return ref;
}