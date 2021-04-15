/******************************************************************************
 * Top contributors (to current version):
 *   Aina Niemetz, Mathias Preiner
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * A bit-blaster wrapper around BBSimple for proof logging.
 */
#include "theory/bv/bitblast/proof_bitblaster.h"

#include <unordered_set>

#include "expr/term_conversion_proof_generator.h"
#include "theory/theory_model.h"

namespace cvc5 {
namespace theory {
namespace bv {

std::unordered_map<Kind, PfRule, kind::KindHashFunction>
    BBProof::s_kindToPfRule = {
        {kind::CONST_BITVECTOR, PfRule::BV_BITBLAST_CONST},
        {kind::EQUAL, PfRule::BV_BITBLAST_EQUAL},
        {kind::BITVECTOR_ULT, PfRule::BV_BITBLAST_ULT},
        {kind::BITVECTOR_ULE, PfRule::BV_BITBLAST_ULE},
        {kind::BITVECTOR_UGT, PfRule::BV_BITBLAST_UGT},
        {kind::BITVECTOR_UGE, PfRule::BV_BITBLAST_UGE},
        {kind::BITVECTOR_SLT, PfRule::BV_BITBLAST_SLT},
        {kind::BITVECTOR_SLE, PfRule::BV_BITBLAST_SLE},
        {kind::BITVECTOR_SGT, PfRule::BV_BITBLAST_SGT},
        {kind::BITVECTOR_SGE, PfRule::BV_BITBLAST_SGE},
        {kind::BITVECTOR_NOT, PfRule::BV_BITBLAST_NOT},
        {kind::BITVECTOR_CONCAT, PfRule::BV_BITBLAST_CONCAT},
        {kind::BITVECTOR_AND, PfRule::BV_BITBLAST_AND},
        {kind::BITVECTOR_OR, PfRule::BV_BITBLAST_OR},
        {kind::BITVECTOR_XOR, PfRule::BV_BITBLAST_XOR},
        {kind::BITVECTOR_XNOR, PfRule::BV_BITBLAST_XNOR},
        {kind::BITVECTOR_NAND, PfRule::BV_BITBLAST_NAND},
        {kind::BITVECTOR_NOR, PfRule::BV_BITBLAST_NOR},
        {kind::BITVECTOR_COMP, PfRule::BV_BITBLAST_COMP},
        {kind::BITVECTOR_MULT, PfRule::BV_BITBLAST_MULT},
        {kind::BITVECTOR_PLUS, PfRule::BV_BITBLAST_PLUS},
        {kind::BITVECTOR_SUB, PfRule::BV_BITBLAST_SUB},
        {kind::BITVECTOR_NEG, PfRule::BV_BITBLAST_NEG},
        {kind::BITVECTOR_UDIV, PfRule::BV_BITBLAST_UDIV},
        {kind::BITVECTOR_UREM, PfRule::BV_BITBLAST_UREM},
        {kind::BITVECTOR_SDIV, PfRule::BV_BITBLAST_SDIV},
        {kind::BITVECTOR_SREM, PfRule::BV_BITBLAST_SREM},
        {kind::BITVECTOR_SMOD, PfRule::BV_BITBLAST_SMOD},
        {kind::BITVECTOR_SHL, PfRule::BV_BITBLAST_SHL},
        {kind::BITVECTOR_LSHR, PfRule::BV_BITBLAST_LSHR},
        {kind::BITVECTOR_ASHR, PfRule::BV_BITBLAST_ASHR},
        {kind::BITVECTOR_ULTBV, PfRule::BV_BITBLAST_ULTBV},
        {kind::BITVECTOR_SLTBV, PfRule::BV_BITBLAST_SLTBV},
        {kind::BITVECTOR_ITE, PfRule::BV_BITBLAST_ITE},
        {kind::BITVECTOR_EXTRACT, PfRule::BV_BITBLAST_EXTRACT},
        {kind::BITVECTOR_REPEAT, PfRule::BV_BITBLAST_REPEAT},
        {kind::BITVECTOR_ZERO_EXTEND, PfRule::BV_BITBLAST_ZERO_EXTEND},
        {kind::BITVECTOR_SIGN_EXTEND, PfRule::BV_BITBLAST_SIGN_EXTEND},
        {kind::BITVECTOR_ROTATE_RIGHT, PfRule::BV_BITBLAST_ROTATE_RIGHT},
        {kind::BITVECTOR_ROTATE_LEFT, PfRule::BV_BITBLAST_ROTATE_LEFT},
};

BBProof::BBProof(TheoryState* state, ProofNodeManager* pnm)
    : d_bb(new BBSimple(state)),
      d_pnm(pnm),
      d_tcpg(new TConvProofGenerator(pnm,
                                     nullptr,
                                     TConvPolicy::FIXPOINT,
                                     TConvCachePolicy::NEVER,
                                     "BBProof::TConvProofGenerator",
                                     nullptr,
                                     true))
{
}

BBProof::~BBProof() {}

void BBProof::bbAtom(TNode node)
{
  std::vector<TNode> visit;
  visit.push_back(node);
  std::unordered_set<TNode, TNodeHashFunction> visited;
  NodeManager* nm = NodeManager::currentNM();
  while (!visit.empty())
  {
    TNode n = visit.back();
    if (hasBBAtom(n) || hasBBTerm(n))
    {
      visit.pop_back();
      continue;
    }

    if (visited.find(n) == visited.end())
    {
      visited.insert(n);
      if (!Theory::isLeafOf(n, theory::THEORY_BV))
      {
        visit.insert(visit.end(), n.begin(), n.end());
      }
    }
    else
    {
      if (Theory::isLeafOf(n, theory::THEORY_BV) && !n.isConst())
      {
        Bits bits;
        d_bb->makeVariable(n, bits);
        if (isProofsEnabled())
        {
          Node n_tobv = nm->mkNode(kind::BITVECTOR_TO_BV, bits);
          d_bbMap.emplace(n, n_tobv);
          d_tcpg->addRewriteStep(
              n, n_tobv, PfRule::BV_BITBLAST_VAR, {}, {}, false);
        }
      }
      else if (n.getType().isBitVector())
      {
        Bits bits;
        d_bb->bbTerm(n, bits);
        Kind kind = n.getKind();
        if (isProofsEnabled())
        {
          Node n_tobv = nm->mkNode(kind::BITVECTOR_TO_BV, bits);
          d_bbMap.emplace(n, n_tobv);
          Node c_tobv;
          if (n.isConst())
          {
            c_tobv = n;
          }
          else
          {
            std::vector<Node> children_tobv;
            if (n.getMetaKind() == kind::metakind::PARAMETERIZED)
            {
              children_tobv.push_back(n.getOperator());
            }

            for (const auto& child : n)
            {
              children_tobv.push_back(d_bbMap.at(child));
            }
            c_tobv = nm->mkNode(kind, children_tobv);
          }
          d_tcpg->addRewriteStep(
              c_tobv, n_tobv, s_kindToPfRule.at(kind), {}, {}, false);
        }
      }
      else
      {
        d_bb->bbAtom(n);
        if (isProofsEnabled())
        {
          Node n_tobv = getStoredBBAtom(n);
          std::vector<Node> children_tobv;
          for (const auto& child : n)
          {
            children_tobv.push_back(d_bbMap.at(child));
          }
          Node c_tobv = nm->mkNode(n.getKind(), children_tobv);
          d_tcpg->addRewriteStep(
              c_tobv, n_tobv, s_kindToPfRule.at(n.getKind()), {}, {}, false);
        }
      }
      visit.pop_back();
    }
  }
}

bool BBProof::hasBBAtom(TNode atom) const { return d_bb->hasBBAtom(atom); }

bool BBProof::hasBBTerm(TNode atom) const { return d_bb->hasBBTerm(atom); }

Node BBProof::getStoredBBAtom(TNode node)
{
  return d_bb->getStoredBBAtom(node);
}

bool BBProof::collectModelValues(TheoryModel* m,
                                 const std::set<Node>& relevantTerms)
{
  return d_bb->collectModelValues(m, relevantTerms);
}

bool BBProof::isProofsEnabled() const { return d_pnm != nullptr; }

}  // namespace bv
}  // namespace theory
}  // namespace cvc5
