/*!
 * \file CVariable.hpp
 * \brief Declaration and inlines of the class
 *        to define the variables of the mesh movement.
 * \author R. Sanchez
 * \version 6.2.0 "Falcon"
 *
 * The current SU2 release has been coordinated by the
 * SU2 International Developers Society <www.su2devsociety.org>
 * with selected contributions from the open-source community.
 *
 * The main research teams contributing to the current release are:
 *  - Prof. Juan J. Alonso's group at Stanford University.
 *  - Prof. Piero Colonna's group at Delft University of Technology.
 *  - Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *  - Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *  - Prof. Rafael Palacios' group at Imperial College London.
 *  - Prof. Vincent Terrapon's group at the University of Liege.
 *  - Prof. Edwin van der Weide's group at the University of Twente.
 *  - Lab. of New Concepts in Aeronautics at Tech. Institute of Aeronautics.
 *
 * Copyright 2012-2019, Francisco D. Palacios, Thomas D. Economon,
 *                      Tim Albring, and the SU2 contributors.
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../../../Common/include/mpi_structure.hpp"

#include <cmath>
#include <iostream>
#include <cstdlib>

#include "../../../Common/include/config_structure.hpp"

class CMeshElement {
protected:

  su2double Ref_Volume;       /*!< \brief Store the reference coordinates of the mesh. */
  su2double Curr_Volume;      /*!< \brief Store the current coordinates of the mesh. */

  su2double WallDistance;     /*!< \brief Store the distance of the center of the element to the wall. */

public:

  /*!
   * \brief Constructor of the class.
   * \param[in] val_coor - Values of the coordinates (initialization value).
   * \param[in] val_nDim - Number of dimensions of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  CMeshElement(void);

  /*!
   * \brief Destructor of the class.
   */
  ~CMeshElement(void);

  /*!
   * \brief Get the value of the element volume with undeformed coordinates.
   * \return Value of the element volume with reference coordinates.
   */
  inline su2double GetRef_Volume(void) { return Ref_Volume; }

  /*!
   * \brief Set the value of the element volume with undeformed coordinates.
   * \param[in] val_volume - Value of the reference volume.
   */
  inline void SetRef_Volume(su2double val_volume) { Ref_Volume = val_volume; }

  /*!
   * \brief Get the value of the element volume with deformed coordinates.
   * \return Value of the element volume with deformed coordinates.
   */
  inline su2double GetCurr_Volume(void) { return Curr_Volume; }

  /*!
   * \brief Set the value of the element distance to the nearest wall with deformed coordinates.
   * \param[in] val_volume - Value of the element distance to the nearest wall.
   */
  inline void SetCurr_Volume(su2double val_volume) { Curr_Volume = val_volume; }

  /*!
   * \brief Get the value of the element distance to the nearest wall with undeformed coordinates.
   * \return Value of the element distance to the nearest wall with reference coordinates.
   */
  inline su2double GetWallDistance(void) { return WallDistance; }

  /*!
   * \brief Set the value of the element distance to the nearest wall with undeformed coordinates.
   * \param[in] val_volume - Value of the element distance to the nearest wall.
   */
  inline void SetWallDistance(su2double val_volume) { WallDistance = val_volume; }

};