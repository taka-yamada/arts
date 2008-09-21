/* Copyright (C) 2008
   Patrick Eriksson <Patrick.Eriksson@rss.chalmers.se>
   Stefan Buehler   <sbuehler@ltu.se>
                            
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA. */



/*===========================================================================
  === File description 
  ===========================================================================*/

/*!
  \file   m_atmosphere.cc
  \author Patrick Eriksson <Patrick.Eriksson@rss.chalmers.se>
  \date   2008-09-17

  \brief  Workspace functions associated with the geoid and the surface.

  These functions are listed in the doxygen documentation as entries of the
  file auto_md.h.
*/




/*===========================================================================
  === External declarations
  ===========================================================================*/

#include <cmath>
//#include <stdexcept>
#include "arts.h"
#include "check_input.h"
#include "complex.h"          
#include "physics_funcs.h"
#include "matpackIII.h"
#include "math_funcs.h"
#include "messages.h"
#include "special_interp.h"
#include "absorption.h"
#include "interpolation.h"
#include "fastem.h"

extern const Numeric DEG2RAD;
extern const Numeric RAD2DEG;
extern const Numeric EARTH_RADIUS;

extern const Index GFIELD4_IA_GRID;
extern const Index GFIELD4_F_GRID;
extern const Index GFIELD4_LAT_GRID;
extern const Index GFIELD4_LON_GRID;




/*===========================================================================
  === Help functions (not WSMs)
  ===========================================================================*/

//! surface_specular_los
/*!
    Calculates the LOS for a specular surface reflection.

    \param   los                In/Out: LOS to be modified.
    \param   atmosphere_dim     Input: As the WSV with the same name.

    \author Patrick Eriksson 
    \date   2002-09-22
*/
void surface_specular_los(
              VectorView   los,
        const Index&       atmosphere_dim )
{
  assert( atmosphere_dim >= 1  &&  atmosphere_dim <= 3 );

  if( atmosphere_dim == 1 )
    {
      assert( los.nelem() == 1 );
      assert( los[0] > 90 );      // Otherwise surface refl. not possible
      assert( los[0] <= 180 ); 

      los[0] = 180 - los[0];
    }

  else if( atmosphere_dim == 2 )
    {
      assert( los.nelem() == 1 );
      assert( abs(los[0]) <= 180 ); 

      los[0] = sign( los[0] ) * 180 - los[0];
    }

  else if( atmosphere_dim == 3 )
    {
      assert( los.nelem() == 2 );
      assert( los[0] >= 0 ); 
      assert( los[0] <= 180 ); 
      assert( abs( los[1] ) <= 180 ); 

      // Calculate LOS neglecting any tilt of the surface
      los[0] = 180 - los[0];
      los[1] = los[1];
    }
}



//! surface_specular_R_and_b
/*!
    Sets up the surface reflection matrix and emission vector for
    the case of specular reflection.

    The function handles only one frequency at the time.

    See further the surface chapter in the user guide.

    \param   surface_rmatrix   Out: As the WSV with the same name, but slice
                                    for one direction and one frequency.
    \param   surface_emission  Out: As the WSV with the same name, but slice
                                    for one direction and one frequency.
    \param   Rv                In: Complex amplitude relection coefficient
                                   for vertical polarisation.
    \param   Rh                In: Complex amplitude relection coefficient
                                   for horisontal polarisation.
    \param   f                 In: Frequency (a scalar).
    \param   stokes_dim        In: As the WSV with the same name.
    \param   surface_skin_t    In: As the WSV with the same name.

    \author Patrick Eriksson 
    \date   2004-09-24
*/
void surface_specular_R_and_b(
              MatrixView   surface_rmatrix,
              VectorView   surface_emission,
        const Complex&     Rv,
        const Complex&     Rh,
        const Numeric&     f,
        const Index&       stokes_dim,
        const Numeric&     surface_skin_t )
{
  assert( surface_rmatrix.nrows() == stokes_dim );
  assert( surface_rmatrix.ncols() == stokes_dim );
  assert( surface_emission.nelem() == stokes_dim );

  // Expressions are derived in the surface chapter in the user guide

  const Numeric   rv    = pow( abs(Rv), 2.0 );
  const Numeric   rh    = pow( abs(Rh), 2.0 );
  const Numeric   rmean = ( rv + rh ) / 2;
  const Numeric   B     = planck( f, surface_skin_t );

  surface_rmatrix   = 0.0;
  surface_emission  = 0.0;

  surface_rmatrix(0,0) = rmean;
  surface_emission[0]  = B * ( 1 - rmean );

  if( stokes_dim > 1 )
    {
      const Numeric   rdiff = ( rv - rh ) / 2;

      surface_rmatrix(1,0) = rdiff;
      surface_rmatrix(0,1) = rdiff;
      surface_rmatrix(1,1) = rmean;
      surface_emission[1]  = -B * rdiff;

        if( stokes_dim > 2 )
          {
            const Complex   a     = Rh * conj(Rv);
            const Complex   b     = Rv * conj(Rh);
            const Numeric   c     = real( a + b ) / 2.0;

            surface_rmatrix(2,2) = c;
      
            if( stokes_dim > 3 )
              {
                const Numeric   d     = imag( a - b ) / 2.0;

                surface_rmatrix(3,2) = d;
                surface_rmatrix(2,3) = d;
                surface_rmatrix(3,3) = c;
              }
          }
    }
  /*
  cout << Rv << "\n";
  cout << Rh << "\n";
  cout << surface_rmatrix << "\n";
  cout << surface_emission << "\n";
  */
}





/*===========================================================================
  === The functions (in alphabetical order)
  ===========================================================================*/

/* Workspace method: Doxygen documentation will be auto-generated */
void InterpSurfaceFieldToRteGps(
                 Numeric&   outvalue,
           const Index&     atmosphere_dim,
           const Vector&    lat_grid,
           const Vector&    lon_grid,
           const GridPos&   rte_gp_lat,
           const GridPos&   rte_gp_lon,
           const Matrix&    field)
{
  // Interpolate
  outvalue = interp_atmsurface_by_gp( atmosphere_dim, lat_grid, 
                lon_grid, field, rte_gp_lat, rte_gp_lon );

  out3 << "    Result = " << outvalue << "\n";
}



