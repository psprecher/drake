#include "drake/common/symbolic_formula.h"

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "drake/common/drake_assert.h"
#include "drake/common/hash_combine.h"
#include "drake/common/symbolic_environment.h"
#include "drake/common/symbolic_expression.h"
#include "drake/common/symbolic_variable.h"
#include "drake/common/symbolic_variables.h"

namespace drake {
namespace symbolic {

using std::hash;
using std::make_shared;
using std::ostream;
using std::ostringstream;
using std::runtime_error;
using std::shared_ptr;
using std::string;

Formula::Formula(shared_ptr<FormulaCell> const ptr) : ptr_{ptr} {}

FormulaKind Formula::get_kind() const {
  DRAKE_ASSERT(ptr_ != nullptr);
  return ptr_->get_kind();
}

size_t Formula::get_hash() const {
  DRAKE_ASSERT(ptr_ != nullptr);
  return ptr_->get_hash();
}

symbolic::Variables Formula::GetFreeVariables() const {
  DRAKE_ASSERT(ptr_ != nullptr);
  return ptr_->GetFreeVariables();
}

bool Formula::EqualTo(const Formula& f) const {
  DRAKE_ASSERT(ptr_ != nullptr);
  DRAKE_ASSERT(f.ptr_ != nullptr);
  if (ptr_ == f.ptr_) {
    return true;
  }
  if (get_kind() != f.get_kind()) {
    return false;
  }
  if (get_hash() != f.get_hash()) {
    return false;
  }
  // Same kind/hash, but it could be the result of hash collision,
  // check structural equality.
  return ptr_->EqualTo(*(f.ptr_));
}

bool Formula::Evaluate(const Environment& env) const {
  DRAKE_ASSERT(ptr_ != nullptr);
  return ptr_->Evaluate(env);
}

string Formula::to_string() const {
  ostringstream oss;
  oss << *this;
  return oss.str();
}

Formula Formula::True() {
  static Formula tt{make_shared<FormulaTrue>()};
  return tt;
}
Formula Formula::False() {
  static Formula ff{make_shared<FormulaFalse>()};
  return ff;
}

Formula forall(const Variables& vars, const Formula& f) {
  return Formula{make_shared<FormulaForall>(vars, f)};
}

Formula operator&&(const Formula& f1, const Formula& f2) {
  // ff && x => ff    x && ff => ff
  if (f1.get_kind() == FormulaKind::False ||
      f2.get_kind() == FormulaKind::False) {
    return Formula::False();
  }
  // tt && f2 => f2
  if (f1.get_kind() == FormulaKind::True) {
    return f2;
  }
  // f1 && tt => f1
  if (f2.get_kind() == FormulaKind::True) {
    return f1;
  }
  return Formula{make_shared<FormulaAnd>(f1, f2)};
}

Formula operator||(const Formula& f1, const Formula& f2) {
  // tt || x => tt    x && tt => tt
  if (f1.get_kind() == FormulaKind::True ||
      f2.get_kind() == FormulaKind::True) {
    return Formula::True();
  }
  // ff && f2 => f2
  if (f1.get_kind() == FormulaKind::False) {
    return f2;
  }
  // f1 && ff => f1
  if (f2.get_kind() == FormulaKind::False) {
    return f1;
  }
  return Formula{make_shared<FormulaOr>(f1, f2)};
}

Formula operator!(const Formula& f) {
  if (f.get_kind() == FormulaKind::True) {
    return Formula::False();
  }
  if (f.get_kind() == FormulaKind::False) {
    return Formula::True();
  }
  return Formula{make_shared<FormulaNot>(f)};
}

ostream& operator<<(ostream& os, const Formula& e) {
  DRAKE_ASSERT(e.ptr_ != nullptr);
  return e.ptr_->Display(os);
}

Formula operator==(const Expression& e1, const Expression& e2) {
  // If e1 and e2 are structurally equal, simplify e1 == e2 to true.
  if (e1.EqualTo(e2)) {
    return Formula::True();
  }
  return Formula{make_shared<FormulaEq>(e1, e2)};
}
Formula operator==(const double v1, const Expression& e2) {
  // use () to avoid a conflict between cpplint and clang-format
  return (Expression{v1}) == e2;
}
Formula operator==(const Expression& e1, const double v2) {
  return e1 == Expression{v2};
}

Formula operator!=(const Expression& e1, const Expression& e2) {
  // If e1 and e2 are structurally equal, simplify e1 != e2 to false.
  if (e1.EqualTo(e2)) {
    return Formula::False();
  }
  return Formula{make_shared<FormulaNeq>(e1, e2)};
}
Formula operator!=(const double v1, const Expression& e2) {
  // use () to avoid a conflict between cpplint and clang-format
  return (Expression{v1}) != e2;
}
Formula operator!=(const Expression& e1, const double v2) {
  return e1 != Expression{v2};
}

Formula operator<(const Expression& e1, const Expression& e2) {
  // simplification: E < E  -->  False
  if (e1.EqualTo(e2)) {
    return Formula::False();
  }
  return Formula{make_shared<FormulaLt>(e1, e2)};
}
Formula operator<(const double v1, const Expression& e2) {
  return Expression{v1} < e2;
}
Formula operator<(const Expression& e1, const double v2) {
  return e1 < Expression{v2};
}

Formula operator<=(const Expression& e1, const Expression& e2) {
  // simplification: E <= E  -->  True
  if (e1.EqualTo(e2)) {
    return Formula::True();
  }
  return Formula{make_shared<FormulaLeq>(e1, e2)};
}
Formula operator<=(const double v1, const Expression& e2) {
  return Expression{v1} <= e2;
}
Formula operator<=(const Expression& e1, const double v2) {
  return e1 <= Expression{v2};
}

Formula operator>(const Expression& e1, const Expression& e2) {
  // simplification: E > E  -->  False
  if (e1.EqualTo(e2)) {
    return Formula::False();
  }
  return Formula{make_shared<FormulaGt>(e1, e2)};
}
Formula operator>(const double v1, const Expression& e2) {
  return Expression{v1} > e2;
}
Formula operator>(const Expression& e1, const double v2) {
  return e1 > Expression{v2};
}

Formula operator>=(const Expression& e1, const Expression& e2) {
  // simplification: E >= E  -->  True
  if (e1.EqualTo(e2)) {
    return Formula::True();
  }
  return Formula{make_shared<FormulaGeq>(e1, e2)};
}
Formula operator>=(const double v1, const Expression& e2) {
  return Expression{v1} >= e2;
}
Formula operator>=(const Expression& e1, const double v2) {
  return e1 >= Expression{v2};
}

FormulaCell::FormulaCell(FormulaKind const k, size_t const hash)
    : kind_{k}, hash_{hash_combine(static_cast<size_t>(kind_), hash)} {}

FormulaTrue::FormulaTrue()
    : FormulaCell{FormulaKind::True, hash<string>{}("True")} {}

symbolic::Variables FormulaTrue::GetFreeVariables() const {
  return symbolic::Variables{};
}

bool FormulaTrue::EqualTo(const FormulaCell& f) const {
  return get_kind() == f.get_kind();
}

bool FormulaTrue::Evaluate(const Environment& env) const { return true; }

ostream& FormulaTrue::Display(ostream& os) const {
  os << "True";
  return os;
}

FormulaFalse::FormulaFalse()
    : FormulaCell{FormulaKind::False, hash<string>{}("False")} {}

symbolic::Variables FormulaFalse::GetFreeVariables() const {
  return symbolic::Variables{};
}

bool FormulaFalse::EqualTo(const FormulaCell& f) const {
  return get_kind() == f.get_kind();
}

bool FormulaFalse::Evaluate(const Environment& env) const { return false; }

ostream& FormulaFalse::Display(ostream& os) const {
  os << "False";
  return os;
}

FormulaEq::FormulaEq(const Expression& e1, const Expression& e2)
    : FormulaCell{FormulaKind::Eq, hash_combine(e1.get_hash(), e2.get_hash())},
      e1_{e1},
      e2_{e2} {}

symbolic::Variables FormulaEq::GetFreeVariables() const {
  symbolic::Variables ret{e1_.GetVariables()};
  symbolic::Variables const res_from_e2{e2_.GetVariables()};
  ret.insert(res_from_e2.begin(), res_from_e2.end());
  return ret;
}

bool FormulaEq::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaEq& f_eq{static_cast<const FormulaEq&>(f)};
  return e1_.EqualTo(f_eq.e1_) && e2_.EqualTo(f_eq.e2_);
}

bool FormulaEq::Evaluate(const Environment& env) const {
  return e1_.Evaluate(env) == e2_.Evaluate(env);
}

ostream& FormulaEq::Display(ostream& os) const {
  os << "(" << e1_ << " = " << e2_ << ")";
  return os;
}

FormulaNeq::FormulaNeq(const Expression& e1, const Expression& e2)
    : FormulaCell{FormulaKind::Neq, hash_combine(e1.get_hash(), e2.get_hash())},
      e1_{e1},
      e2_{e2} {}

symbolic::Variables FormulaNeq::GetFreeVariables() const {
  symbolic::Variables ret{e1_.GetVariables()};
  symbolic::Variables const res_from_e2{e2_.GetVariables()};
  ret.insert(res_from_e2.begin(), res_from_e2.end());
  return ret;
}

bool FormulaNeq::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaNeq& f_neq{static_cast<const FormulaNeq&>(f)};
  return e1_.EqualTo(f_neq.e1_) && e2_.EqualTo(f_neq.e2_);
}

