//@HEADER
/*
*******************************************************************************

    Copyright (C) 2004, 2005, 2007 EPFL, Politecnico di Milano, INRIA
    Copyright (C) 2010 EPFL, Politecnico di Milano, Emory University

    This file is part of LifeV.

    LifeV is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LifeV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with LifeV.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************
*/
//@HEADER

/*!
 *  @file
 *  @brief This file contains an abstract class to implement different kinds of materials for structural dynamic problems (St. Venant-Kirchhoff, Neo-Hookean and Exponential materials right now )
 *
 *  @version 1.0
 *  @date 01-01-2010
 *  @author Paolo Tricerri
 *  @author Gianmarco Mengaldo
 *  @maintainer  Paolo Tricerri <paolo.tricerri@epfl.ch>
 *  @contributor  Gianmarco Mengaldo <gianmarco.mengaldo@gmail.com>
 */

#ifndef _STRUCTURALISOTROPICCONSTITUTIVELAW_H_
#define _STRUCTURALISOTROPICCONSTITUTIVELAW_H_ 1

#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <Epetra_Vector.h>
#include <EpetraExt_MatrixMatrix.h>
#include <Epetra_SerialDenseMatrix.h>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <lifev/core/array/MatrixElemental.hpp>
#include <lifev/core/array/VectorElemental.hpp>
#include <lifev/core/array/MatrixEpetra.hpp>
#include <lifev/core/array/VectorEpetra.hpp>

#include <lifev/core/fem/Assembly.hpp>
#include <lifev/core/fem/AssemblyElemental.hpp>
#include <lifev/structure/fem/AssemblyElementalStructure.hpp>
#include <lifev/structure/fem/ExpressionDefinitions.hpp>
#include <lifev/core/fem/FESpace.hpp>

#include <lifev/core/LifeV.hpp>
#include <lifev/core/util/Displayer.hpp>
#include <lifev/core/util/Factory.hpp>
#include <lifev/core/util/FactorySingleton.hpp>

#include <lifev/structure/solver/StructuralConstitutiveLawData.hpp>

//ET include for assemblings
#include <lifev/eta/fem/ETFESpace.hpp>
#include <lifev/eta/expression/Integrate.hpp>

namespace LifeV
{
/*!
  \class StructuralConstitutiveLaw
  \brief
  This class is an abstract class to define different type of models for the arterial wall.
  This class has just pure virtual methods. They are implemented in the specific class for one material
*/

template <typename MeshType>
class StructuralIsotropicConstitutiveLaw
{
public:

    //!@name Type definitions
    //@{
    typedef StructuralConstitutiveLawData          data_Type;

    typedef MatrixEpetra<Real>            matrix_Type;
    typedef std::shared_ptr<matrix_Type>         matrixPtr_Type;
    typedef VectorEpetra           vector_Type;
    typedef std::shared_ptr<vector_Type>         vectorPtr_Type;

    typedef typename std::shared_ptr<data_Type>  dataPtr_Type;
    typedef typename std::shared_ptr<const Displayer>    displayerPtr_Type;

    typedef FactorySingleton<Factory<StructuralIsotropicConstitutiveLaw<MeshType>, std::string> >  StructureIsotropicMaterialFactory;

    typedef std::vector< typename MeshType::element_Type* > vectorVolumes_Type;

    typedef std::map< UInt, vectorVolumes_Type>           mapMarkerVolumes_Type;
    typedef std::shared_ptr<mapMarkerVolumes_Type>      mapMarkerVolumesPtr_Type;

    typedef std::vector<UInt>                             vectorIndexes_Type;
    typedef std::map< UInt, vectorIndexes_Type>           mapMarkerIndexes_Type;
    typedef std::shared_ptr<mapMarkerIndexes_Type>      mapMarkerIndexesPtr_Type;


    typedef ETFESpace<MeshType, MapEpetra, 3, 3 >         ETFESpace_Type;
    typedef std::shared_ptr<ETFESpace_Type>             ETFESpacePtr_Type;

    typedef FESpace< MeshType, MapEpetra >                FESpace_Type;
    typedef std::shared_ptr<FESpace_Type>               FESpacePtr_Type;

    //Vector for vector parameters
    typedef std::vector<std::vector<Real> >           vectorsParameters_Type;
    typedef std::shared_ptr<vectorsParameters_Type> vectorsParametersPtr_Type;


    // Typedefs for tensors
    typedef ExpressionDefinitions::deformationGradient_Type     tensorF_Type;
    typedef ExpressionDefinitions::determinantTensorF_Type      determinantF_Type;
    typedef ExpressionDefinitions::rightCauchyGreenTensor_Type  tensorC_Type;
    typedef ExpressionDefinitions::minusTransposedTensor_Type   minusT_Type;
    typedef ExpressionDefinitions::traceTensor_Type             traceTensor_Type;
    //@}



    //! @name Constructor &  Deconstructor
    //@{

    StructuralIsotropicConstitutiveLaw();

    virtual ~StructuralIsotropicConstitutiveLaw() {}

    //@}



    //!@name Methods
    //@{

    //! Setup the created object of the class StructuralIsotropicConstitutiveLaw
    /*!
      \param dFespace: the FiniteElement Space
      \param monolithicMap: the MapEpetra
      \param offset: the offset parameter used assembling the matrices
    */
    virtual void setup ( const FESpacePtr_Type& dFESpace,
                         const ETFESpacePtr_Type& ETFESpace,
                         const std::shared_ptr<const MapEpetra>&   monolithicMap,
                         const UInt offset, const dataPtr_Type& dataMaterial ) = 0;