/* Workspace method: Doxygen documentation will be auto-generated */
void r_geoidSpherical(
        // WS Output:
              Matrix&    r_geoid,
        // WS Input:
        const Index&     atmosphere_dim,
        const Vector&    lat_grid,
        const Vector&    lon_grid,
        const Numeric&   r )
{
  // Check input (use dummy for *p_grid*).
  chk_if_in_range( "atmosphere_dim", atmosphere_dim, 1, 3 );
  chk_atm_grids( atmosphere_dim, Vector(2,2,-1), lat_grid, lon_grid );

  // What radius to use?
  Numeric   r_local = r;
  if( r < 0 )
    { r_local = EARTH_RADIUS; }

  out2 << "  Sets r_geoid to a sphere with a constant radius of " 
       << r_local/1e3 << " km.\n";

  // Effective number of latitudes and longitudes
  Index nlat=1, nlon=1;
  if( atmosphere_dim >= 2 )
    { nlat = lat_grid.nelem(); }
  if( atmosphere_dim >= 3 )
    { nlon = lon_grid.nelem(); }
        
  r_geoid.resize( nlat, nlon );

  r_geoid = r_local;
  
  out3 << "            nrows  : " << r_geoid.nrows() << "\n";
  out3 << "            ncols  : " << r_geoid.ncols() << "\n";
}



/* Workspace method: Doxygen documentation will be auto-generated */
void r_geoidWGS84(
        // WS Output:
              Matrix&    r_geoid,
        // WS Input:
        const Index&     atmosphere_dim,
        const Vector&    lat_grid,
        const Vector&    lon_grid,
        const Numeric&   latitude_1d,
        const Numeric&   azimuth_angle_1d )
{
  // Check input (use dummy for *p_grid*).
  chk_if_in_range( "atmosphere_dim", atmosphere_dim, 1, 3 );
  chk_atm_grids( atmosphere_dim, Vector(2,2,-1), lat_grid, lon_grid );

  // Radii of the reference ellipsiod and the values squared, where double
  // is used to avoid numerical problems.
  const Numeric rq  = 6378.138e3, rp  = 6356.752e3;
  const double  rq2 = rq*rq,      rp2 = rp*rp;

  // For 1D the geoid is set to curvature radius for the point and direction
  // given by latitude_1d and azimuth_angle_1d.
  if( atmosphere_dim == 1 )
    {
      chk_if_in_range( "latitude_1d", latitude_1d, -90., 90. );
      chk_if_in_range( "azimuth_angle_1d", azimuth_angle_1d, -180., 180. );

      out2 << "  Sets r_geoid to the curvature radius of the WGS-84 "
           << "reference ellipsiod.\n";

      r_geoid.resize(1,1);

      // Cosine and sine of the latitude. The values are only used squared.
      double cv = cos( latitude_1d * DEG2RAD ); 
             cv = cv * cv; 
      double sv = sin( latitude_1d * DEG2RAD );
             sv = sv * sv; 

      // Calculate NS and EW radius
      Numeric rns = rq2*rp2/pow(rq2*cv+rp2*sv,1.5);
      Numeric rew = rq2/sqrt(rq2*cv+rp2*sv);

      // Calculate the curvature radius in the observation direction
      cv = cos( azimuth_angle_1d * DEG2RAD );
      sv = sin( azimuth_angle_1d * DEG2RAD );
      r_geoid(0,0) = 1/(cv*cv/rns+sv*sv/rew);
    }

  else
    {
      out2 << "  Sets r_geoid to the radius of the WGS-84 "
           << "reference ellipsiod.\n";

      // Number of latitudes
      const Index nlat = lat_grid.nelem();

      // Effective number of longitudes
      Index nlon;
      if( atmosphere_dim == 2 )
        nlon = 1;
      else
        nlon = lon_grid.nelem();

      r_geoid.resize( nlat, nlon );

      Numeric sv, cv, rv;
      for( Index lat=0; lat<nlat; lat++ )
        {
          // Check that the latutide is inside [-90,90]
          if( ( lat_grid[lat] < -90 ) || ( lat_grid[lat] > 90 ) )
            {
              ostringstream os;
              os << "The accepted range for latitudes in this function is "
                 << "[-90,90],\nbut a value of " << lat_grid[lat] 
                 << " was found.";
              throw runtime_error( os.str() );
            }       

          // Cosine and sine of the latitude.
          cv = cos( lat_grid[lat] * DEG2RAD ); 
          sv = sin( lat_grid[lat] * DEG2RAD );
      
          // The radius of the ellipsiod
          rv = sqrt( rq2*rp2 / ( rq2*sv*sv + rp2*cv*cv ) );
      
          // Loop longitudes and fill *r_geoid*.
          for( Index lon=0; lon<nlon; lon++ )
            r_geoid(lat,lon) = rv;
        }
    }
  out3 << "            nrows  : " << r_geoid.nrows() << "\n";
  out3 << "            ncols  : " << r_geoid.ncols() << "\n";
}



/* Workspace method: Doxygen documentation will be auto-generated */
void surfaceBlackbody(
              Matrix&    surface_los,
              Tensor4&   surface_rmatrix,
              Matrix&    surface_emission,
        const Vector&    f_grid,
        const Index&     stokes_dim,
        const Numeric&   surface_skin_t )
{
  chk_if_in_range( "stokes_dim", stokes_dim, 1, 4 );
  chk_if_over_0( "surface_skin_t", surface_skin_t );

  out2 << "  Sets variables to model a blackbody surface with a temperature "
       << " of " << surface_skin_t << " K.\n";
  surface_los.resize(0,0);
  surface_rmatrix.resize(0,0,0,0);

  const Index   nf = f_grid.nelem();
  surface_emission.resize( nf, stokes_dim );
  surface_emission = 0.0;
  for( Index iv=0; iv<nf; iv++ )
    { 
      surface_emission(iv,0) = planck( f_grid[iv], surface_skin_t );
    }
}



