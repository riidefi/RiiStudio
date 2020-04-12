/* @file
 * @brief Simple header-only string parser for text files.
 */

#pragma once

#include <sstream>
#include <string>
#include <vector>

class Parser {
public:
  Parser(std::stringstream& stream) {
    std::string line;
    while (std::getline(stream, line)) {
      std::istringstream iss(line);
      std::string token;
      while (iss >> token)
        addToken(token);
    }
  }
  ~Parser() = default;

  inline std::string readToken() {
    if (m_index == m_tokens.size())
      return m_tokens.back();

    return m_tokens[m_index++];
  }
  inline std::string getToken() const { return m_tokens[m_index]; }
  inline bool atEnd() const { return m_index == m_tokens.size(); }
  inline bool atIs(const std::string& checkFor) const {
    return m_tokens[m_index] == checkFor;
  }

  inline void addToken(std::string& toAdd) { m_tokens.push_back(toAdd); }
  inline void setTokens(std::vector<std::string>& setTo) { m_tokens = setTo; }
  inline const std::vector<std::string>& getTokens() const { return m_tokens; }
  inline std::size_t getIndex() const { return m_index; }

private:
  std::vector<std::string> m_tokens;
  std::size_t m_index = 0;
};
