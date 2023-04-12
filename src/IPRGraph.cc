
#include <stack>
#include <unordered_set>

#include "zen/config.hpp"

#include "bolt/CST.hpp"
#include "bolt/IPRGraph.hpp"

namespace bolt {

  void IPRGraph::populate(Node* X, Node* Decl) {

    switch (X->Type) {

      case NodeType::SourceFile:
      {
        auto Y = static_cast<SourceFile*>(X);
        for (auto Element: Y->Elements) {
          populate(Element, Decl);
        }
        break;
      }

      case NodeType::IfStatement:
      {
        auto Y = static_cast<IfStatement*>(X);
        for (auto Part: Y->Parts) {
          for (auto Element: Part->Elements) {
            populate(Element, Decl);
          }
        }
        break;
      }

      case NodeType::LetDeclaration:
      {
        auto Y = static_cast<LetDeclaration*>(X);
        if (Y->Body) {
          switch (Y->Body->Type) {
            case NodeType::LetBlockBody:
            {
              auto Z = static_cast<LetBlockBody*>(Y->Body);
              for (auto Element: Z->Elements) {
                populate(Element, Y);
              }
              break;
            }
            case NodeType::LetExprBody:
            {
              auto Z = static_cast<LetExprBody*>(Y->Body);
              populate(Z->Expression, Y);
              break;
            }
            default:
              ZEN_UNREACHABLE
          }
        }
        break;
      }

      case NodeType::ConstantExpression:
        break;

      case NodeType::CallExpression:
      {
        auto Y = static_cast<CallExpression*>(X);
        populate(Y->Function, Decl);
        for (auto Arg: Y->Args) {
          populate(Arg, Decl);
        }
        break;
      }

      case NodeType::ReferenceExpression:
      {
        auto Y = static_cast<ReferenceExpression*>(X);
        auto Def = Y->getScope()->lookup(Y->Name->getSymbolPath());
        ZEN_ASSERT(Def != nullptr);
        if (Decl != nullptr) {
          Edges.emplace(Decl, Y);
        }
        Edges.emplace(Y, Def);
        break;
      }

      default:
        ZEN_UNREACHABLE

    }

  }

  /* bool IPRGraph::isRecursive(ReferenceExpression* From) { */
  /*   std::unordered_set<Node*> Visited; */
  /*   std::stack<Node*> Queue; */
  /*   while (Queue.size()) { */
  /*     auto A = Queue.top(); */
  /*     Queue.pop(); */
  /*     if (Visited.count(A)) { */
  /*       return true; */
  /*     } */
  /*     for (auto B: getOutEdges(A)) { */
  /*     } */
  /*   } */
  /*   return false; */
  /* } */

}