/* Workspace method: Doxygen documentation will be auto-generated */
void surfaceFlatRefractiveIndex(
              Matrix&    surface_los,
              Tensor4&   surface_rmatrix,
              Matrix&    surface_emission,
        const Vector&    f_grid,
        const Index&     stokes_dim,
        const Index&     atmosphere_dim,
        const Vector&    rte_los,
        const Numeric&   surface_skin_t,
        const Matrix&    complex_n )
{
  const Index   nf = f_grid.nelem();

  chk_if_in_range( "atmosphere_dim", atmosphere_dim, 1, 3 );
  chk_if_in_range( "stokes_dim", stokes_dim, 1, 4 );
  chk_if_over_0( "surface_skin_t", surface_skin_t );

  chk_matrix_ncols( "complex_n", complex_n, 2 ); 
  //
  if( complex_n.nrows() != nf )
    {
      ostringstream os;
      os << "The number of rows in *complex_n* should match length of *f_grid*."
         << "\n length of *f_grid*  : " << nf 
         << "\n rows in *complex_n* : " << complex_n.nrows() << "\n";
      throw runtime_error( os.str() );
    }

  out2 << "  Sets variables to model a flat surface\n";
  out3 << "     surface temperature: " << surface_skin_t << " K.\n";


  surface_los.resize( 1, rte_los.nelem() );
  surface_los(0,joker) = rte_los;
  surface_specular_los( surface_los(0,joker), atmosphere_dim );

  surface_emission.resize( nf, stokes_dim );
  surface_rmatrix.resize( 1, nf, stokes_dim, stokes_dim );

  // Complex (amplitude) reflection coefficients
  Complex  Rv, Rh;

  for( Index iv=0; iv<nf; iv++ )
    { 
      // Set n2 (refractive index of surface medium)
      Complex n2( complex_n(iv,0), complex_n(iv,1) );

      // Amplitude reflection coefficients
      //
      fresnel( Rv, Rh, Numeric(1.0), n2, 180.0-abs(rte_los[0]) );

      // Fill reflection matrix and emission vector
      surface_specular_R_and_b( surface_rmatrix(0,iv,joker,joker), 
                                surface_emission(iv,joker), Rv, Rh, 
                                f_grid[iv], stokes_dim, surface_skin_t );
    }
}



/* Workspace method: Doxygen documentation will be auto-generated */
void surfaceFlatVaryingEmissivity(
              Matrix&    surface_los,
              Tensor4&   surface_rmatrix,
              Matrix&    surface_emission,
        const Vector&    f_grid,
        const Index&     stokes_dim,
        const Index&     atmosphere_dim,
        const Vector&    rte_los,
        const Numeric&   surface_skin_t,
        const Vector&    surface_emissivity )
{
  chk_if_in_range( "atmosphere_dim", atmosphere_dim, 1, 3 );
  chk_if_in_range( "stokes_dim", stokes_dim, 1, 4 );
  chk_if_over_0( "surface_skin_t", surface_skin_t );

  const Index   nf = f_grid.nelem();

  if( surface_emissivity.nelem() != nf )
    {
      ostringstream os;
      os << "The number of elements in *surface_emissivity* should match\n"
         << "length of *f_grid*."
         << "\n length of *f_grid* : " << nf 
         << "\n length of *surface_emissivity* : " << surface_emissivity.nelem()
         << "\n";
      throw runtime_error( os.str() );
    }

  if( min(surface_emissivity) < 0  ||  max(surface_emissivity) > 1 )
    {
      throw runtime_error( 
                  "All values in *surface_emissivity* must be inside [0,1]." );
    }

  out2 << "  Sets variables to model a flat surface\n";
  out3 << "     surface temperature: " << surface_skin_t << " K.\n";

  surface_los.resize( 1, rte_los.nelem() );
  surface_los(0,joker) = rte_los;
  surface_specular_los( surface_los(0, joker) , atmosphere_dim );

  surface_emission.resize( nf, stokes_dim );
  surface_rmatrix.resize(1,nf,stokes_dim,stokes_dim);
  surface_rmatrix = 0.0;
  surface_emission = 0.0;

  for( Index iv=0; iv<nf; iv++ )
    { 
      surface_emission(iv,0) = surface_emissivity[iv] * 
                                          planck( f_grid[iv], surface_skin_t );
      for( Index is=0; is<stokes_dim; is++ )
        { surface_rmatrix(0,iv,is,is) = 1 - surface_emissivity[iv]; }
    }
}



/* Workspace method: Doxygen documentation will be auto-generated */
void surfaceFlatSingleEmissivity(
              Matrix&    surface_los,
              Tensor4&   surface_rmatrix,
              Matrix&    surface_emission,
        const Vector&    f_grid,
        const Index&     stokes_dim,
        const Index&     atmosphere_dim,
        const Vector&    rte_los,
        const Numeric&   surface_skin_t,
        const Numeric&   surface_emissivity )
{
  chk_if_in_range( "surface_emissivity", surface_emissivity, 0, 1 );

  Vector a_vector( f_grid.nelem(), surface_emissivity );

  surfaceFlatVaryingEmissivity( surface_los, surface_rmatrix, surface_emission, 
                                f_grid, stokes_dim, atmosphere_dim, rte_los, 
                                surface_skin_t, a_vector );
}










//
//
// Old stuff:
//
//


/* Workspace method: Doxygen documentation will be auto-generated */
// void surfaceEmissivityInterpolate(
//                                   Matrix&    surface_los,
//                                   Tensor4&   surface_rmatrix,
//                                   Matrix&    surface_emission,
//                                   const Vector&    f_grid,
//                                   const GridPos&   rte_gp_lat,
//                                   const GridPos&   rte_gp_lon,
//                                   const Index&     stokes_dim,
//                                   const Index&     atmosphere_dim,
//                                   const Vector&    rte_los,
//                                   const Numeric&   surface_skin_t,
//                                   const Matrix&  surface_emissivity_field)
// {
//   const Index   nf = f_grid.nelem();
//   Numeric e;
//   Vector itw(4);
//   surface_los.resize( 1, rte_los.nelem() );
//   surface_los(0,joker) = rte_los;
//   surface_specular_los( surface_los(0, joker) , atmosphere_dim );

//   surface_emission.resize( nf, stokes_dim );
//   surface_rmatrix.resize(1,nf,stokes_dim,stokes_dim);
//   surface_rmatrix = 0.0;
//   surface_emission = 0.0;