bool FormulaNeq::Evaluate(const Environment& env) const {
  return e1_.Evaluate(env) != e2_.Evaluate(env);
}

ostream& FormulaNeq::Display(ostream& os) const {
  os << "(" << e1_ << " != " << e2_ << ")";
  return os;
}

FormulaGt::FormulaGt(const Expression& e1, const Expression& e2)
    : FormulaCell{FormulaKind::Gt, hash_combine(e1.get_hash(), e2.get_hash())},
      e1_{e1},
      e2_{e2} {}

symbolic::Variables FormulaGt::GetFreeVariables() const {
  symbolic::Variables ret{e1_.GetVariables()};
  symbolic::Variables const res_from_e2{e2_.GetVariables()};
  ret.insert(res_from_e2.begin(), res_from_e2.end());
  return ret;
}

bool FormulaGt::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaGt& f_gt{static_cast<const FormulaGt&>(f)};
  return e1_.EqualTo(f_gt.e1_) && e2_.EqualTo(f_gt.e2_);
}

bool FormulaGt::Evaluate(const Environment& env) const {
  return e1_.Evaluate(env) > e2_.Evaluate(env);
}

ostream& FormulaGt::Display(ostream& os) const {
  os << "(" << e1_ << " > " << e2_ << ")";
  return os;
}

