/*
  Operators

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon (ecoon@lanl.gov)
*/

#include <vector>

// TPLs
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

// Amanzi
#include "GraphFE.hh"
#include "MatrixFE.hh"
#include "PreconditionerFactory.hh"
#include "SuperMap.hh"
#include "VerboseObject.hh"

// Operators
#include "Operator.hh"
#include "OperatorUtils.hh"
#include "TreeOperator.hh"

namespace Amanzi {
namespace Operators {

/* ******************************************************************
* Constructor from a tree vector.
****************************************************************** */
TreeOperator::TreeOperator(Teuchos::RCP<const TreeVectorSpace> tvs) :
    tvs_(tvs),
    block_diagonal_(false)
{
  // make sure we have the right kind of TreeVectorSpace -- it should be
  // one parent node with all leaf node children.
  AMANZI_ASSERT(tvs_->Data() == Teuchos::null);
  for (TreeVectorSpace::const_iterator it = tvs_->begin(); it != tvs_->end(); ++it) {
    AMANZI_ASSERT((*it)->Data() != Teuchos::null);
  }

  // resize the blocks
  int n_blocks = tvs_->size();
  blocks_.resize(n_blocks, Teuchos::Array<Teuchos::RCP<const Operator> >(n_blocks, Teuchos::null));
  transpose_.resize(n_blocks, Teuchos::Array<bool>(n_blocks, false));
}


/* ******************************************************************
* Populate block matrix with pointers to operators.
****************************************************************** */
void TreeOperator::SetOperatorBlock(int i, int j, const Teuchos::RCP<const Operator>& op, bool transpose) {
  blocks_[i][j] = op;
  transpose_[i][j] = transpose;
}


/* ******************************************************************
* Calculate Y = A * X using matrix-free matvec on blocks of operators.
****************************************************************** */
int TreeOperator::Apply(const TreeVector& X, TreeVector& Y) const
{
  Y.PutScalar(0.0);

  int ierr(0), n(0);
  for (TreeVector::iterator yN_tv = Y.begin(); yN_tv != Y.end(); ++yN_tv, ++n) {
    CompositeVector& yN = *(*yN_tv)->Data();
    int m(0);
    for (TreeVector::const_iterator xM_tv = X.begin(); xM_tv != X.end(); ++xM_tv, ++m) {
      if (blocks_[n][m] != Teuchos::null) {
        if (transpose_[n][m]) {
          ierr |= blocks_[n][m]->ApplyTranspose(*(*xM_tv)->Data(), yN, 1.0);
        } else {
          ierr |= blocks_[n][m]->Apply(*(*xM_tv)->Data(), yN, 1.0);
        }
      }
    }
  }
  return ierr;
}


/* ******************************************************************
* Calculate Y = A * X using matrix-free matvec on blocks of operators.
****************************************************************** */
int TreeOperator::ApplyAssembled(const TreeVector& X, TreeVector& Y) const
{
  Y.PutScalar(0.0);
  Epetra_Vector Xcopy(A_->RowMap());
  Epetra_Vector Ycopy(A_->RowMap());

  int ierr = CopyTreeVectorToSuperVector(*smap_, X, Xcopy);
  ierr |= A_->Apply(Xcopy, Ycopy);
  ierr |= CopySuperVectorToTreeVector(*smap_, Ycopy, Y);
  AMANZI_ASSERT(!ierr);

  return ierr;
}


/* ******************************************************************
* Calculate Y = inv(A) * X using global matrix.
****************************************************************** */
int TreeOperator::ApplyInverse(const TreeVector& X, TreeVector& Y) const
{
  int code(0);
  if (!block_diagonal_) {
    Epetra_Vector Xcopy(A_->RowMap());
    Epetra_Vector Ycopy(A_->RowMap());

    int ierr = CopyTreeVectorToSuperVector(*smap_, X, Xcopy);
    code = preconditioner_->ApplyInverse(Xcopy, Ycopy);
    ierr |= CopySuperVectorToTreeVector(*smap_, Ycopy, Y);

    AMANZI_ASSERT(!ierr);
  } else {
    for (int n = 0; n < tvs_->size(); ++n) {
      const CompositeVector& Xn = *X.SubVector(n)->Data();
      CompositeVector& Yn = *Y.SubVector(n)->Data();
      code |= blocks_[n][n]->ApplyInverse(Xn, Yn);
    }
  }

  return code;
}

    
/* ******************************************************************
* Sumbolic assemble global matrix from elemental matrices of block 
* operators. The algorithm is limited to the case the all blocks are
* square matrices.
****************************************************************** */
void TreeOperator::SymbolicAssembleMatrix()
{
  int n_blocks = blocks_.size();

  // Currently we assume all diagonal schema are the same and well defined.
  // May be ways to relax this a bit in the future, but it currently covers
  // all uses.
  int schema = 0;

  // Check that each row has at least one non-null operator block
  Teuchos::RCP<const Operator> an_op;
  for (int lcv_row = 0; lcv_row != n_blocks; ++lcv_row) {
    bool is_block(false);
    for (int lcv_col = 0; lcv_col != n_blocks; ++lcv_col) {
      if (blocks_[lcv_row][lcv_col] != Teuchos::null) {
        an_op = blocks_[lcv_row][lcv_col];
        is_block = true;
      }

      if (lcv_row == lcv_col) {
        AMANZI_ASSERT(blocks_[lcv_row][lcv_col] != Teuchos::null);
        if (schema == 0) {
          schema = blocks_[lcv_row][lcv_col]->schema();
        } else {
          AMANZI_ASSERT(schema == blocks_[lcv_row][lcv_col]->schema());
        }
      }
    }
    AMANZI_ASSERT(is_block);
  }

  // create the supermap and graph
  smap_ = CreateSuperMap(an_op->DomainMap(), schema, n_blocks);
  int row_size = MaxRowSize(*an_op->DomainMap().Mesh(), schema, n_blocks);
  Teuchos::RCP<GraphFE> graph = Teuchos::rcp(new GraphFE(smap_->Map(),
          smap_->GhostedMap(), smap_->GhostedMap(), row_size));

  // fill the graph
  for (int lcv_row = 0; lcv_row != n_blocks; ++lcv_row) {
    for (int lcv_col = 0; lcv_col != n_blocks; ++lcv_col) {
      Teuchos::RCP<const Operator> block = blocks_[lcv_row][lcv_col];
      if (block != Teuchos::null) {
        block->SymbolicAssembleMatrix(*smap_, *graph, lcv_row, lcv_col);
      }
    }
  }

  // assemble the graph
  int ierr = graph->FillComplete(smap_->Map(), smap_->Map());
  AMANZI_ASSERT(!ierr);

  // create the matrix
  Amat_ = Teuchos::rcp(new MatrixFE(graph));
  A_ = Amat_->Matrix();
}


/* ******************************************************************
* Assemble global matrix from elemental matrices of block operators.
****************************************************************** */
void TreeOperator::AssembleMatrix() {
  int n_blocks = blocks_.size();
  Amat_->Zero();

  // check that each row has at least one non-null operator block
  for (int lcv_row = 0; lcv_row != n_blocks; ++lcv_row) {
    for (int lcv_col = 0; lcv_col != n_blocks; ++lcv_col) {
      Teuchos::RCP<const Operator> block = blocks_[lcv_row][lcv_col];
      if (block != Teuchos::null) {
        block->AssembleMatrix(*smap_, *Amat_, lcv_row, lcv_col);
      }
    }
  }

  int ierr = Amat_->FillComplete();
  AMANZI_ASSERT(!ierr);
}


/* ******************************************************************
* Create preconditioner using name and a factory.
****************************************************************** */
void TreeOperator::InitPreconditioner(
    const std::string& prec_name, const Teuchos::ParameterList& plist)
{
  AmanziPreconditioners::PreconditionerFactory factory;
  preconditioner_ = factory.Create(prec_name, plist);
  preconditioner_->Update(A_);
}


/* ******************************************************************
* Create preconditioner using name and a factory.
****************************************************************** */
void TreeOperator::InitPreconditioner(
    Teuchos::ParameterList& plist)
{
  AmanziPreconditioners::PreconditionerFactory factory;
  preconditioner_ = factory.Create(plist);
  preconditioner_->Update(A_);
}


/* ******************************************************************
* Two-stage initialization of preconditioner, part 1.
* Create the PC and set options.  SymbolicAssemble() must have been called.
****************************************************************** */
void TreeOperator::InitializePreconditioner(Teuchos::ParameterList& plist)
{
  AMANZI_ASSERT(A_.get());
  AMANZI_ASSERT(smap_.get());

  // provide block ids for block strategies.
  if (plist.isParameter("preconditioner type") &&
      plist.get<std::string>("preconditioner type") == "boomer amg" &&
      plist.isSublist("boomer amg parameters")) {

    // NOTE: Hypre frees this
    auto block_ids = smap_->BlockIndices();

    plist.sublist("boomer amg parameters").set("number of unique block indices", block_ids.first);

    // Note, this passes a raw pointer through a ParameterList.  I was surprised
    // this worked too, but ParameterList is a boost::any at heart... --etc
    plist.sublist("boomer amg parameters").set("block indices", block_ids.second);
  }

  AmanziPreconditioners::PreconditionerFactory factory;
  preconditioner_ = factory.Create(plist);
}


/* ******************************************************************
* Two-stage initialization of preconditioner, part 2.
* Set the matrix in the preconditioner.  Assemble() must have been called.
****************************************************************** */
void TreeOperator::UpdatePreconditioner()
{
  AMANZI_ASSERT(preconditioner_.get());
  AMANZI_ASSERT(A_.get());
  preconditioner_->Update(A_);
}


/* ******************************************************************
* Init block-diagonal preconditioner
****************************************************************** */
void TreeOperator::InitBlockDiagonalPreconditioner()
{
  block_diagonal_ = true;
}


/* ******************************************************************
* Deep copy for building interfaces to TPLs, mainly to solvers.
* We assume that domain = range, which is natural for solvers.
****************************************************************** */
void TreeOperator::CopyVectorToSuperVector(const TreeVector& tv, Epetra_Vector& sv) const
{
  int ierr(0);
  int my_dof = 0;
  for (auto it = tv.begin(); it != tv.end(); ++it) {
    ierr |= CopyCompositeVectorToSuperVector(*smap_, *(*it)->Data(), sv, my_dof);
    my_dof++;            
  }
  AMANZI_ASSERT(!ierr);
}


void TreeOperator::CopySuperVectorToVector(const Epetra_Vector& sv, TreeVector& tv) const
{
  int ierr(0);
  int my_dof = 0;
  for (auto it = tv.begin(); it != tv.end(); ++it) {
    ierr |= CopySuperVectorToCompositeVector(*smap_, sv, *(*it)->Data(), my_dof);
    my_dof++;            
  }
  AMANZI_ASSERT(!ierr);
}

}  // namespace Operators
}  // namespace Amanzi


