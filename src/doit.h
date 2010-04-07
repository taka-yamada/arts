/* Copyright (C) 2003-2008 Claudia Emde <claudia.emde@dlr.de>

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
  ===  File description 
  ===========================================================================*/

/*!
  \file   doit.h
  \author Claudia Emde <claudia.emde@dlr.de>
  \date   2003-06-03
  
  \brief  Radiative transfer in cloudbox.
  
  This file contains functions related to the radiative transfer in the 
  cloudbox using the DOIT method.
*/


#ifndef doit_h
#define doit_h

#include "agenda_class.h"
#include "matpackVI.h"
#include "ppath.h"


void cloud_fieldsCalc(Workspace& ws,
                      // Output:
                      Tensor5View ext_mat_field,
                      Tensor4View abs_vec_field,
                      // Input:
                      const Agenda& spt_calc_agenda,
                      const Agenda& opt_prop_part_agenda,
                      const Index& scat_za_index, 
                      const Index& scat_aa_index,
                      const ArrayOfIndex& cloudbox_limits,
                      const Tensor3View t_field, 
                      const Tensor4View pnd_field
                     );

void cloud_ppath_update1D(
                  Workspace& ws,
                  Tensor6View i_field,
                  // ppath_step_agenda:
                  const Index& p_index,
                  const Index& scat_za_index,
                  ConstVectorView scat_za_grid,
                  const ArrayOfIndex& cloudbox_limits,
                  ConstTensor6View scat_field,
                  // Calculate scalar gas absorption:
                  const Agenda& abs_scalar_gas_agenda,
                  ConstTensor4View vmr_field,
                  // Gas absorption:
                  const Agenda& opt_prop_gas_agenda,
                  // Propagation path calculation:
                  const Agenda& ppath_step_agenda,
                  ConstVectorView p_grid,
                  ConstTensor3View z_field,
                  ConstMatrixView r_geoid,
                  ConstMatrixView z_surface,
                  // Calculate thermal emission:
                  ConstTensor3View t_field,
                  ConstVectorView f_grid,
                  const Index& f_index,
                  //particle opticla properties
                  ConstTensor5View ext_mat_field,
                  ConstTensor4View abs_vec_field,
                  const Agenda& surface_prop_agenda,
                  const Index& scat_za_interp
                  );

void cloud_ppath_update1D_noseq(
                          Workspace& ws,
                          // Input and output
                          Tensor6View doit_i_field,
                          // ppath_step_agenda:
                          const Index& p_index,
                          const Index& scat_za_index,
                          ConstVectorView scat_za_grid,
                          const ArrayOfIndex& cloudbox_limits,
                          ConstTensor6View doit_i_field_old,
                          ConstTensor6View doit_scat_field,
                          // Calculate scalar gas absorption:
                          const Agenda& abs_scalar_gas_agenda,
                          ConstTensor4View vmr_field,
                          // Gas absorption: 
                          const Agenda& opt_prop_gas_agenda,
                          // Propagation path calculation:
                          const Agenda& ppath_step_agenda,
                          ConstVectorView  p_grid,
                          ConstTensor3View z_field,
                          ConstMatrixView r_geoid,
                          ConstMatrixView z_surface,
                          // Calculate thermal emission:
                          ConstTensor3View t_field,
                          ConstVectorView f_grid,
                          // used for surface ?
                          const Index& f_index,
                          //particle optical properties
                          ConstTensor5View ext_mat_field,
                          ConstTensor4View abs_vec_field,
                          //const Agenda& surface_prop_agenda,
                          const Index& scat_za_interp
                          );

void cloud_ppath_update1D_planeparallel(
                  Workspace& ws,
                  Tensor6View i_field,
                  // ppath_step_agenda:
                  const Index& p_index,
                  const Index& scat_za_index,
                  ConstVectorView scat_za_grid,
                  const ArrayOfIndex& cloudbox_limits,
                  ConstTensor6View scat_field,
                  // Calculate scalar gas absorption:
                  const Agenda& abs_scalar_gas_agenda,
                  ConstTensor4View vmr_field,
                  // Gas absorption:
                  const Agenda& opt_prop_gas_agenda,
                  // Propagation path calculation:
                  const Agenda& ppath_step_agenda,
                  ConstVectorView p_grid,
                  ConstTensor3View z_field,
                  ConstMatrixView r_geoid,
                  // Calculate thermal emission:
                  ConstTensor3View t_field,
                  ConstVectorView f_grid,
                  const Index& f_index,
                  //particle opticla properties
                  ConstTensor5View ext_mat_field,
                  ConstTensor4View abs_vec_field
                  // const Agenda& surface_agenda 
                  );

