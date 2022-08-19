
#include "bolt/CST.hpp"
#include "bolt/Scanner.hpp"
#include "bolt/Parser.hpp"
#include "bolt/Diagnostics.hpp" 

namespace bolt {

  Parser::Parser(Stream<Token*>& S):
    Tokens(S) {}

  Token* Parser::peekFirstTokenAfterModifiers() {
    std::size_t I = 0;
    for (;;) {
      auto T0 = Tokens.peek(I++);
      switch (T0->Type) {
        case NodeType::PubKeyword:
        case NodeType::MutKeyword:
          continue;
        default:
          return T0;
      }
    }
  }

#define BOLT_EXPECT_TOKEN(name) \
  { \
    auto __Token = Tokens.get(); \
    if (__Token->Type != NodeType::name) { \
      throw UnexpectedTokenDiagnostic(__Token, std::vector<NodeType> { NodeType::name }); \
    } \
  }

  Pattern* Parser::parsePattern() {
    auto T0 = Tokens.peek();
    switch (T0->Type) {
      case NodeType::Identifier:
        Tokens.get();
        return new BindPattern(static_cast<Identifier*>(T0));
      default:
        throw UnexpectedTokenDiagnostic(T0, std::vector { NodeType::Identifier });
    }
  }

  QualifiedName* Parser::parseQualifiedName() {
    std::vector<Identifier*> ModulePath;
    auto Name = Tokens.get();
    if (Name->Type != NodeType::Identifier) {
      throw UnexpectedTokenDiagnostic(Name, std::vector { NodeType::Identifier });
    }
    for (;;) {
      auto T1 = Tokens.peek();
      if (T1->Type == NodeType::Dot) {
        break;
      }
      Tokens.get();
      ModulePath.push_back(static_cast<Identifier*>(Name));
      Name = Tokens.get();
      if (Name->Type != NodeType::Identifier) {
        throw UnexpectedTokenDiagnostic(Name, std::vector { NodeType::Identifier });
      }
    }
    return new QualifiedName(ModulePath, static_cast<Identifier*>(Name));
  }

  TypeExpression* Parser::parseTypeExpression() {
    auto T0 = Tokens.peek();
    switch (T0->Type) {
      case NodeType::Identifier:
        return new ReferenceTypeExpression(parseQualifiedName());
      default:
        throw UnexpectedTokenDiagnostic(T0, std::vector { NodeType::Identifier });
    }
  }

  Expression* Parser::parseExpression() {
    auto T0 = Tokens.peek();
    switch (T0->Type) {
      case NodeType::Identifier:
        Tokens.get();
        return new ReferenceExpression(static_cast<Identifier*>(T0));
      case NodeType::IntegerLiteral:
      case NodeType::StringLiteral:
        Tokens.get();
        return new ConstantExpression(T0);
      default:
        throw UnexpectedTokenDiagnostic(T0, std::vector { NodeType::Identifier, NodeType::IntegerLiteral });
    }
  }

  ExpressionStatement* Parser::parseExpressionStatement() {
    auto E = parseExpression();
    BOLT_EXPECT_TOKEN(LineFoldEnd);
    return new ExpressionStatement(E);
  }

  LetDeclaration* Parser::parseLetDeclaration() {

    PubKeyword* Pub;
    LetKeyword* Let;
    MutKeyword* Mut;
    auto T0 = Tokens.get();
    if (T0->Type == NodeType::PubKeyword) {
      Pub = static_cast<PubKeyword*>(T0);
      T0 = Tokens.get();
    }
    if (T0->Type != NodeType::LetKeyword) {
      throw UnexpectedTokenDiagnostic(T0, std::vector { NodeType::LetKeyword });
    }
    Let = static_cast<LetKeyword*>(T0);
    auto T1 = Tokens.peek();
    if (T1->Type == NodeType::MutKeyword) {
      Mut = static_cast<MutKeyword*>(T1);
      Tokens.get();
    }

    auto Patt = parsePattern();

    std::vector<Param*> Params;
    Token* T2;
    for (;;) {
      T2 = Tokens.peek();
      switch (T2->Type) {
        case NodeType::LineFoldEnd:
        case NodeType::BlockStart:
        case NodeType::Equals:
        case NodeType::Colon:
          goto after_params;
        default:
          Params.push_back(new Param(parsePattern(), nullptr));
      }
    }

after_params:

    TypeAssert* TA = nullptr;
    if (T2->Type == NodeType::Colon) {
      Tokens.get();
      auto TE = parseTypeExpression();
      TA = new TypeAssert(static_cast<Colon*>(T2), TE);
      T2 = Tokens.peek();
    }

    LetBody* Body;
    switch (T2->Type) {
      case NodeType::BlockStart:
      {
        Tokens.get();
        std::vector<LetBodyElement*> Elements;
        for (;;) {
          auto T3 = Tokens.peek();
          if (T3->Type == NodeType::BlockEnd) {
            break;
          }
          Elements.push_back(parseLetBodyElement());
        }
        Tokens.get();
        Body = new LetBlockBody(static_cast<BlockStart*>(T2), Elements);
        break;
      }
      case NodeType::Equals:
        Tokens.get();
        Body = new LetExprBody(static_cast<Equals*>(T2), parseExpression());
        break;
      case NodeType::LineFoldEnd:
        Body = nullptr;
        break;
      default:
        std::vector<NodeType> Expected { NodeType::BlockStart, NodeType::LineFoldEnd, NodeType::Equals };
        if (TA == nullptr) {
          // First tokens of TypeAssert
          Expected.push_back(NodeType::Colon);
          // First tokens of Pattern
          Expected.push_back(NodeType::Identifier);
        }
        throw UnexpectedTokenDiagnostic(T2, Expected);
    }

    BOLT_EXPECT_TOKEN(LineFoldEnd);

    return new LetDeclaration(
      Pub,
      Let,
      Mut,
      Patt,
      Params,
      TA,
      Body
    );
  }

  LetBodyElement* Parser::parseLetBodyElement() {
    auto T0 = peekFirstTokenAfterModifiers();
    switch (T0->Type) {
      case NodeType::LetKeyword:
        return parseLetDeclaration();
      default:
        return parseExpressionStatement();
    }
  }

  SourceElement* Parser::parseSourceElement() {
    auto T0 = peekFirstTokenAfterModifiers();
    switch (T0->Type) {
      case NodeType::LetKeyword:
        return parseLetDeclaration();
      default:
        return parseExpressionStatement();
    }
  }

  SourceFile* Parser::parseSourceFile() {
    std::vector<SourceElement*> Elements;
    for (;;) {
      auto T0 = Tokens.peek();
      if (T0->Type == NodeType::EndOfFile) {
        break;
      }
      Elements.push_back(parseSourceElement());
    }
    return new SourceFile(Elements);
  }

}

