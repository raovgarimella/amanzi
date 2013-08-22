/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
   ATS

   License: see $ATS_DIR/COPYRIGHT
   Author: Ethan Coon

   Interface for TreeVector, a nested, hierarchical data structure for PK
   hiearchies.  This nested vector allows each physical PK to push back
   Epetra_MultiVectors to store their solution, and allows MPCs to push back
   TreeVectors in a nested format.  It also provides an implementation of the
   Vector interface for use with time integrators/nonlinear solvers.

   Note that a TreeVector may have EITHER subvecs OR data (in the form of a
   CompositeVector), but NOT BOTH!  Nothing HERE precludes this, but it is
   assumed in other places.
   ------------------------------------------------------------------------- */

#ifndef DATA_STRUCTURES_TREEVECTOR_HH_
#define DATA_STRUCTURES_TREEVECTOR_HH_

#include <string>
#include <vector>
#include "Teuchos_RCP.hpp"
#include "Epetra_MultiVector.h"
#include "Epetra_Vector.h"

#include "data_structures_types.hh"
#include "composite_vector.hh"

namespace Amanzi {

  class TreeVector {

  public:
    // Basic constructor of an empty TreeVector
    explicit TreeVector(std::string name);

    // These constructors allocate their own memory and do NOT copy the values,
    // just the layout, of their arguments.

    // these copy constructors are clearly necessary
    TreeVector(const TreeVector& other,
               ConstructMode mode=CONSTRUCT_WITH_NEW_DATA);
    TreeVector(std::string name, const TreeVector& other,
               ConstructMode mode=CONSTRUCT_WITH_NEW_DATA);

    // assigment
    TreeVector& operator=(const TreeVector& other);

    // metadata
    void set_name(std::string name) { name_ = name; }
    std::string name() { return name_; }

    // operations
    // this <- scalar
    int PutScalar(double scalar);

    // n_l <- || this ||_{l}
    int Norm2(double* n2) const;
    int Norm1(double* n1) const;
    int NormInf(double* ninf) const;

    // this <- value*this
    int Scale(double value);

    // this <- this + scalarA
    int Shift(double scalarA);

    // result <- other \dot this
    int Dot(const TreeVector& other, double* result) const;

    // this <- scalarA*A + scalarThis*this
    TreeVector& Update(double scalarA, const TreeVector& A, double scalarThis);

    // this <- scalarA*A + scalarB*B + scalarThis*this
    TreeVector& Update(double scalarA, const TreeVector& A,
                       double scalarB, const TreeVector& B, double scalarThis);

    // this <- scalarAB * A@B + scalarThis*this  (@ is the elementwise product
    int Multiply(double scalarAB, const TreeVector& A, const TreeVector& B,
                 double scalarThis);

    // non-inherited extras
    void Print(ostream &os) const;

    // Iterators over SubVectors
    std::vector< Teuchos::RCP<TreeVector> > SubVectors() { return subvecs_; }
    const std::vector< Teuchos::RCP<TreeVector> > SubVectors() const { return subvecs_; }

    // Get a pointer to the sub-vector "subname".
    Teuchos::RCP<TreeVector> SubVector(std::string subname);
    Teuchos::RCP<const TreeVector> SubVector(std::string subname) const;

    // Get a pointer to the CompositeVector
    Teuchos::RCP<CompositeVector> data() { return data_; }
    Teuchos::RCP<const CompositeVector> data() const { return data_; }

    // Add a sub-vector as a child of this node.
    void PushBack(Teuchos::RCP<TreeVector>& subvec);

    // Set data by pointer
    void set_data(const Teuchos::RCP<CompositeVector>& data) { data_ = data; }

    // Set data by copy
    void set_data(const CompositeVector& data) { *data_ = data; }

  private:
    std::string name_;
    Teuchos::RCP<CompositeVector> data_;
    std::vector< Teuchos::RCP<TreeVector> > subvecs_;
  };

} // namespace


#endif