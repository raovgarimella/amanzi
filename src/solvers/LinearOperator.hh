/*
  Solvers

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
           Konstantin Lipnikov (lipnikov@lanl.gov)
*/

//!  Base class for linear solvers.

/*!

``[linear-solver-typed-spec]``

* `"iterative method type`" ``[string]`` Iterative method to be used.
* `"_iterative_method_type_ parameters`" ``[_iterative_method_type_-spec]``
  Parameters associated with the requested iterative method.
  
 */


#ifndef AMANZI_LINEAR_OPERATOR_HH_
#define AMANZI_LINEAR_OPERATOR_HH_

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "dbc.hh"
#include "errors.hh"

#include "LinearOperatorDefs.hh"

namespace Amanzi {
namespace AmanziSolvers {

template<class Matrix, class Vector, class VectorSpace>
class LinearOperator : public Matrix {
 public:
  LinearOperator(const Teuchos::RCP<const Matrix>& m,
                 const Teuchos::RCP<const Matrix>& h) :
      Matrix(),
      m_(m),
      h_(h)
  {};

  virtual ~LinearOperator() {};

  virtual void Init(Teuchos::ParameterList& plist) = 0;
  void Init() { 
    Teuchos::ParameterList plist;
    Init(plist);
  }

  // standard interface recommended in MatrixBase.hh
  virtual int Apply(const Vector& v, Vector& mv) const { return m_->Apply(v, mv); }
  virtual int ApplyInverse(const Vector& v, Vector& hv) const = 0;

  virtual const VectorSpace& DomainMap() const { return m_->DomainMap(); }
  virtual const VectorSpace& RangeMap() const { return m_->RangeMap(); }

  double TrueResidual(const Vector& f, const Vector& v) const {
    Vector r(f);
    m_->Apply(v, r);  // r = f - M * x
    r.Update(1.0, f, -1.0);

    double true_residual;
    r.Norm2(&true_residual);
    return true_residual;
  }

  // control and statistics
  virtual double residual() = 0;
  virtual int num_itrs() = 0;
  virtual void add_criteria(int criteria) = 0;
  virtual int returned_code() = 0;

  std::string name() { return name_; }
  void set_name(std::string name) { name_ = name; }

  // to recuperate from a crash, we post-process errors here
  Errors::Message DecodeErrorCode(int ierr) {
    Errors::Message msg;
    switch(ierr) {
    case AmanziSolvers::LIN_SOLVER_NON_SPD_APPLY:
         msg << "Linear system is not SPD.\n";
    case AmanziSolvers::LIN_SOLVER_NON_SPD_APPLY_INVERSE:
         msg << "Linear system is not SPD.\n";
    case AmanziSolvers::LIN_SOLVER_MAX_ITERATIONS:
         msg << "Maximum iterations are reached in solution of linear system.\n";
    case AmanziSolvers::LIN_SOLVER_RESIDUAL_OVERFLOW:
         msg << "Residual overflow in solution of linear system.\n";
    default:
         msg << "\nLinear solver returned an unrecoverable error code: " << ierr << ".\n";
    }
    return msg;
  }

 protected:
  Teuchos::RCP<const Matrix> m_;
  Teuchos::RCP<const Matrix> h_;
  std::string name_;

 private:
  LinearOperator();
  LinearOperator(const LinearOperator<Matrix,Vector,VectorSpace>& other); // specifically not implemented
};

}  // namespace AmanziSolvers
}  // namespace Amanzi
 
#endif
               