FormulaGeq::FormulaGeq(const Expression& e1, const Expression& e2)
    : FormulaCell{FormulaKind::Geq, hash_combine(e1.get_hash(), e2.get_hash())},
      e1_{e1},
      e2_{e2} {}

symbolic::Variables FormulaGeq::GetFreeVariables() const {
  symbolic::Variables ret{e1_.GetVariables()};
  symbolic::Variables const res_from_e2{e2_.GetVariables()};
  ret.insert(res_from_e2.begin(), res_from_e2.end());
  return ret;
}

bool FormulaGeq::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaGeq& f_geq{static_cast<const FormulaGeq&>(f)};
  return e1_.EqualTo(f_geq.e1_) && e2_.EqualTo(f_geq.e2_);
}

bool FormulaGeq::Evaluate(const Environment& env) const {
  return e1_.Evaluate(env) >= e2_.Evaluate(env);
}

ostream& FormulaGeq::Display(ostream& os) const {
  os << "(" << e1_ << " >= " << e2_ << ")";
  return os;
}

FormulaLt::FormulaLt(const Expression& e1, const Expression& e2)
    : FormulaCell{FormulaKind::Lt, hash_combine(e1.get_hash(), e2.get_hash())},
      e1_{e1},
      e2_{e2} {}

symbolic::Variables FormulaLt::GetFreeVariables() const {
  symbolic::Variables ret{e1_.GetVariables()};
  symbolic::Variables const res_from_e2{e2_.GetVariables()};
  ret.insert(res_from_e2.begin(), res_from_e2.end());
  return ret;
}

bool FormulaLt::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaLt& f_gt{static_cast<const FormulaLt&>(f)};
  return e1_.EqualTo(f_gt.e1_) && e2_.EqualTo(f_gt.e2_);
}

bool FormulaLt::Evaluate(const Environment& env) const {
  return e1_.Evaluate(env) < e2_.Evaluate(env);
}

ostream& FormulaLt::Display(ostream& os) const {
  os << "(" << e1_ << " < " << e2_ << ")";
  return os;
}

FormulaLeq::FormulaLeq(const Expression& e1, const Expression& e2)
    : FormulaCell{FormulaKind::Leq, hash_combine(e1.get_hash(), e2.get_hash())},
      e1_{e1},
      e2_{e2} {}

symbolic::Variables FormulaLeq::GetFreeVariables() const {
  symbolic::Variables ret{e1_.GetVariables()};
  symbolic::Variables const res_from_e2{e2_.GetVariables()};
  ret.insert(res_from_e2.begin(), res_from_e2.end());
  return ret;
}

