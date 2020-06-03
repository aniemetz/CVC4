/*********************                                                        */
/*! \file proof_rule.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Proof rule enumeration
 **/

#include "cvc4_private.h"

#ifndef CVC4__EXPR__PROOF_RULE_H
#define CVC4__EXPR__PROOF_RULE_H

#include <iosfwd>

namespace CVC4 {

/**
 * An enumeration for proof rules. This enumeration is analogous to Kind for
 * Node objects. In the documentation below, P:F denotes a ProofNode that
 * proves formula F.
 *
 * Conceptually, the following proof rules form a calculus whose target
 * user is the Node-level theory solvers. This means that the rules below
 * are designed to reason about, among other things, common operations on Node
 * objects like Rewriter::rewrite or Node::substitute. It is intended to be
 * translated or printed in other formats.
 *
 * The following PfRule values include core rules and those categorized by
 * theory, including the theory of equality.
 *
 * The "core rules" include two distinguished rules which have special status:
 * (1) ASSUME, which represents an open leaf in a proof.
 * (2) SCOPE, which closes the scope of assumptions.
 * The core rules additionally correspond to generic operations that are done
 * internally on nodes, e.g. calling Rewriter::rewrite.
 */
enum class PfRule : uint32_t
{
  //================================================= Core rules
  //======================== Assume and Scope
  // ======== Assumption (a leaf)
  // Children: none
  // Arguments: (F)
  // --------------
  // Conclusion: F
  //
  // This rule has special status, in that an application of assume is an
  // open leaf in a proof that is not (yet) justified. An assume leaf is
  // analogous to a free variable in a term, where we say "F is a free
  // assumption in proof P" if it contains an application of F that is not
  // bound by SCOPE (see below).
  ASSUME,
  // ======== Scope (a binder for assumptions)
  // Children: (P:F)
  // Arguments: (F1, ..., Fn)
  // --------------
  // Conclusion: (=> (and F1 ... Fn) F) or (not (and F1 ... Fn)) if F is false
  //
  // This rule has a dual purpose with ASSUME. It is a way to close
  // assumptions in a proof. We require that F1 ... Fn are free assumptions in
  // P and say that F1, ..., Fn are not free in (SCOPE P). In other words, they
  // are bound by this application. For example, the proof node:
  //   (SCOPE (ASSUME F) :args F)
  // has the conclusion (=> F F) and has no free assumptions. More generally, a
  // proof with no free assumptions always concludes a valid formula.
  SCOPE,

  //======================== Builtin theory (common node operations)
  // ======== Substitution
  // Children: (P1:F1, ..., Pn:Fn)
  // Arguments: (t, (ids)?)
  // ---------------------------------------------------------------
  // Conclusion: (= t t*sigma{ids}(Fn)*...*sigma{ids}(F1))
  // where sigma{ids}(Fi) are substitutions, which notice are applied in
  // reverse order.
  // Notice that ids is a MethodId identifier, which determines how to convert
  // the formulas F1, ..., Fn into substitutions.
  SUBS,
  // ======== Rewrite
  // Children: none
  // Arguments: (t, (idr)?)
  // ----------------------------------------
  // Conclusion: (= t Rewriter{idr}(t))
  // where idr is a MethodId identifier, which determines the kind of rewriter
  // to apply, e.g. Rewriter::rewrite.
  REWRITE,
  // ======== Substitution + Rewriting equality introduction
  //
  // In this rule, we provide a term t and conclude that it is equal to its
  // rewritten form under a (proven) substitution.
  //
  // Children: (P1:F1, ..., Pn:Fn)
  // Arguments: (t, (ids (idr)?)?)
  // ---------------------------------------------------------------
  // Conclusion: (= t t')
  // where
  //   t' is 
  //   toWitness(Rewriter{idr}(toSkolem(t)*sigma{ids}(Fn)*...*sigma{ids}(F1)))
  //   toSkolem(...) converts terms from witness form to Skolem form,
  //   toWitness(...) converts terms from Skolem form to witness form.
  //
  // Notice that:
  //   toSkolem(t')=Rewriter{idr}(toSkolem(t)*sigma{ids}(Fn)*...*sigma{ids}(F1))
  // In other words, from the point of view of Skolem forms, this rule
  // transforms t to t' by standard substitution + rewriting.
  //
  // The argument ids and idr is optional and specify the identifier of the
  // substitution and rewriter respectively to be used. For details, see
  // theory/builtin/proof_checker.h.
  MACRO_SR_EQ_INTRO,
  // ======== Substitution + Rewriting predicate introduction
  //
  // In this rule, we provide a formula F and conclude it, under the condition
  // that it rewrites to true under a proven substitution.
  //
  // Children: (P1:F1, ..., Pn:Fn)
  // Arguments: (F, (ids (idr)?)?)
  // ---------------------------------------------------------------
  // Conclusion: F
  // where
  //   Rewriter{idr}(F*sigma{ids}(Fn)*...*sigma{ids}(F1)) == true
  // where ids and idr are method identifiers.
  //
  // Notice that we apply rewriting on the witness form of F, meaning that this
  // rule may conclude an F whose Skolem form is justified by the definition of
  // its (fresh) Skolem variables. Furthermore, notice that the rewriting and
  // substitution is applied only within the side condition, meaning the
  // rewritten form of the witness form of F does not escape this rule.
  MACRO_SR_PRED_INTRO,
  // ======== Substitution + Rewriting predicate elimination
  //
  // In this rule, if we have proven a formula F, then we may conclude its
  // rewritten form under a proven substitution.
  //
  // Children: (P1:F, P2:F1, ..., P_{n+1}:Fn)
  // Arguments: ((ids (idr)?)?)
  // ----------------------------------------
  // Conclusion: F'
  // where
  //   F' is
  //   toWitness(Rewriter{idr}(toSkolem(F)*sigma{ids}(Fn)*...*sigma{ids}(F1)).
  // where ids and idr are method identifiers.
  //
  // We rewrite only on the Skolem form of F, similar to MACRO_SR_EQ_INTRO.
  MACRO_SR_PRED_ELIM,
  // ======== Substitution + Rewriting predicate transform
  //
  // In this rule, if we have proven a formula F, then we may provide a formula
  // G and conclude it if F and G are equivalent after rewriting under a proven
  // substitution.
  //
  // Children: (P1:F, P2:F1, ..., P_{n+1}:Fn)
  // Arguments: (G, (ids (idr)?)?)
  // ----------------------------------------
  // Conclusion: G
  // where
  //   Rewriter{idr}(F*sigma{ids}(Fn)*...*sigma{ids}(F1)) ==
  //   Rewriter{idr}(G*sigma{ids}(Fn)*...*sigma{ids}(F1))
  //
  // Notice that we apply rewriting on the witness form of F and G, similar to
  // MACRO_SR_PRED_INTRO.
  MACRO_SR_PRED_TRANSFORM,
  
  //================================================= Unknown rule
  UNKNOWN,
};

/**
 * Converts a proof rule to a string. Note: This function is also used in
 * `safe_print()`. Changing this function name or signature will result in
 * `safe_print()` printing "<unsupported>" instead of the proper strings for
 * the enum values.
 *
 * @param id The proof rule
 * @return The name of the proof rule
 */
const char* toString(PfRule id);

/**
 * Writes a proof rule name to a stream.
 *
 * @param out The stream to write to
 * @param id The proof rule to write to the stream
 * @return The stream
 */
std::ostream& operator<<(std::ostream& out, PfRule id);

}  // namespace CVC4

#endif /* CVC4__EXPR__PROOF_RULE_H */