//   interpweights(itw,rte_gp_lat,rte_gp_lon);
//   e=interp(itw,surface_emissivity_field,rte_gp_lat,rte_gp_lon);
//   for( Index iv=0; iv<nf; iv++ )
//     { 
//       surface_emission(iv,0) = e * planck( f_grid[iv], surface_skin_t );
//       for( Index is=0; is<stokes_dim; is++ )
//         { surface_rmatrix(0,iv,is,is) = 1 - e; }
//     }
// }



// //! surfaceCalc
// /*!
//    See the the online help (arts -d FUNCTION_NAME)

//    \author Patrick Eriksson
//    \date   2004-05-21
// */
// void surfaceCalc(
//               Matrix&                  iy,
//               ArrayOfTensor4&          diy_dvmr,
//               ArrayOfTensor4&          diy_dt,
//               Ppath&                   ppath,
//               Ppath&                   ppath_step,
//               ArrayOfPpath&            ppath_array,
//               Index&                   ppath_array_index,
//         const Vector&                  rte_pos,
//         const Vector&                  rte_los,
//         const Agenda&                  ppath_step_agenda,
//         const Agenda&                  rte_agenda,
//         const Agenda&                  iy_space_agenda,
//         const Agenda&                  iy_surface_agenda,
//         const Agenda&                  iy_cloudbox_agenda,
//         const Index&                   atmosphere_dim,
//         const Vector&                  p_grid,
//         const Vector&                  lat_grid,
//         const Vector&                  lon_grid,
//         const Tensor3&                 z_field,
//         const Tensor3&                 t_field,
//         const Tensor4&                 vmr_field,
//         const Matrix&                  r_geoid,
//         const Matrix&                  z_surface,
//         const Index&                   cloudbox_on, 
//         const ArrayOfIndex&            cloudbox_limits,
//         const Vector&                  f_grid,
//         const Index&                   stokes_dim,
//         const Index&                   ppath_array_do,
//         const ArrayOfIndex&            rte_do_vmr_jacs,
//         const Index&                   rte_do_t_jacs,
//         const Matrix&                  surface_los,
//         const Tensor4&                 surface_rmatrix,
//         const Matrix&                  surface_emission )
// {
//   // Some sizes
//   const Index   nf   = f_grid.nelem();
//   const Index   nlos = surface_los.nrows();


//   //--- Check input -----------------------------------------------------------
//   if( nlos )   // nlos = 0 if blackbody ground and some checks are not needed
//     {
//       if( surface_los.ncols() != rte_los.nelem() )
//         throw runtime_error( 
//                         "Number of columns in *surface_los* is not correct." );
//       if( nlos != surface_rmatrix.nbooks() )
//         throw runtime_error( 
//                   "Mismatch in size of *surface_los* and *surface_rmatrix*." );
//       if( surface_rmatrix.npages() != nf )
//         throw runtime_error( 
//                        "Mismatch in size of *surface_rmatrix* and *f_grid*." );
//       if( surface_rmatrix.ncols() != stokes_dim  ||  
//           surface_rmatrix.ncols() != stokes_dim ) throw runtime_error( 
//               "Mismatch between size of *surface_rmatrix* and *stokes_dim*." );
//     }
//   if( surface_emission.ncols() != stokes_dim )
//     throw runtime_error( 
//              "Mismatch between size of *surface_emission* and *stokes_dim*." );
//   if( surface_emission.nrows() != nf )
//     throw runtime_error( 
//                        "Mismatch in size of *surface_emission* and f_grid*." );
//   //---------------------------------------------------------------------------

//   ppath_array_index=0;


//   // Variables to hold downvelling radiation
//   Tensor3   I( nlos, nf, stokes_dim );
 
//   // Loop *surface_los*-es. If no such LOS, we are ready.
//   if( nlos > 0 )
//     {
//       // Make local version of *ppath* that later must be restored 
//       Ppath   pp_copy;
//       ppath_init_structure( pp_copy, atmosphere_dim, ppath.np );
//       ppath_copy( pp_copy, ppath );
//       //
//       const Index  pai = ppath_array_index;

//       for( Index ilos=0; ilos<nlos; ilos++ )
//         {
//           // Calculate downwelling radiation for LOS ilos 
//           iy_calc( iy, ppath, ppath_step,
//                    ppath_array_index, ppath_array,
//                    diy_dvmr, diy_dt,
//                    ppath_step_agenda, rte_agenda, 
//                    iy_space_agenda, iy_surface_agenda, iy_cloudbox_agenda, 
//                    atmosphere_dim, p_grid, lat_grid, lon_grid, z_field, 
//                    t_field, vmr_field,
//                    r_geoid, z_surface, cloudbox_on, cloudbox_limits, 
//                    rte_pos, surface_los(ilos,joker),
//                    f_grid, stokes_dim, 
//                    ppath_array_do,
//                    rte_do_vmr_jacs, rte_do_t_jacs );

//           I(ilos,joker,joker) = iy;

//           // Include reflection matrix in jacobian quantities
//           //
//           // Assume polarisation effects in surface_rmatrix
//           // (not of any impoartnce if stokes_dim=1)
//           const bool pol_r = true;
//           //
//           if( rte_do_vmr_jacs.nelem() )
//             {
//               for( Index iv=0; iv<nf; iv++ )
//                 {
//                   include_trans_in_diy_dq( diy_dvmr, iv, pol_r,
//                                           surface_rmatrix(ilos,iv,joker,joker),
//                                           ppath_array, ppath_array_index );
//                 }
//             }
//           if( rte_do_t_jacs )
//             {
//               for( Index iv=0; iv<nf; iv++ )
//                 {
//                   include_trans_in_diy_dq( diy_dt, iv, pol_r,
//                                           surface_rmatrix(ilos,iv,joker,joker),
//                                           ppath_array, ppath_array_index );
//                 }
//             }

//           // Reset *ppath_array_index*
//           ppath_array_index = pai;
//         }

//       // Copy data back to *ppath*.
//       ppath_init_structure( ppath, atmosphere_dim, pp_copy.np );
//       ppath_copy( ppath, pp_copy );
//     }

//   surface_calc( iy, I, surface_los, surface_rmatrix, surface_emission );
// }



// //! surfaceSpecular
// /*!
//    See the the online help (arts -d FUNCTION_NAME)

