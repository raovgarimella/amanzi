#ifndef __State_hpp__
#define __State_hpp__

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Epetra_Vector.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Map.h"
#include "Epetra_Export.h"
#include "Mesh.hh"
#include "Vis.hpp"

typedef enum { COMPLETE, UPDATING } status_type;

class State {
  
public:

  State( int, Teuchos::RCP<Amanzi::AmanziMesh::Mesh> );

  State( Teuchos::RCP<Amanzi::AmanziMesh::Mesh> );

  State( Teuchos::ParameterList &parameter_list, 
	 Teuchos::RCP<Amanzi::AmanziMesh::Mesh> );

  ~State();

  // access methods

  Teuchos::RCP<Epetra_Vector>       get_pressure ()         { return pressure; }; 
  Teuchos::RCP<Epetra_Vector>       get_darcy_flux ()       { return darcy_flux; };
  Teuchos::RCP<Epetra_Vector>       get_porosity ()         { return porosity; };
  Teuchos::RCP<Epetra_Vector>       get_water_saturation () { return water_saturation; };
  Teuchos::RCP<Epetra_Vector>       get_water_density ()    { return water_density; };
  Teuchos::RCP<Epetra_Vector>       get_permeability ()     { return permeability; };
  
  Teuchos::RCP<double>              get_density()      { return density; } 
  Teuchos::RCP<double>              get_viscosity()    { return viscosity; }
  Teuchos::RCP<double*>             get_gravity()      { return gravity; }

  Teuchos::RCP<Epetra_MultiVector>  get_darcy_velocity () { return darcy_velocity; }
  Teuchos::RCP<Epetra_MultiVector>  get_total_component_concentration () 
  { return total_component_concentration; };
  
  const Teuchos::RCP<Amanzi::AmanziMesh::Mesh> get_mesh_maps() const { return mesh_maps; };

  const double get_time () const { return time; };
  const int get_cycle () const { return cycle; };

  const int get_number_of_components() const { return number_of_components; };

  const Amanzi::AmanziMesh::Mesh& get_mesh() { return *mesh_maps; };

  // modify methods
  void set_time ( double new_time );
  void set_cycle ( int new_cycle );
  void advance_time(double dT);
  void update_total_component_concentration(Teuchos::RCP<Epetra_MultiVector>);
  void update_total_component_concentration(const Epetra_MultiVector&);
  void update_darcy_flux(const Epetra_Vector&);
  void update_pressure(const Epetra_Vector&);

  // status methods
  const status_type get_status () const { return status; };
  void set_status ( status_type new_status ) { status = new_status; }


  // debug helpers
  void set_darcy_flux( const double* u, const int mesh_block_id );
  void set_water_saturation(const double ws );
  void set_water_density(const double wd );
  void set_zero_total_component_concentration();
  void set_total_component_concentration(const double* conc, const int mesh_block_id); 
  void set_porosity( const double phi );
  void set_porosity( const double phi, const int mesh_block_id );
  void set_permeability (const double kappa);
  void set_permeability (const double kappa, const int mesh_block_id);
  void set_viscosity(const double mu);
  void set_gravity(const double *g);
  void set_number_of_components(const int n);


  // set methods 
  void set_darcy_flux ( const Epetra_Vector& darcy_flux_ );
  void set_water_saturation ( const Epetra_Vector& water_saturation_ );
  void set_water_density ( const Epetra_Vector& water_density_ );
  void set_porosity ( const Epetra_Vector& porosity_ );
  void set_permeability ( const Epetra_Vector& permeability_ );
  void set_pressure ( const Epetra_Vector& pressure_ );
  void set_darcy_velocity ( const Epetra_MultiVector& darcy_velocity_ );
  void set_total_component_concentration ( const Epetra_MultiVector& total_component_concentration_ );

  // observation functions
  double water_mass();
      

  void create_storage();

  void write_vis (Amanzi::Vis& vis);
  void write_vis (Amanzi::Vis& vis, Epetra_MultiVector *auxdata, std::vector<std::string>& auxnames);
  void set_compnames(std::vector<std::string>& compnames_);
 
private:
  void initialize_from_parameter_list();

  void set_cell_value_in_mesh_block(double value, Epetra_Vector &v,
				    int mesh_block_id);

  // state vectors
  Teuchos::RCP<Epetra_Vector> water_density;  
  Teuchos::RCP<Epetra_Vector> pressure;
  Teuchos::RCP<Epetra_Vector> darcy_flux;
  Teuchos::RCP<Epetra_Vector> porosity;
  Teuchos::RCP<Epetra_MultiVector> total_component_concentration; 
  Teuchos::RCP<Epetra_Vector> water_saturation;
  Teuchos::RCP<Epetra_Vector> permeability;
  Teuchos::RCP<Epetra_MultiVector> darcy_velocity;

  Teuchos::RCP<double*> gravity;
  Teuchos::RCP<double> density;
  Teuchos::RCP<double> viscosity;
  
  int number_of_components;

  double time;
  int cycle;
  status_type status;

  // mesh
  const Teuchos::RCP<Amanzi::AmanziMesh::Mesh> mesh_maps;

  // parameter list
  Teuchos::ParameterList parameter_list;

  // names for components
  std::vector<std::string> compnames;

};

#endif
