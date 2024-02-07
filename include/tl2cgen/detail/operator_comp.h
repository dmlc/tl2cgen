/*!
 * Copyright (c) 2024 by Contributors
 * \file operator_comp.h
 * \author Hyunsu Cho
 * \brief Make comparison with a Treelite operator
 */

#ifndef TL2CGEN_DETAIL_OPERATOR_COMP_H_
#define TL2CGEN_DETAIL_OPERATOR_COMP_H_

#include <tl2cgen/logging.h>
#include <treelite/enum/operator.h>

namespace tl2cgen::detail {

template <typename LHS, typename RHS>
inline bool CompareWithOp(LHS lhs, treelite::Operator op, RHS rhs) {
  switch (op) {
  case treelite::Operator::kEQ:
    return lhs == rhs;
  case treelite::Operator::kLT:
    return lhs < rhs;
  case treelite::Operator::kLE:
    return lhs <= rhs;
  case treelite::Operator::kGT:
    return lhs > rhs;
  case treelite::Operator::kGE:
    return lhs >= rhs;
  default:
    TL2CGEN_LOG(FATAL) << "Operator undefined: " << static_cast<int>(op);
    return false;
  }
}

}  // namespace tl2cgen::detail

#endif  // TL2CGEN_DETAIL_OPERATOR_COMP_H_