//    \author Patrick Eriksson
//    \date   2004-05-20
// */
// void surfaceSpecular(
//               Matrix&         iy,
//               Ppath&          ppath,
//               Ppath&          ppath_step,
//               Vector&         rte_pos,
//               GridPos&        rte_gp_p,
//               GridPos&        rte_gp_lat,
//               GridPos&        rte_gp_lon,
//               Vector&         rte_los,
//         const Agenda&         ppath_step_agenda,
//         const Agenda&         rte_agenda,
//         const Agenda&         iy_space_agenda,
//         const Agenda&         iy_surface_agenda,
//         const Agenda&         iy_cloudbox_agenda,
//         const Index&          atmosphere_dim,
//         const Vector&         p_grid,
//         const Vector&         lat_grid,
//         const Vector&         lon_grid,
//         const Tensor3&        z_field,
//         const Matrix&         r_geoid,
//         const Matrix&         z_surface,
//         const Index&          cloudbox_on, 
//         const ArrayOfIndex&   cloudbox_limits,
//         const Vector&         f_grid,
//         const Index&          stokes_dim,
//         const Numeric&        surface_t,
//         const Vector&         surface_rv,
//         const Vector&         surface_rh )
// {
//   // Check input
//   if( surface_rv.nelem() !=  f_grid.nelem() )
//     throw runtime_error( "Lengths of *surface_rv* and *f_grid* differ." );
//   if( surface_rh.nelem() !=  f_grid.nelem() )
//     throw runtime_error( "Lengths of *surface_rh* and *f_grid* differ." );
//   if( max(surface_rv)<0  ||  max(surface_rv)>1 )
//     throw runtime_error( 
//                       "*surface_rv* can only contain values in range [0,1]." );
//   if( max(surface_rh)<0  ||  max(surface_rh)>1 )
//     throw runtime_error( 
//                       "*surface_rh* can only contain values in range [0,1]." );



//   // Make local version of *ppath* that later must be restored 
//   Ppath   pp_copy;
//   ppath_init_structure( pp_copy, atmosphere_dim, ppath.np );
//   ppath_copy( pp_copy, ppath );

//   // Determine specular direction  
//   surface_specular_los( rte_los, atmosphere_dim );

//   // Calculate downwelling radiation and put in local variable
//   Vector   pos( rte_pos.nelem() ), los( rte_los.nelem() );
//   //
//   pos = rte_pos;
//   los = rte_los;
//   //  
//   iy_calc( iy, ppath, ppath_step,
//            ppath_step_agenda, rte_agenda, iy_space_agenda, 
//            iy_surface_agenda, iy_cloudbox_agenda, atmosphere_dim, p_grid, 
//            lat_grid, lon_grid, z_field, r_geoid, z_surface, cloudbox_on, 
//            cloudbox_limits, pos, los, f_grid, stokes_dim );
//   //
//   Matrix   idown( iy.nrows(), iy.ncols() );
//   idown = iy;

//   // Add up reflected radiation and surface emission
//   //
//   Vector b(stokes_dim);             // Surface emission Stokes vector 
//   Matrix R(stokes_dim, stokes_dim); // Reflection matrix
//   //
//   for( Index i=0; i<f_grid.nelem(); i++ )
//     {
//       // Set up variables

//       // THIS PART IS NOT COMPLETE. THERE SHALL BE MORE TERMS
//       const Numeric   rmean = ( surface_rh[i] + surface_rv[i] ) / 2;
//       b[0] = ( 1 - rmean ) * planck( f_grid[i], surface_t );
//       for( Index j=0; j<stokes_dim; j++ )
//         { R(j,j) = rmean; }

//       // Make math
//       mult( iy(i,joker), R, iy(i,joker) ); //
//       iy(i,joker) += b;
//     }

//   // Copy data back to *ppath*.
//   ppath_init_structure( ppath, atmosphere_dim, pp_copy.np );
//   ppath_copy( ppath, pp_copy );
// }


//   agenda_data.push_back
//     (AgRecord
//      ( NAME( "surface_agenda" ),
//        DESCRIPTION
//        (
//         "Describes the properties of the surface to consider when there is a\n"
//         "surface reflection.\n"
//         "\n"
//         "The surface properties are described by the WSVs\n"
//         "*surface_emission*, *surface_los* and *surface_refl_coeffs*.\n"
//         "\n"
//         "The upwelling radiation from the surface is calculated as the sum\n"
//         "of *surface_emission* and the spectra calculated for the directions\n"
//         "given by *surface_los*, multiplicated with the weights in\n"
//         "*surface_refl_coeffs*. Or (for frequency i): \n"
//         "   i_up = i_emission + sum_over_los( W*i_down ) \n"
//         "where i_up is the upwelling radiation, i_emission is row i of\n"
//         "*surface_emission*, W is the reflection matrix in \n"
//         "*surface_refl_coeffs* for the frequency and LOS of concern and \n"
//         "i_down is the downwelling radiation for the LOS given in\n"
//         "*surface_los*. \n"
//         "\n"
//         "With other words, the scattering properties of the surface are \n"
//         "described by the variables *surface_los* and *surface_refl_coeffs*.\n"
//         "\n"
//         "A function calling this agenda shall set *rte_los* and *rte_gp_XXX*\n"
//         "to match the line-of-sight and position at the surface reflection\n"
//         "point.\n"
//         "\n"
//         "See further the user guide.\n"
//         "\n"
//         "Usage:   Called from *RteCalc*."
//         ),
//        OUTPUT( surface_emission_, surface_los_, surface_refl_coeffs_  ),
//        INPUT(  f_grid_, stokes_dim_, rte_gp_p_, rte_gp_lat_, rte_gp_lon_, 
//                rte_los_, r_geoid_, z_surface_, t_field_ )));




