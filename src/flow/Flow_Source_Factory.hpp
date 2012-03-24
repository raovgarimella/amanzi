#ifndef __FLOW_SOURCE_FACTORY_HPP__
#define __FLOW_SOURCE_FACTORY_HPP__

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "Point.hh"
#include "Mesh.hh"
#include "domain_function.hh"

namespace Amanzi {
namespace AmanziFlow {

class FlowSourceFactory {
 public:
  FlowSourceFactory(const Teuchos::RCP<const AmanziMesh::Mesh> mesh,
                    const Teuchos::RCP<Teuchos::ParameterList> params)
     : mesh_(mesh), params_(params) {};
  ~FlowSourceFactory() {};
  
  DomainFunction* createSource() const;

 private:
  void processSourceSpec(Teuchos::ParameterList& list, DomainFunction* src) const;
     
 private:
  const Teuchos::RCP<const AmanziMesh::Mesh> mesh_;
  const Teuchos::RCP<Teuchos::ParameterList> params_;
};

}  // namespace AmanziFlow
}  // namespace Amanzi

#endif // AMANZI_FLOW_SOURCE_FACTORY_HH_
