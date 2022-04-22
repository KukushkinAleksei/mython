#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <set>
#include <sstream>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

bool valididsimbol(char ch) {
  return (ch == '_') || isalnum(ch);
}

bool Lexer::ParseNumber(char& ch, std::istream& inp) {
  if (isdigit(ch)) {
    ostringstream num_buffer;
    num_buffer << ch;
    while (inp.get(ch) && isdigit(ch)) {
      num_buffer << ch;
    }
    inp.putback(ch);
    int value = std::stoi(num_buffer.str());
    _tokens.push_back(token_type::Number{ value });
    return true;
  }
  return false;
}

bool Lexer::ParseIdentifer(char& ch, std::istream& inp) {
  if (isalpha(ch) || ch == '_') {
    ostringstream buffer;
    
    buffer << ch;
    while (inp.get(ch) && valididsimbol(ch)) {
      buffer << ch;
    }

    std::string id = buffer.str();
    if (id == "print"s) {
      _tokens.push_back(token_type::Print());
    }
    else if (id == "class"s) {
      _tokens.push_back(token_type::Class());
    }
    else if (id == "return"s) {
      _tokens.push_back(token_type::Return());
    }
    else if (id == "if"s) {
      _tokens.push_back(token_type::If());
    }
    else if (id == "else"s) {
      _tokens.push_back(token_type::Else());
    }
    else if (id == "def"s) {
      _tokens.push_back(token_type::Def());
    }
    else if (id == "and"s) {
      _tokens.push_back(token_type::And());
    }
    else if (id == "or"s) {
      _tokens.push_back(token_type::Or());
    }
    else if (id == "not"s) {
      _tokens.push_back(token_type::Not());
    }
    else if (id == "True"s) {
      _tokens.push_back(token_type::True());
    }
    else if (id == "False"s) {
      _tokens.push_back(token_type::False());
    }
    else if (id == "None"s) {
      _tokens.push_back(token_type::None());
    }
    else {
      _tokens.push_back(token_type::Id{ buffer.str() });
    }
    inp.putback(ch);
    return true;
  }
  return false;
}

bool Lexer::ParseSymbol(char& ch, std::istream& inp) {
  static const std::set<char> singl_symbol = {
    '+' ,'-' ,'=', '*','/','<','>',':',',','.','(',')'};
  static const std::set<char> doubl_symbol_start = { '=', '!','<','>' };

  if (doubl_symbol_start.count(ch) || singl_symbol.count(ch)) {

    if (doubl_symbol_start.count(ch)) {
      char next = inp.get();
      if (next == '=') {
        switch (ch) {
        case '=':
          _tokens.push_back(token_type::Eq{});
          return true;
        case '!':
          _tokens.push_back(token_type::NotEq{});
          return true;
        case '<':
          _tokens.push_back(token_type::LessOrEq{});
          return true;
        case '>':
          _tokens.push_back(token_type::GreaterOrEq{});
          return true;
        default:
          throw LexerError("Bad multysimbol bool operation");
        }
      }
      else {
        inp.putback(next);
      }
    }
    if (singl_symbol.count(ch)) {
      _tokens.push_back(token_type::Char{ ch });
    }
    return true;
  }
  return false;
}

bool Lexer::ParseString(char& ch, std::istream& inp){
  if (ch == '\'' || ch == '\"') {
    char start = ch;
    ostringstream buffer;
    while (inp.get(ch) && (ch != start)) {
      if (ch == '\\') {
        char next;
        inp.get(next);
        if (next == '\'')
          buffer << '\'';
        else if (next == '\"')
          buffer << '\"';
        else
          buffer << ch << next;        
      }
      else {
        buffer << ch;
      }
    }
    _tokens.push_back(token_type::String{ buffer.str() });
    return true;
  }
  return false;
}

bool Lexer::ParseComment(char& ch, std::istream& inp) {
  if (ch == '#') {
    
    while (inp.get(ch) && (ch != '\n')) {

    }
    return true;
  }
  return false;
}

bool istruespace(char ch) {
  return ch == ' ';
}

Lexer::Lexer(std::istream& inp) {
    // Реализуйте конструктор самостоятельно
  char ch = '\0';
  bool new_line = true;
  int current_indent = 0;
  while (inp.get(ch)) {

    ParseComment(ch, inp);

    if (new_line && (istruespace(ch) || current_indent > 0)) {
      int counter = 0;

      if (istruespace(ch)) {
        ++counter;
      }
      else {
        inp.putback(ch);
      }

      while (inp.get(ch) && istruespace(ch)) {
        ++counter;
      }
      //empty line
      if (ch != '\n') {
        if (counter % 2 != 0) {
          throw LexerError("Indent must be multiple of two spaces");
        }
        int new_indent = counter / 2;
        int ident_delta = new_indent - current_indent;
        if (ident_delta == 1) {
          _tokens.push_back(token_type::Indent());
          ++current_indent;
        }
        else if (ident_delta < 0) {
          for (int i = 0; i > ident_delta; --i) {
            _tokens.push_back(token_type::Dedent());
            --current_indent;
          }
        }
        else if (ident_delta != 0) {
          throw LexerError("Too big change of indent");
        }
      }
      else {
        continue;
      }
    }
    
    if (new_line && ch == '\n')
      continue;

    new_line = false;

    //new line
    if (ch == '\n') {
      _tokens.push_back(token_type::Newline());
      new_line = true;
      continue;
    }
    //skeep single space
    if (istruespace(ch))
      continue;

    if (ParseNumber(ch, inp))
      continue;

    if (ParseIdentifer(ch, inp))
      continue;

    if (ParseSymbol(ch, inp))
      continue;

    if (ParseString(ch, inp))
      continue;
  }

  if (current_indent > 0) {
    for (int i = 0; i < current_indent; ++i) {
      _tokens.push_back(token_type::Dedent());      
    }
  }
  if (_tokens.size() > 0 &&
    !std::holds_alternative<token_type::Newline>(_tokens.back()) &&
    !std::holds_alternative<token_type::Dedent>(_tokens.back())) {
    _tokens.push_back(token_type::Newline());
  }

  _tokens.push_back(token_type::Eof());
  _current_token = _tokens.cbegin();
}

const Token& Lexer::CurrentToken() const {
  return *_current_token;
}

Token Lexer::NextToken() {
  if (_current_token + 1 != _tokens.cend()) {
    ++_current_token;
  }
  return *_current_token;
}

}  // namespace parse