//    wsv_data.push_back
//      (WsvRecord
//       ( NAME( "surface_agenda" ),
//  DESCRIPTION
//  (
//   "See agendas.cc."
//   ),
//  GROUP( Agenda_ )));
   
   
//    wsv_data.push_back
//      (WsvRecord
//       ( NAME( "surface_emission_field" ),
//  DESCRIPTION
//  (
//   "The emission from the surface which is stored in latitude and \n"
//   " longitude grids.\n"
//   "\n"
//   "An emissivity calculation using the method *surfaceFastem* \n"
//   "calculates surface_emission_field at the same grid points as \n"
//   "lat_grid and lon_grid. The surface_emission_field is then \n"
//   "interpolated on to the grid crossing position using the \n"
//   "method *surfaceInterpolate* to get value of *surface_emission*\n"
//   "at a specified position. \n"
//   "\n"
//   "Usage:      Output from *surfaceFastem*. \n"
//   "\n"
//   "Unit:       W / (m^2 Hz sr)\n"
//   "\n"
//   "Dimensions: [lat_grid, lon_grid, f_grid, stokes_dim ]"
//   ), 
//        GROUP( Tensor4_ )));

//   wsv_data.push_back
//     (WsvRecord
//      ( NAME( "surface_emissivity_field" ),
//        DESCRIPTION
//        (
//         "This variable holds the value of surface emissivity.\n"
//  "\n"
//         "This field is stored in a Tensor3 where the first two dimensions\n"
//         "holds the latitude and longitude grid and the third dimension holds\n"
//         "the value for each *stokes_dim*.  The surface_emissivity_field takes \n"
//         "values between 0 and 1. If only a prescribed value of surface \n"
//         "emissivity is to be used, then set the value accordingly, and use the method \n"
//         "*surfaceNoScatteringSingleEmissivity* in the *surface_agenda*. The value\n"
//         "of the field is set to -1 if one wants to calculate the surface emissivity \n"
//         "using FASTEM.  Accordingly, the method surfaceFastem has to be used in\n"
//  "*surface_agenda*\n"
//         "\n"
//  "Usage:    Set from the control file as a workspace variable. \n"
//         "\n"
//         "Units:      -\n"
//         "\n"
//         "Dimensions: [ lat_grid, lon_grid, stokes_dim ]"
//         ), 
//        GROUP( Tensor3_ )));

//  wsv_data.push_back
//     (WsvRecord
//      ( NAME( "surface_fastem_constants" ),
//        DESCRIPTION
//        (
//         "This variable holds some constant parameters used in fastem model.\n"
//  "\n"
//         "There are 59  ocean surface emissivity model constants that are \n"
//  "used in the fastem calculations.\n"
//         "\n"
//  "Usage:    Read in from file to be used as input for fastem calculations. \n"
//         "\n"
//         "Units:      -\n"
//         "\n"
//         "Size: [ 59 ]"
//         ), 
//        GROUP( Vector_ )));


//   wsv_data.push_back
//     (WsvRecord
//      ( NAME( "surface_refl_coeffs_field" ),
//        DESCRIPTION
//        (
//         "The reflection coefficients from the directions given by\n"
//         "*surface_los* to the direction of interest at the lat_grid\n"
//  "and lon_grid. \n"
//         "\n"
//         "The difference with *surface_refl_coeffs* is that, here the \n"
//         "coefficient matrix is held in latitude and longitude grid \n"
//         "positions whereas *surface_refl_coeffs* corresponds to only\n"
//         "a specific position. Like *surface_emission_field*, \n"
//  "*surface_refl_coeffs_field* can also be calculated using the \n"
//  "method *surfaceFastem*. The variable *surface_refl_coeffs_field*\n"
//         "is interpolated onto the specific grid crossing points using the\n"
//         "method *surfaceInterpolate*. \n"
//  "\n"
//         "See further *surface_refl_coeffs* and *surfaceInterpolate*.\n"
//         "\n"
//         "Usage:      Output from *surfaceFastem*. \n"
//         "\n"
//         "Units:      -\n"
//         "\n"
//         "Dimensions: [ lat_grid, lon_grid, surface_los, f_grid, stokes_dim, stokes_dim ]"
//         ), 
//        GROUP( Tensor6_ )));

//   wsv_data.push_back
//     (WsvRecord
//      ( NAME( "surface_temperature" ),
//        DESCRIPTION
//        (
//         "This variable holds the surface temperature values for all \n"
//  "latitude - longitude grid points. \n"
//  "\n"
//  "Usage:    Input to FASTEM calculations. Can be set from the control file \n"
//  "as a workspace variable. \n"
//         "\n"
//         "Units:      K"
//         "\n"
//         "Dimensions: [ lat_grid, lon_grid, 1 ]"
//         ), 
//        GROUP( Tensor3_ )));

//   wsv_data.push_back
//     (WsvRecord
//      ( NAME( "surface_wind_field" ),
//        DESCRIPTION
//        (
//         "This variable gives the u- and v- components of surface wind.\n"
//  "\n"
//         "This field is stored in a Tensor3 where the first two dimensions\n"
//         "holds the latitude and longitude grid and the third dimension holds\n"
//         "the value for u- and v- components respectively.  The  \n"
//         "surface_wind_field(lat_grid, lon_grid, 0) gives the u- component and  \n"
//         "surface_wind_field(lat_grid, lon_grid, 1) gives the v- component in m/s.\n"
//  "\n"
//  "Usage:    Input to FASTEM calculations. Can be set from the control file \n"
//  "as a workspace variable. \n"
//         "\n"
//         "Units:      m/s"
//         "\n"
//         "Dimensions: [ lat_grid, lon_grid, 2 ]"
//         ), 
//        GROUP( Tensor3_ )));




// //! surfaceNoScatteringSingleEmissivity
// /*!
//    See the the online help (arts -d FUNCTION_NAME)

//    \author Patrick Eriksson
//    \date   2002-09-22
// */
// void surfaceNoScatteringSingleEmissivity(
//               Matrix&    surface_emission, 
//               Matrix&    surface_los, 
//               Tensor4&   surface_refl_coeffs,
//         const Vector&    f_grid,
//         const Index&     stokes_dim,
//         const GridPos&   rte_gp_p,
//         const GridPos&   rte_gp_lat,
//         const GridPos&   rte_gp_lon,
//         const Vector&    rte_los,
//         const Index&     atmosphere_dim,
//         const Vector&    p_grid,
//         const Vector&    lat_grid,
//         const Vector&    lon_grid,
//         const Matrix&    r_geoid,
//         const Matrix&    z_surface,
//         const Tensor3&   t_field,
//         const Numeric&   e )
// {
//   //--- Check input -----------------------------------------------------------
//   chk_if_in_range( "stokes_dim", stokes_dim, 1, 4 );
//   chk_if_in_range( "atmosphere_dim", atmosphere_dim, 1, 3 );
//   chk_atm_grids( atmosphere_dim, p_grid, lat_grid, lon_grid );
//   chk_atm_surface( "r_geoid", r_geoid, atmosphere_dim, lat_grid, lon_grid );
//   chk_atm_surface( "z_surface", z_surface, atmosphere_dim, lat_grid, lon_grid );
//   chk_atm_field( "t_field", t_field, atmosphere_dim, p_grid, lat_grid, 
//                                                                     lon_grid );
//   chk_if_in_range( "the keyword *e*", e, 0, 1 );
//   //---------------------------------------------------------------------------

