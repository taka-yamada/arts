/* Copyright (C) 2018
   Richard Larsson
                            
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


#include "linerecord.h"
#include "matpackI.h"


void statistical_equilibrium_equation(MatrixView A,
                                      ConstVectorView Aij,
                                      ConstVectorView Bij,
                                      ConstVectorView Bji,
                                      ConstVectorView Cij,
                                      ConstVectorView Cji,
                                      ConstVectorView Jij,
                                      const ArrayOfIndex& upper,
                                      const ArrayOfIndex& lower);


void dampened_statistical_equilibrium_equation(MatrixView A,
                                               ConstVectorView x,
                                               ConstVectorView Aij,
                                               ConstVectorView Bij,
                                               ConstVectorView Bji,
                                               ConstVectorView Cij,
                                               ConstVectorView Cji,
                                               ConstVectorView Jij,
                                               ConstVectorView Lambda,
                                               const ArrayOfIndex& upper,
                                               const ArrayOfIndex& lower,
                                               const Numeric& total_number_count=1.0);


void use_total_number_count_statistical_equilibrium_matrix(MatrixView A, VectorView x, ConstVectorView r, const Index row);


Vector createAij(const ArrayOfLineRecord& abs_lines);


Vector createBij(const ArrayOfLineRecord& abs_lines);


Vector createBji(ConstVectorView Bij, const ArrayOfLineRecord& abs_lines);


Vector createCijFromPressureBroadening(const ArrayOfLineRecord& abs_lines, 
                                       ConstVectorView vmrs,
                                       const ArrayOfIndex& broad_spec_locations,
                                       const Numeric& T,
                                       const Numeric& P,
                                       const Index this_species,
                                       const Index water_species);


void setCijFromPressureBroadening(VectorView Cij, 
                                  const ArrayOfLineRecord& abs_lines, 
                                  ConstVectorView vmrs,
                                  const ArrayOfIndex& broad_spec_locations,
                                  const Numeric& T,
                                  const Numeric& P,
                                  const Index this_species,
                                  const Index water_species,
                                  const Index n);


Vector createCji(ConstVectorView Cij, const ArrayOfLineRecord& abs_lines, const Numeric& T);


void setCji(VectorView Cji, ConstVectorView Cij, const ArrayOfLineRecord& abs_lines, const Numeric& T, const Index n);


void nlte_positions_in_statistical_equilibrium_matrix(ArrayOfIndex& upper, ArrayOfIndex& lower, 
                                                      const ArrayOfLineRecord& abs_lines, 
                                                      const ArrayOfQuantumIdentifier& nlte_quantum_identifiers);

Index find_first_unique_in_lower(const ArrayOfIndex& upper, const ArrayOfIndex& lower) noexcept;