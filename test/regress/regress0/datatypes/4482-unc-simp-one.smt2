(set-logic ALL)
(set-info :status unsat)
(declare-fun a () Tuple)
(assert (distinct a mkTuple))
(check-sat)