//   out2 << 
//         "  Sets the surface to have no scattering and a constant emissivity.\n";
//   out3 << 
//         "     Surface temperature is obtained by interpolation of *t_field*.\n";

//   // Some sizes
//   const Index   nf = f_grid.nelem();

//   // Resize output arguments
//   surface_emission.resize( nf, stokes_dim );
//   surface_los.resize( 1, rte_los.nelem() );
//   surface_refl_coeffs.resize( 1, nf, stokes_dim, stokes_dim );
//   surface_refl_coeffs = 0;

//   // Determine the temperature at the point of the surface reflection
//   const Numeric t = interp_atmfield_by_gp( atmosphere_dim, p_grid, lat_grid, 
//               lon_grid, t_field, rte_gp_p, rte_gp_lat, rte_gp_lon );

//   out3 << "     Surface temperature is                     : " << t << "\n";

//   // Fill surface_emission and surface_refl_coeffs
//   for( Index i=0; i<nf; i++ )
//     { 
//       surface_emission(i,0) = e * planck( f_grid[i], t ); 
//       surface_refl_coeffs(0,i,0,0) = 1 - e;
//       for( Index is=1; is<stokes_dim; is++ )
//         { 
//           surface_emission(i,is) = 0; 
//           surface_refl_coeffs(0,i,is,is) = 1 - e;
//         }
//       // Note that other elements of surface_refl_coeffs are set to 0 above.
//     }

//   // Calculate LOS for downwelling radiation
//   surface_specular_los( surface_los(0,joker), atmosphere_dim, r_geoid,
//                 z_surface, lat_grid, lon_grid, rte_gp_lat,rte_gp_lon, rte_los );

//   out3 << "     Zenith angle for upwelling radiation is   : " 
//        << rte_los[0] << "\n";
//   if( atmosphere_dim > 2 )
//     {
//       out3 << "     Azimuth angle for upwelling radiation is  : " 
//            << rte_los[1] << "\n";
//     }
//   out3 << "     Zenith angle for downwelling radiation is : " 
//        << surface_los(0,0) << "\n";
//   if( atmosphere_dim > 2 )
//     {
//       out3 << "     Azimuth angle for downwelling radiation is: " 
//            << surface_los(0,1) << "\n";
//     }
// }


// //! surfaceFastem
// /*!
//   The method decides whether to use *surfaceNoScatteringSingleEmissivity* or 
//   *surfaceFastem*. If *surfaceFastem*, then calculates surface emissivity using
//   fastem model implementation.
//   The decision whether to use *surfaceNoScatteringSingleEmissivity* or
//   *surfaceFastem* is made based on the value of the first element of 
//   *surface_emissivity_field*. If this value is set to be between 0 and 1
//   *surface_emissivity_field* will be returned with the same value.  If this 
//   value is -1 *surfaceFastem* will calculate the emissivity based on the 
//   fastem model implementation in ARTS. The output of this function is 
//   *surface_emission_field* and *surface_refl_coeffs_field+ stored in the
//   latitude and longitude grid points. 
  

//    \author Sreerekha Ravi
//    \date   2004-08-10
// */

// void surfaceFastem(
//           Tensor3&   surface_emissivity_field,
//         Tensor4&    surface_emission_field, 
//         Tensor6&   surface_refl_coeffs_field,
//         const Vector&    f_grid,
//         const Index&     stokes_dim,
//         const Index&     atmosphere_dim,
//         const Vector&    p_grid,
//         const Vector&    lat_grid,
//         const Vector&    lon_grid,
//         const Matrix&    r_geoid,
//         const Matrix&    z_surface,
//         const Tensor3&   t_field,
//         const Vector& surface_fastem_constants,         
//         const Tensor3&   surface_wind_field,
//         const Tensor3&   surface_temperature)
// {
//   //--- Check input -----------------------------------------------------------
//   chk_if_in_range( "stokes_dim", stokes_dim, 1, 4 );
//   chk_if_in_range( "atmosphere_dim", atmosphere_dim, 1, 3 );
//   chk_atm_grids( atmosphere_dim, p_grid, lat_grid, lon_grid );
//   chk_atm_surface( "r_geoid", r_geoid, atmosphere_dim, lat_grid, lon_grid );
//   chk_atm_surface( "z_surface", z_surface, atmosphere_dim, lat_grid, lon_grid ); 
//   chk_atm_field( "t_field", t_field, atmosphere_dim, p_grid, lat_grid, 
//                                                                     lon_grid );
//   //-----------------------------------------------------------------------------
//    const Index   nf = f_grid.nelem();
   
//    surface_emission_field.resize(lat_grid.nelem(), lon_grid.nelem(), 
//               f_grid.nelem(), stokes_dim);
//    surface_refl_coeffs_field.resize(lat_grid.nelem(), lon_grid.nelem(), 
//                  1, f_grid.nelem(),  stokes_dim, stokes_dim);
   
//    if (surface_emissivity_field(0, 0, 0) >= 0||
//        surface_emissivity_field(0, 0, 0) <= 1.0)
//      {
//        return;
//      }
//    else
//      if (surface_emissivity_field(0, 0, 0) == -1)
//        {
//   //call FASTEM functions for each latitude and longitude grid
//   //points 
//   for (Index lat_ind = 0; lat_ind < lat_grid.nelem(); ++ lat_ind)
//     {
//       for (Index lon_ind = 0; lon_ind < lon_grid.nelem(); ++ lon_ind)
//         {
                 
