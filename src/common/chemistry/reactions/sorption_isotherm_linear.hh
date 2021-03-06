/*
  Chemistry 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Class for linear isotherm
*/

#ifndef AMANZI_CHEMISTRY_SORPTION_ISOTHERM_LINEAR_HH_
#define AMANZI_CHEMISTRY_SORPTION_ISOTHERM_LINEAR_HH_

#include "sorption_isotherm.hh"

namespace Amanzi {
namespace AmanziChemistry {

class SorptionIsothermLinear : public SorptionIsotherm {
 public:
  SorptionIsothermLinear();
  SorptionIsothermLinear(const double KD);
  ~SorptionIsothermLinear() {};

  void Init(const double KD);
  // returns sorbed concentration
  double Evaluate(const Species& primarySpecies);
  double EvaluateDerivative(const Species& primarySpecies);
  void Display(const Teuchos::RCP<VerboseObject>& vo) const;

  double KD(void) const { return KD_; }
  void set_KD(const double KD) { KD_ = KD; }

  const std::vector<double>& GetParameters(void);
  void SetParameters(const std::vector<double>& params);

 private:
  // distribution coefficient
  // (currently) units = kg water/m^3 bulk
  double KD_; 
  std::vector<double> params_;
};

}  // namespace AmanziChemistry
}  // namespace Amanzi
#endif
