#include "lexer.h"
#include "runtime.h"
#include "test_runner_p.h"

#include <iostream>

namespace parse {
void RunOpenLexerTests(TestRunner& tr);
}
namespace runtime {
void RunObjectHolderTests(TestRunner& tr);
void RunObjectsTests(TestRunner& tr);
}  // namespace runtime

namespace {

  void TestAll() {
    TestRunner tr;
    runtime::RunObjectHolderTests(tr);
    runtime::RunObjectsTests(tr);
  }
}

int TestLexer() {
    try {
        TestRunner tr;
        parse::RunOpenLexerTests(tr);
        parse::Lexer lexer(std::cin);
        parse::Token t;
        while ((t = lexer.CurrentToken()) != parse::token_type::Eof{}) {
            std::cout << t << std::endl;
            lexer.NextToken();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
    return 0;
}

int main() { 
  TestAll();
  //TestLexer();

  return 0;
}