//       for( Index i=0; i<nf; i++ )
//         { 
//           //Calculate surf_emis_field using fastem
//           fastem(surface_emissivity_field(lat_ind, lon_ind, joker),
//              surface_temperature(lat_ind, lon_ind, 0),
//              surface_wind_field(lat_ind, lon_ind, joker) , 
//              surface_fastem_constants, f_grid[i]);
             
//           //Calculate emission by multiplying surface
//           //emissivity with surface temperature.
//           surface_emission_field(lat_ind, lon_ind, i, joker) = 
//             surface_emissivity_field(lat_ind, lon_ind, joker) *
//             planck( f_grid[i], surface_temperature (lat_ind, lon_ind, 0) ); 
             
//           for( Index is=0; is<stokes_dim; is++ )
//             { 
//           surface_refl_coeffs_field(lat_ind, lon_ind, 0,i,is,is) = 
//             1 - surface_emission_field(lat_ind, lon_ind, i, is);
//             } //stokes_dim
//         }//frequency grid
//         }//lon grid
//     }//lat_grid
//        }// if fastem
// }// end of function



// //! surfaceTreatAsBlackbody
// /*!
//    See the the online help (arts -d FUNCTION_NAME)

//    \author Patrick Eriksson
//    \date   2002-09-17
// */




//   md_data_raw.push_back     
//     ( MdRecord
//       ( NAME("surfaceFastem"),
//         DESCRIPTION
//         (
//          "The method decides whether to use *surfaceNoScatteringSingleEmissivity* or \n"
//          "*surfaceFastem*. If *surfaceFastem*, then calculates surface emissivity using\n"
//   "fastem model implementation.\n"
//          "\n"
//          "The decision whether to use *surfaceNoScatteringSingleEmissivity* or\n"
//   "*surfaceFastem* is made based on the value of the first element of \n"
//          "surface_emissivity_field. If this value is set to be between 0 and 1\n"
//   "*surface_emissivity_field* will be returned with the same value.  If this \n"
//          "value is -1 *surfaceFastem* will calculate the emissivity based on the \n"
//   "fastem model implementation in ARTS. The output of this function is \n"
//   "*surface_emission_field* and *surface_refl_coeffs_field* stored in the\n"
//          "latitude and longitude grid points. \n"
//   ),
//         OUTPUT( surface_emissivity_field_, surface_emission_field_,
//      surface_refl_coeffs_field_),
//         INPUT( surface_emissivity_field_, f_grid_, stokes_dim_, atmosphere_dim_, p_grid_,
//         lat_grid_, lon_grid_, r_geoid_,z_surface_, t_field_, surface_fastem_constants_,
//         surface_wind_field_, surface_temperature_),
//         GOUTPUT( ),
//         GINPUT( ),
//         KEYWORDS( ),
//         TYPES( )));
 
//   md_data_raw.push_back     
//     ( MdRecord
//       ( NAME("surfaceNoScatteringSingleEmissivity"),
//         DESCRIPTION
//         (
//          "Treats the surface to not cause any scattering, and to have a\n"
//          "reflection coefficient of 1-e. \n"
//          "\n"
//          "The size of *surface_emission* is set to [ nf, stokes_dim ] where \n"
//          "nf is the length of *f_grid*. Columns 2-4 are set to zero.\n"
//          "The temperature of the surface is obtained by interpolating \n"
//          "*t_field* to the position of the surface reflection. The obtained \n"
//          "temperature and *f_grid* are then used as input to the Planck\n"
//          "function. The emission from the surface is then calculated as eB,\n"
//          "where B is the Planck function.\n"
//          "\n"
//          "It is here assumed that the downwelling radiation to consider\n"
//          "comes from a single direction and the returned *surface_los*\n"
//          "contains only one LOS. The slope of the surface is considered\n"
//          "when calculating the LOS for the downwelling radiation. The\n"
//          "reflection matrices in *surface_refl_coeffs* are all set to be\n"
//          "diagonal matrices, where all diagonal elements are 1-e.\n"
//          "\n"
//          "Keywords: \n"
//          "   e : Surface emissivity. Must be a value in the range [0,1].\n"
//          "       All frequencies are assumed to have the same e."
//         ),
//         OUTPUT( surface_emission_, surface_los_, surface_refl_coeffs_ ),
//         INPUT( f_grid_, stokes_dim_, rte_gp_p_, rte_gp_lat_, rte_gp_lon_, 
//                rte_los_, atmosphere_dim_, p_grid_, lat_grid_, lon_grid_, 
//                r_geoid_,z_surface_, t_field_ ),
//         GOUTPUT( ),
//         GINPUT( ),
//         KEYWORDS(    "e"    ),
//         TYPES(    Numeric_t )));
  
//   md_data_raw.push_back     
//     ( MdRecord
//       ( NAME("surfaceTreatAsBlackbody"),
//         DESCRIPTION
//         (
//          "Sets the surface variables (see below) to model a blackbdoy surface.\n"
//          "\n"
//          "The function creates the variables *surface_emission*, *surface_los*\n"
//          "and *surface_refl_coeffs*. When the surface is treated to act as a\n"
//          "blackbody, no downwelling radiation needs to be calculated and\n"
//          "*surface_los* and *surface_refl_coeffs* are set to be empty.\n"
//          "\n"
//          "The size of *surface_emission* is set to [ nf, stokes_dim ] where \n"
//          "nf is the length of *f_grid*. Columns 2-4 are set to zero.\n"
//          "\n"
//          "The temperature of the surface is obtained by interpolating \n"
//          "*t_field* to the position of the surface reflection. The obtained \n"
//          "temperature and *f_grid* are then used as input to the Planck\n"
//          "function and the calculated blackbody radiation is put into the\n"
//          "first column of *surface_emission*.\n"
//          "\n"
//          "Note that this function does not use *rte_los*, *r_geoid* and\n"
//          "*z_surface* as input, and if used inside *surface_agenda*,\n"
//          "ignore commands for those variables must be added to the agenda."
//         ),
//         OUTPUT( surface_emission_, surface_los_, surface_refl_coeffs_ ),
//         INPUT( f_grid_, stokes_dim_, rte_gp_p_, rte_gp_lat_, rte_gp_lon_,
//                atmosphere_dim_, p_grid_, lat_grid_, lon_grid_, t_field_ ),
//         GOUTPUT( ),
//         GINPUT( ),
//         KEYWORDS( ),
//         TYPES( )));