void cloud_ppath_update3D(Workspace& ws,
                          Tensor6View doit_i_field,
                          // ppath_step_agenda:
                          const Index& p_index,
                          const Index& lat_index,
                          const Index& lon_index,
                          const Index& scat_za_index,
                          const Index& scat_aa_index,
                          ConstVectorView scat_za_grid,
                          ConstVectorView scat_aa_grid,
                          const ArrayOfIndex& cloudbox_limits,
                          ConstTensor6View doit_scat_field,
                          // Calculate scalar gas absorption:
                          const Agenda& abs_scalar_gas_agenda,
                          ConstTensor4View vmr_field,
                          // Gas absorption:
                          const Agenda& opt_prop_gas_agenda,
                          // Propagation path calculation:
                          const Agenda& ppath_step_agenda,
                          ConstVectorView p_grid,
                          ConstVectorView lat_grid,
                          ConstVectorView lon_grid,
                          ConstTensor3View z_field,
                          ConstMatrixView r_geoid,
                          ConstMatrixView z_surface,
                          // Calculate thermal emission:
                          ConstTensor3View t_field,
                          ConstVectorView f_grid,
                          const Index& f_index,
                          //particle optical properties
                          ConstTensor5View ext_mat_field,
                          ConstTensor4View abs_vec_field,
                          const Index& //scat_za_interp
                          );

void cloud_RT_no_background(Workspace& ws,
                            //Output
                            Tensor6View doit_i_field,
                            // Input
                            const Agenda& abs_scalar_gas_agenda,
                            const Agenda& opt_prop_gas_agenda,
                            const Ppath& ppath_step, 
                            ConstVectorView t_int,
                            ConstMatrixView vmr_list_int,
                            ConstTensor3View ext_mat_int,
                            ConstMatrixView abs_vec_int,
                            ConstMatrixView sca_vec_int,
                            ConstMatrixView doit_i_field_int,
                            ConstVectorView p_int,
                            const ArrayOfIndex& cloudbox_limits,
                            ConstVectorView f_grid,
                            const Index& f_index,
                            const Index& p_index,
                            const Index& lat_index,
                            const Index& lon_index, 
                            const Index& scat_za_index,
                            const Index& scat_aa_index);

void cloud_RT_surface(Workspace& ws,
                      //Output
                      Tensor6View doit_i_field,
                      //Input
                      const Agenda& surface_prop_agenda,
                      const Index& f_index,
                      const Index& stokes_dim,
                      const Ppath& ppath_step,
                      const ArrayOfIndex& cloudbox_limits, 
                      ConstVectorView scat_za_grid, 
                      const Index& scat_za_index
                      );


void ppath_step_in_cloudbox(Workspace& ws,
                            Ppath& ppath_step,
                            const Agenda& ppath_step_agenda,
                            const Index& p,
                            const Index& lat, 
                            const Index& lon,
                            ConstTensor3View z_field,
                            ConstMatrixView r_geoid,
                            ConstMatrixView z_surface,
                            ConstVectorView scat_za_grid,
                            ConstVectorView aa_grid,
                            const Index& scat_za_index,
                            const Index& scat_aa_index,
                            ConstVectorView p_grid,
                            ConstVectorView lat_grid,
                            ConstVectorView lon_grid);

void interp_cloud_coeff1D(//Output
                          Tensor3View ext_mat_int,
                          MatrixView abs_vec_int,
                          MatrixView sca_vec_int,
                          MatrixView doit_i_field_int,
                          VectorView t_int, 
                          MatrixView vmr_list_int,
                          VectorView p_int,
                          //Input
                          ConstTensor5View ext_mat_field, 
                          ConstTensor4View abs_vec_field,
                          ConstTensor6View doit_scat_field,
                          ConstTensor6View doit_i_field,
                          ConstTensor3View t_field, 
                          ConstTensor4View vmr_field, 
                          ConstVectorView p_grid,
                          const Ppath& ppath_step,
                          const ArrayOfIndex& cloudbox_limits,
                          ConstVectorView scat_za_grid,
                          const Index& scat_za_interp);

void za_gridOpt(//Output:
                Vector& za_grid_opt,
                Matrix& i_field_opt,
                // Input
                ConstVectorView za_grid_fine,
                ConstTensor6View i_field,
                const Numeric& acc,
                const Index& scat_za_interp);


#endif //doit_h