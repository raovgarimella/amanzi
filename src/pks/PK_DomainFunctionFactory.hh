/*
  Process Kernels

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov
*/

#ifndef AMANZI_PK_DOMAIN_FUNCTION_FACTORY_HH_
#define AMANZI_PK_DOMAIN_FUNCTION_FACTORY_HH_

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "Mesh.hh"
#include "PK_DomainFunctionSimple.hh"
#include "PK_DomainFunctionVolume.hh"
#include "PK_DomainFunctionVolumeFraction.hh"
#include "PK_DomainFunctionWeight.hh"
#include "PK_DomainFunctionCoupling.hh"

namespace Amanzi {

template <class FunctionBase>
class PK_DomainFunctionFactory : public FunctionBase {
 public:
  PK_DomainFunctionFactory(const Teuchos::RCP<const AmanziMesh::Mesh>& mesh)
      : mesh_(mesh) {};
  ~PK_DomainFunctionFactory() {};

  Teuchos::RCP<FunctionBase> Create(const Teuchos::ParameterList& plist,
                                    const std::string& keyword,
                                    AmanziMesh::Entity_kind kind,
                                    Teuchos::RCP<const Epetra_Vector> weight);

 protected:
  Teuchos::RCP<const AmanziMesh::Mesh> mesh_;
};


template <class FunctionBase>
Teuchos::RCP<FunctionBase> PK_DomainFunctionFactory<FunctionBase>::Create(
    const Teuchos::ParameterList& plist,
    const std::string& keyword,
    AmanziMesh::Entity_kind kind,
    Teuchos::RCP<const Epetra_Vector> weight)
{
  // verify completeness of the list
  Errors::Message msg;

  if (!plist.isParameter("regions")) {
    msg << "required parameter \"regions\" is missing";
    Exceptions::amanzi_throw(msg);
  }

  if (!plist.isSublist(keyword)) {
    msg << "required parameter \"" << keyword << "\" is not a sublist";
    Exceptions::amanzi_throw(msg);
  }

  // select model for the source function
  std::string model("none");
  if (plist.isParameter("spatial distribution method")) {
    model = plist.get<std::string>("spatial distribution method");
  }

  if (model == "volume") {
    Teuchos::RCP<PK_DomainFunctionVolume<FunctionBase> >
        func = Teuchos::rcp(new PK_DomainFunctionVolume<FunctionBase>(mesh_, kind));
    func->Init(plist, keyword);
    return func; 
  }
  else if (model == "permeability") {
    Teuchos::RCP<PK_DomainFunctionWeight<FunctionBase> >
        func = Teuchos::rcp(new PK_DomainFunctionWeight<FunctionBase>(mesh_));
    func->Init(plist, keyword, weight);
    return func; 
  }
  else if (model == "volume fraction") {
    Teuchos::RCP<PK_DomainFunctionVolumeFraction<FunctionBase> >
      func = Teuchos::rcp(new PK_DomainFunctionVolumeFraction<FunctionBase>(mesh_));
    func->Init(plist, keyword);
    return func; 
  }
  else if (model == "domain coupling") {
    Teuchos::RCP<PK_DomainFunctionCoupling<FunctionBase> >
      func = Teuchos::rcp(new PK_DomainFunctionCoupling<FunctionBase>(mesh_));
    func->Init(plist, keyword, kind);
    return func;
  }
  else {
    Teuchos::RCP<PK_DomainFunctionSimple<FunctionBase> >
        func = Teuchos::rcp(new PK_DomainFunctionSimple<FunctionBase>(mesh_, plist, kind));
    func->Init(plist, keyword);
    return func; 
  }

  return Teuchos::null;
}

}  // namespace Amanzi

#endif