    //! Computes the linear part of the stiffness matrix StructuralSolver::buildSystem
    /*!
      \param dataMaterial the class with Material properties data
    */
    virtual  void computeLinearStiff ( dataPtr_Type& dataMaterial,
                                       const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
                                       const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/ ) = 0;

    //! Updates the Jacobian matrix in StructuralSolver::updateJacobian
    /*!
      \param disp: solution at the k-th iteration of NonLinearRichardson Method
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get the
                           material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */
    virtual  void updateJacobianMatrix ( const vector_Type& disp, const dataPtr_Type& dataMaterial,
                                         const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                         const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                         const displayerPtr_Type& displayer ) = 0;

    //! Computes the new Stiffness matrix in StructuralSolver given a certain displacement field.
    //! This function is used both in StructuralSolver::evalResidual and in
    //! StructuralSolver::updateSystem since the matrix is the expression of the matrix is the same.
    //!This is virtual and not pure virtual since in the linear St. Venant-Kirchhoff law it is not needed.
    /*!
      \param sol:  the solution vector
      \param factor: scaling factor used in FSI
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get the
                           material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */
    virtual  void computeStiffness ( const vector_Type& sol, Real factor, const dataPtr_Type& dataMaterial,
                                     const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                     const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                     const displayerPtr_Type& displayer ) = 0;


    //! Computes the deformation Gradient F, the cofactor of F Cof(F),
    //! the determinant of F J = det(F), the trace of C Tr(C).
    /*!
      \param dk_loc: local displacement vector
    */
    virtual  void computeKinematicsVariables ( const VectorElemental& dk_loc ) = 0;


    //! Output of the class
    /*!
       \param fileNamelinearStiff the filename where to apply the spy method for the linear part of the Stiffness matrix
       \param fileNameStiff the filename where to apply the spy method for the Stiffness matrix
    */
    virtual void showMe ( std::string const& fileNameStiff, std::string const& fileNameJacobian ) = 0;


    //! Compute the First Piola Kirchhoff Tensor
    /*!
       \param firstPiola Epetra_SerialDenseMatrix that has to be filled
       \param tensorF Epetra_SerialDenseMatrix the deformation gradient
       \param cofactorF Epetra_SerialDenseMatrix cofactor of F
       \param invariants std::vector with the invariants of C and the detF
       \param material UInt number to get the material parameteres form the VenantElasticData class
    */
    virtual void computeLocalFirstPiolaKirchhoffTensor ( Epetra_SerialDenseMatrix& firstPiola,
                                                         const Epetra_SerialDenseMatrix& tensorF,
                                                         const Epetra_SerialDenseMatrix& cofactorF,
                                                         const std::vector<Real>& invariants,
                                                         const UInt material) = 0;

    //! Compute the First Piola Kirchhoff Tensor
    /*!
       \param disp the displacement field from which we compute the fisrt piola-Kirchhoff tensor
       \param sigma_1 the first column of the Cauchy stress tensor
       \param sigma_2 the second column of the Cauchy stress tensor
       \param sigma_3 the third column of the Cauchy stress tensor
    */
    virtual void computeCauchyStressTensor ( const vectorPtr_Type disp,
					     const QuadratureRule& evalQuad,
					     vectorPtr_Type sigma_1,
					     vectorPtr_Type sigma_2,
					     vectorPtr_Type sigma_3) = 0;


    //! @name Set Methods
    //@{

    //No set Methods

    //@}


    //! @name Get Methods
    //@{

    //! Getters
    //! Get the Epetramap
    MapEpetra   const& map()     const
    {
        return *M_localMap;
    }

    //! Get the FESpace object
    FESpace_Type& dFESpace()
    {
        return M_dispFESpace;
    }

    //! Get the Stiffness matrix
    matrixPtr_Type const jacobian()    const
    {
        return M_jacobian;
    }

    //! Get the Stiffness matrix
    virtual matrixPtr_Type const stiffMatrix() const = 0;

    //! Get the Stiffness matrix
    virtual vectorPtr_Type const stiffVector() const = 0;

    virtual void apply ( const vector_Type& sol, vector_Type& res,
                         const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                         const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                         const displayerPtr_Type displayer) = 0;

    //@}

protected:


    //! construct the vectors for the parameters
    /*!
      \param VOID
      \return VOID
    */
    virtual void setupVectorsParameters ( void ) = 0;

    //!Protected Members
    dataPtr_Type                                   M_dataMaterial;

    FESpacePtr_Type                                M_dispFESpace;

    ETFESpacePtr_Type                              M_dispETFESpace;

    std::shared_ptr<const MapEpetra>             M_localMap;

    //! Matrix jacobian
    matrixPtr_Type                                 M_jacobian;

    //! The Offset parameter
    UInt                                           M_offset;

    //! Map between markers and volumes on the mesh
    vectorsParametersPtr_Type                      M_vectorsParameters;
};

//=====================================
// Constructor
//=====================================

template <typename MeshType>
StructuralIsotropicConstitutiveLaw<MeshType>::StructuralIsotropicConstitutiveLaw( ) :
    M_dataMaterial               ( ),
    M_dispFESpace                ( ),
    M_dispETFESpace              ( ),
    M_localMap                   ( ),
    M_jacobian                   ( ),
    M_offset                     ( 0 ),
    M_vectorsParameters          ( )
{
    //    std::cout << "I am in the constructor of StructuralIsotropicConstitutiveLaw" << std::endl;
}

}
#endif /*_STRUCTURALISOTROPICCONSTITUTIVELAW_H_*/
