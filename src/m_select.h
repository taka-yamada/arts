/* Copyright (C) 2002-2008 Oliver Lemke <olemke@core-dump.info>

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

/*!
  \file   m_copy.h
  \author Oliver Lemke <olemke@core-dump.info>
  \date   2008-08-27
  
  \brief  Implementation of Select.
  
  This file contains the implementation of the supergeneric method Select.
*/

#ifndef m_select_h
#define m_select_h

#include "messages.h"
#include "mystring.h"
#include "workspace_ng.h"
#include "agenda_class.h"


/* Workspace method: Doxygen documentation will be auto-generated */
template< class T >
void Select(// WS Generic Output:
            T& needles _U_,
            // WS Generic Input:
            const T& haystack _U_,
            const ArrayOfIndex& needleind _U_)
{
  throw runtime_error( "Select is not implemented for this type" );
}


/* Workspace method: Doxygen documentation will be auto-generated */
void Select(Workspace& ws _U_,
            // WS Generic Output:
            Agenda& out _U_,
            // WS Generic Input:
            const Agenda& in _U_,
            const ArrayOfIndex& needleind _U_)
{
  throw runtime_error( "Select is not implemented for this type" );
}


/* Workspace method: Doxygen documentation will be auto-generated */
template< class T >
void Select(// WS Generic Output:
            Array<T>& needles,
            // WS Generic Input:
            const Array<T>& haystack,
            const ArrayOfIndex& needleind)
{
  needles.resize( needleind.nelem() );
  for( Index i = 0; i < needleind.nelem(); i++)
    {
      if (haystack.nelem() <= needleind[i])
        {
          ostringstream os;
          os << "The input vector only has " << haystack.nelem()
            << " elements. But one of the needle indexes is "
            << needleind[i] << "." << endl;
          os << "The indexes must be between 0 and " << haystack.nelem() - 1;
          throw runtime_error (os.str());
        }
      else
        needles[i] = haystack[needleind[i]];
    }
}


/* Workspace method: Doxygen documentation will be auto-generated */
void Select(// WS Generic Output:
            Vector& needles,
            // WS Generic Input:
            const Vector& haystack,
            const ArrayOfIndex& needleind)
{
  needles.resize( needleind.nelem() );
  for( Index i = 0; i < needleind.nelem(); i++)
    {
      if (haystack.nelem() <= needleind[i])
        {
          ostringstream os;
          os << "The input vector only has " << haystack.nelem()
            << " elements. But one of the needle indexes is "
            << needleind[i] << "." << endl;
          os << "The indexes must be between 0 and " << haystack.nelem() - 1;
          throw runtime_error (os.str());
        }
      else
        needles[i] = haystack[needleind[i]];
    }
}


#endif // m_select_h