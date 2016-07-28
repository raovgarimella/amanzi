/*
  Transport PK 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Neil Carlson
           Ethan Coon
           Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#ifndef AMANZI_TRANSPORT_BOUNDARY_FUNCTION_HH_
#define AMANZI_TRANSPORT_BOUNDARY_FUNCTION_HH_

#include <vector>
#include <map>
#include <string>

#include "Epetra_MultiVector.h"
#include "Teuchos_RCP.hpp"

#include "CommonDefs.hh"
#include "Mesh.hh"
#include "DenseVector.hh"

namespace Amanzi {
namespace Transport {

class TransportBoundaryFunction {
 public:
  TransportBoundaryFunction()
    : domain_volume_(-1.0),
      mass_ratio_(false) {};

  TransportBoundaryFunction(const Teuchos::ParameterList& plist);
  ~TransportBoundaryFunction() {};

  // source term on time interval (t0, t1]
  virtual void Compute(double t0, double t1) { ASSERT(false); }
  void ComputeSubmodel(const Teuchos::RCP<const AmanziMesh::Mesh>& mesh,
                       Teuchos::RCP<const Epetra_MultiVector>& field);

  // model name
  virtual std::string name() { return "undefined"; } 

  // access
  double domain_volume() { return domain_volume_; }

  std::vector<std::string>& tcc_names() { return tcc_names_; }
  std::vector<int>& tcc_index() { return tcc_index_; }

  // iterator methods
  typedef std::map<int, WhetStone::DenseVector>::iterator Iterator;
  Iterator begin() { return value_.begin(); }
  Iterator end() { return value_.end(); }
  std::map<int, double>::size_type size() { return value_.size(); }

 protected:
  double domain_volume_;
  std::map<int, WhetStone::DenseVector> value_;  // tcc values on boundary faces

  std::vector<std::string> tcc_names_;  // list of component names
  std::vector<int> tcc_index_;  // index of component in the global list

  // submodels
  bool mass_ratio_;
  double molar_mass_;
};

}  // namespace Transport
}  // namespace Amanzi

#endif
