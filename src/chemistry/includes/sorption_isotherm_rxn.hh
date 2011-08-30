/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
#ifndef AMANZI_CHEMISTRY_SORPTION_ISOTHERM_RXN_HH_
#define AMANZI_CHEMISTRY_SORPTION_ISOTHERM_RXN_HH_

// Base class for sorption isotherm (linear, Langmuir, Freundlich) reactions

#include <string>
#include <vector>

#include<species.hh>
#include<sorption_isotherm.hh>

namespace amanzi {
namespace chemistry {

// forward declarations from chemistry
class Block;

class SorptionIsothermRxn {
 public:
  SorptionIsothermRxn();
  SorptionIsothermRxn(const SpeciesName species_name, 
                      const SpeciesId species_id,
                      SorptionIsotherm *isotherm);
  ~SorptionIsothermRxn();

  void Update(const std::vector<Species>& primarySpecies);
  // add sorbed concentration to sorbed total
  void AddContributionToTotal(std::vector<double> *total);
  // add sorbed concentration to sorbed total
  void AddContributionToDTotal(const std::vector<Species>& primarySpecies,
                               Block* dtotal);
  void Display(void) const;

 protected:

 private:

  SpeciesId species_id_; // ID of primary species
  SpeciesName species_name_; // Name of primary species
  double sorbed_concentration_;
  SorptionIsotherm *isotherm_;

}; // SorptionIsothermRxn

}  // namespace chemistry
}  // namespace amanzi
#endif  // AMANZI_CHEMISTRY_SORPTION_ISOTHERM_RXN_HH_
