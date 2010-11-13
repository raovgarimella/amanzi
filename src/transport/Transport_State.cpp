
#include "State.hpp"
#include "cell_geometry.hh"
#include "Transport_State.hpp"


using namespace Teuchos;
using namespace cell_geometry;


/* ************************************************************* */
/* at the moment a transport state is a copy of the global state */  
/* ************************************************************* */
Transport_State::Transport_State( State S )
{
  total_component_concentration = S.get_total_component_concentration();
  porosity                      = S.get_porosity();
  darcy_flux                    = S.get_darcy_flux();
  water_saturation              = S.get_water_saturation();
  water_density                 = S.get_water_density();
  mesh_maps                     = S.get_mesh_maps();
}




/* ************************************************************* */
/* trivial (at the moment) copy of a constant transport state    */  
/* ************************************************************* */
void Transport_State::copy_constant_state( Transport_State & S )
{
  total_component_concentration = S.get_total_component_concentration();
  porosity                      = S.get_porosity();
  darcy_flux                    = S.get_darcy_flux();
  water_saturation              = S.get_water_saturation();
  water_density                 = S.get_water_density();
  mesh_maps                     = S.get_mesh_maps();
}




/* ************************************************************* */
/* internal transport state uses internal variable for the total */
/* component concentration                                       */
/* ************************************************************* */
void Transport_State::create_internal_state( Transport_State & S )
{
  porosity         = S.get_porosity(); 
  darcy_flux       = S.get_darcy_flux(); 
  water_saturation = S.get_water_saturation(); 
  water_density    = S.get_water_density();

  RCP<Epetra_MultiVector> tcc = S.get_total_component_concentration();
  total_component_concentration = rcp( new Epetra_MultiVector( *tcc ) );
}




/* ************************************************************* */
/* DEBUG: create constant analytical Darcy velocity fieldx u     */
/* ************************************************************* */
void Transport_State::analytic_darcy_flux( double* u )
{
  int  i, f;
  double x[4][3], normal[3];

  const Epetra_Map & face_map = mesh_maps->face_map( true );

  for( f=face_map.MinLID(); f<=face_map.MaxLID(); f++ ) { 
     mesh_maps->face_to_coordinates( f, (double*) x, (double*) x+12 );

     quad_face_normal(x[0], x[1], x[2], x[3], normal);

     (*darcy_flux)[f] = u[0] * normal[0] + u[1] * normal[1] + u[2] * normal[2];
  }
}




/* ************************************************************* */
/* DEBUG: create constant analytical concentration C_0 = x       */
/* ************************************************************* */
void Transport_State::analytic_total_component_concentration( double t )
{
  int  i, j, c;
  double x[8][3], center[3];

  const Epetra_Map & cell_map = mesh_maps->cell_map( true );

  for( c=cell_map.MinLID(); c<=cell_map.MaxLID(); c++ ) { 
     mesh_maps->cell_to_coordinates( c, (double*) x, (double*) x+24);

     for( i=0; i<3; i++ ) { 
        center[i] = 0;
        for( j=0; j<8; j++ ) center[i] += x[j][i];
        center[i] /= 8;
     }

     if ( center[0] <= t ) (*total_component_concentration)[0][c] = 1;
  }
}




void Transport_State::error_total_component_concentration( double t, vector<double> & cell_volume, double* L1, double* L2 )
{
  int  i, j, c;
  double  d, x[8][3], center[3];

  const Epetra_Map & cell_map = mesh_maps->cell_map( true );

  *L1 = *L2 = 0.0;
  for( c=cell_map.MinLID(); c<=cell_map.MaxLID(); c++ ) { 
     mesh_maps->cell_to_coordinates( c, (double*) x, (double*) x+24);

     for( i=0; i<3; i++ ) { 
        center[i] = 0;
        for( j=0; j<8; j++ ) center[i] += x[j][i];
        center[i] /= 8;
     }

     if ( center[0] <= t ) { d = (*total_component_concentration)[0][c] - 1; }
     else                  { d = (*total_component_concentration)[0][c]; }

     *L1 += fabs( d ) * cell_volume[c];
     *L2 += d * d * cell_volume[c];
  }

  *L2 = sqrt( *L2 );
}




/* ************************************************************* */
/* DEBUG: create constant analytical porosity                    */
/* ************************************************************* */
void Transport_State::analytic_porosity( double phi )
{
  int  c;
  const Epetra_Map & cell_map = mesh_maps->cell_map( true );

  for( c=cell_map.MinLID(); c<=cell_map.MaxLID(); c++ ) { 
     (*porosity)[c] = phi;  /* default is 0.2 */
  }
}




/* ************************************************************* */
/* DEBUG: create constant analytical water saturation            */
/* ************************************************************* */
void Transport_State::analytic_water_saturation( double ws )
{
  int  c;
  const Epetra_Map & cell_map = mesh_maps->cell_map( true );

  for( c=cell_map.MinLID(); c<=cell_map.MaxLID(); c++ ) { 
     (*water_saturation)[c] = ws;  /* default is 1.0 */
  }
}




/* ************************************************************* */
/* DEBUG: create constant analytical water density               */
/* ************************************************************* */
void Transport_State::analytic_water_density( double wd )
{
  int  c;
  const Epetra_Map & cell_map = mesh_maps->cell_map( true );

  for( c=cell_map.MinLID(); c<=cell_map.MaxLID(); c++ ) { 
     (*water_density)[c] = wd;  /* default is 1000.0 */
  }
}