bool FormulaLeq::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaLeq& f_geq{static_cast<const FormulaLeq&>(f)};
  return e1_.EqualTo(f_geq.e1_) && e2_.EqualTo(f_geq.e2_);
}

bool FormulaLeq::Evaluate(const Environment& env) const {
  return e1_.Evaluate(env) <= e2_.Evaluate(env);
}

ostream& FormulaLeq::Display(ostream& os) const {
  os << "(" << e1_ << " <= " << e2_ << ")";
  return os;
}

FormulaAnd::FormulaAnd(const Formula& f1, const Formula& f2)
    : FormulaCell{FormulaKind::And, hash_combine(f1.get_hash(), f2.get_hash())},
      f1_{f1},
      f2_{f2} {}

symbolic::Variables FormulaAnd::GetFreeVariables() const {
  symbolic::Variables ret{f1_.GetFreeVariables()};
  symbolic::Variables const res_from_f2{f2_.GetFreeVariables()};
  ret.insert(res_from_f2.begin(), res_from_f2.end());
  return ret;
}

bool FormulaAnd::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaAnd& f_and{static_cast<const FormulaAnd&>(f)};
  return f1_.EqualTo(f_and.f1_) && f2_.EqualTo(f_and.f2_);
}

bool FormulaAnd::Evaluate(const Environment& env) const {
  return f1_.Evaluate(env) && f2_.Evaluate(env);
}

ostream& FormulaAnd::Display(ostream& os) const {
  os << "(" << f1_ << " and " << f2_ << ")";
  return os;
}

FormulaOr::FormulaOr(const Formula& f1, const Formula& f2)
    : FormulaCell{FormulaKind::Or, hash_combine(f1.get_hash(), f2.get_hash())},
      f1_{f1},
      f2_{f2} {}

symbolic::Variables FormulaOr::GetFreeVariables() const {
  symbolic::Variables ret{f1_.GetFreeVariables()};
  symbolic::Variables const res_from_f2{f2_.GetFreeVariables()};
  ret.insert(res_from_f2.begin(), res_from_f2.end());
  return ret;
}

bool FormulaOr::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaOr& f_or{static_cast<const FormulaOr&>(f)};
  return f1_.EqualTo(f_or.f1_) && f2_.EqualTo(f_or.f2_);
}

bool FormulaOr::Evaluate(const Environment& env) const {
  return f1_.Evaluate(env) || f2_.Evaluate(env);
}

ostream& FormulaOr::Display(ostream& os) const {
  os << "(" << f1_ << " or " << f2_ << ")";
  return os;
}

FormulaNot::FormulaNot(const Formula& f)
    : FormulaCell{FormulaKind::Not, f.get_hash()}, f_{f} {}

symbolic::Variables FormulaNot::GetFreeVariables() const {
  return f_.GetFreeVariables();
}

bool FormulaNot::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaNot& f_not{static_cast<const FormulaNot&>(f)};
  return f_.EqualTo(f_not.f_);
}

bool FormulaNot::Evaluate(const Environment& env) const {
  return !f_.Evaluate(env);
}

ostream& FormulaNot::Display(ostream& os) const {
  os << "!(" << f_ << ")";
  return os;
}

FormulaForall::FormulaForall(const Variables& vars, const Formula& f)
    : FormulaCell{FormulaKind::Forall,
                  hash_combine(vars.get_hash(), f.get_hash())},
      vars_{vars},
      f_{f} {}

symbolic::Variables FormulaForall::GetFreeVariables() const {
  return f_.GetFreeVariables() - vars_;
}

bool FormulaForall::EqualTo(const FormulaCell& f) const {
  if (get_kind() != f.get_kind()) {
    return false;
  }
  const FormulaForall& f_forall{static_cast<const FormulaForall&>(f)};
  return vars_ == f_forall.vars_ && f_.EqualTo(f_forall.f_);
}

bool FormulaForall::Evaluate(const Environment& env) const {
  // Given ∀ x1, ..., xn. F, check if there is a counterexample satisfying
  // ¬F. If exists, it returns false. Otherwise, return true.
  // That is, it returns !check(∃ x1, ..., xn. ¬F)

  throw runtime_error("not implemented yet");
}

ostream& FormulaForall::Display(ostream& os) const {
  os << "forall(" << vars_ << ". " << f_ << ")";
  return os;
}
}  // namespace symbolic
}  // namespace drake
