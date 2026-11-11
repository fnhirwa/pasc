#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "arena.h"
#include "codegen_x86.h"
#include "diag.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"

#ifndef PASC_RT_DEFAULT
#define PASC_RT_DEFAULT "libpasrt.a"
#endif

namespace {

int usage() {
  std::cerr << "usage: pasc [options] file.pas\n"
               "  -o <file>       output file (default: a.out, or out.s with -S)\n"
               "  -S              write assembly and stop\n"
               "  --rt <file>     runtime library (default: $PASC_RT, then the\n"
               "                  library built next to the compiler)\n"
               "  --dump-tokens   print the token stream and stop\n"
               "  --dump-ast      print the typed AST and stop\n";
  return 2;
}

std::string shellQuote(const std::string& s) {
  std::string out = "'";
  for (char c : s) {
    if (c == '\'') out += "'\\''";
    else out += c;
  }
  return out + "'";
}

}  // namespace

int main(int argc, char** argv) {
  std::string input, output, runtime;
  bool asmOnly = false, dumpTokens = false, dumpTree = false;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "-o" && i + 1 < argc) output = argv[++i];
    else if (a == "--rt" && i + 1 < argc) runtime = argv[++i];
    else if (a == "-S") asmOnly = true;
    else if (a == "--dump-tokens") dumpTokens = true;
    else if (a == "--dump-ast") dumpTree = true;
    else if (!a.empty() && a[0] == '-') return usage();
    else if (input.empty()) input = a;
    else return usage();
  }
  if (input.empty()) return usage();

  std::ifstream in(input, std::ios::binary);
  if (!in) {
    std::cerr << "pasc: cannot open " << input << '\n';
    return 2;
  }
  std::stringstream buffer;
  buffer << in.rdbuf();
  const std::string source = buffer.str();

  Diag diag(input);
  if (dumpTokens) {
    Lexer lexer(source, diag);
    for (Token t = lexer.next();; t = lexer.next()) {
      std::cout << t.loc.line << ':' << t.loc.col << '\t' << tokName(t.kind);
      if (!t.text.empty()) std::cout << '\t' << t.text;
      std::cout << '\n';
      if (t.kind == Tok::Eof) break;
    }
    return diag.errorCount() ? 1 : 0;
  }

  Arena arena;
  TypeContext types;
  Lexer lexer(source, diag);
  Parser parser(lexer, arena, diag);
  Program* program = parser.parseProgram();
  // 5.1 e): a program with violations is reported and not executed.
  if (diag.errorCount()) return 1;
  Sema sema(types, arena, diag);
  sema.check(*program);
  if (diag.errorCount()) return 1;

  if (dumpTree) {
    dumpAst(*program);
    return 0;
  }

  if (output.empty()) output = asmOnly ? "out.s" : "a.out";
  std::string asmPath = asmOnly ? output : output + ".s";
  {
    std::ofstream out(asmPath);
    if (!out) {
      std::cerr << "pasc: cannot write " << asmPath << '\n';
      return 2;
    }
    CodegenX86 codegen(out);
    codegen.emitProgram(*program);
  }
  if (asmOnly) return 0;

  if (runtime.empty()) {
    const char* env = std::getenv("PASC_RT");
    runtime = env ? env : PASC_RT_DEFAULT;
  }
  std::string cmd = "cc -o " + shellQuote(output) + " " + shellQuote(asmPath) +
                    " " + shellQuote(runtime);
  int status = std::system(cmd.c_str());
  std::remove(asmPath.c_str());
  if (status != 0) {
    std::cerr << "pasc: assembling or linking failed\n";
    return 2;
  }
  return 0;
}
