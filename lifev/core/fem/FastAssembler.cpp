#include <lifev/core/fem/FastAssembler.hpp>
#include <chrono>
#include <omp.h>
using namespace std::chrono;

using namespace std;

using namespace LifeV;

//=========================================================================
// Constructor //
FastAssembler::FastAssembler( const meshPtr_Type& mesh, const commPtr_Type& comm, const ReferenceFE* refFE, const qr_Type* qr ) :
		M_mesh ( mesh ),
		M_comm ( comm ),
		M_referenceFE ( refFE ),
		M_qr ( qr ),
		M_useSUPG ( false )
{
}
//=========================================================================
// Destructor //
FastAssembler::~FastAssembler()
{
	//---------------------

	delete[] M_detJacobian;

	//---------------------

	for( int i = 0 ; i < M_numElements ; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			delete [] M_invJacobian[i][j];
		}
		delete [] M_invJacobian[i];
	}
	delete [] M_invJacobian;

	//---------------------

	for ( int i = 0; i < M_referenceFE->nbDof(); i++ )
	{
		delete [] M_phi[i];
	}

	delete [] M_phi;

	//---------------------

	for( int i = 0 ; i < M_referenceFE->nbDof() ; i++ )
	{
		for ( int j = 0; j < M_qr->nbQuadPt(); j++ )
		{
			delete [] M_dphi[i][j];
		}
		delete [] M_dphi[i];
	}
	delete [] M_dphi;

	//---------------------

	for( int i = 0 ; i < M_numElements ; i++ )
	{
		delete [] M_elements[i];
	}
	delete [] M_elements;

	//---------------------

	for( int i = 0 ; i < M_numElements ; i++ )
	{
		for ( int j = 0; j < M_referenceFE->nbDof(); j++ )
		{
			delete [] M_vals[i][j];
		}
		delete [] M_vals[i];
	}
	delete [] M_vals;

	//---------------------

	for( int i = 0 ; i < M_numElements ; i++ )
	{
		delete [] M_rows[i];
		delete [] M_cols[i];
	}
	delete [] M_rows;
	delete [] M_cols;

	//---------------------
	if ( M_useSUPG )
	{
		for ( int i_elem = 0; i_elem <  M_numElements; i_elem++ )
		{
			for ( int k = 0; k <  3; k++ )
			{
				for ( int z = 0; z <  3; z++ )
				{
					for ( int i = 0; i <  M_referenceFE->nbDof(); i++ )
					{
						delete [] M_vals_supg[i_elem][k][z][i];
					}
					delete [] M_vals_supg[i_elem][k][z];
				}
				delete [] M_vals_supg[i_elem][k];
			}
			delete [] M_vals_supg[i_elem];
		}
		delete [] M_vals_supg;
        
        for( int i = 0 ; i < M_numElements ; i++ )
        {
            delete [] M_rows_tmp[i];
            delete [] M_cols_tmp[i];
        }
        delete [] M_rows_tmp;
        delete [] M_cols_tmp;


        for( int i = 0 ; i < M_numElements ; i++ )
        {
        	for ( int j = 0; j < 3; j++ )
        	{
        		delete [] M_G[i][j];
        	}
        	delete [] M_G[i];
        	delete [] M_g[i];
        }
        delete [] M_G;
        delete [] M_g;
    }

    
}
//=========================================================================
void
FastAssembler::allocateSpace ( const int& numElements, CurrentFE* fe, const fespacePtr_Type& fespace )
{
	M_numElements = numElements;

	M_numScalarDofs = fespace->dof().numTotalDof();

	M_detJacobian = new double[ M_numElements ];

	M_invJacobian = new double** [ M_numElements ];

	//-------------------------------------------------------------------------------------------------

	M_invJacobian = new double** [ M_numElements];

	for ( int i = 0; i <  M_numElements; i++ )
	{
		M_invJacobian[i] = new double* [ 3 ];
		for ( int j = 0; j < 3 ; j++ )
		{
			M_invJacobian[i][j] = new double [ 3 ];
		}
	}

	for ( int i = 0; i < M_numElements; i++ )
	{
		fe->update( M_mesh->element (i), UPDATE_DPHI );
		M_detJacobian[i] = fe->detJacobian(0);
		for ( int j = 0; j < 3; j++ )
		{
			for ( int k = 0; k < 3; k++ )
			{
				M_invJacobian[i][j][k] = fe->tInverseJacobian(j,k,2);
			}
		}
	}

	//-------------------------------------------------------------------------------------------------

	M_phi = new double* [ M_referenceFE->nbDof() ];
	for ( int i = 0; i < M_referenceFE->nbDof(); i++ )
	{
		M_phi[i] = new double [ M_qr->nbQuadPt() ];
	}

    // PHI REF
    for (UInt q (0); q < M_qr->nbQuadPt(); ++q)
    {
        for (UInt j (0); j < M_referenceFE->nbDof(); ++j)
        {
        	M_phi[j][q] =  M_referenceFE->phi (j, M_qr->quadPointCoor (q) );
        }
    }

    //-------------------------------------------------------------------------------------------------

    M_dphi = new double** [ M_referenceFE->nbDof() ];

    for ( int i = 0; i <  M_referenceFE->nbDof(); i++ )
    {
    	M_dphi[i] = new double* [ M_qr->nbQuadPt() ];
    	for ( int j = 0; j < M_qr->nbQuadPt() ; j++ )
    	{
    		M_dphi[i][j] = new double [ 3 ];
    	}
    }

    //DPHI REF
    for (UInt q (0); q < M_qr->nbQuadPt(); ++q)
    {
    	for (UInt i (0); i < M_referenceFE->nbDof(); ++i)
    	{
    		for (UInt j (0); j < 3; ++j)
    		{
    			M_dphi[i][q][j] = M_referenceFE->dPhi (i, j, M_qr->quadPointCoor (q) );
    		}
    	}
    }

    //-------------------------------------------------------------------------------------------------

    M_elements = new double* [ M_numElements ];

    for ( int i = 0; i <  M_numElements; i++ )
    {
        M_elements[i] = new double [ M_referenceFE->nbDof() ];
    }

    for ( int i = 0; i <  M_numElements; i++ )
    {
        for ( int j = 0; j <  M_referenceFE->nbDof(); j++ )
        {
            M_elements[i][j] =     fespace->dof().localToGlobalMap (i, j);
        }
    }

    //-------------------------------------------------------------------------------------------------

    // Allocate space for M_rows, M_cols and M_vals

    M_vals = new double** [M_numElements];

    for ( int i_elem = 0; i_elem <  M_numElements; i_elem++ )
    {
    	M_vals[i_elem] = new double* [ M_referenceFE->nbDof() ];
    	for ( int i = 0; i <  M_referenceFE->nbDof(); i++ )
    	{
    		M_vals[i_elem][i] = new double [ M_referenceFE->nbDof() ];
    	}
    }

    M_rows = new int* [M_numElements];

    for ( int i_elem = 0; i_elem <  M_numElements; i_elem++ )
    {
    	M_rows[i_elem] = new int [ M_referenceFE->nbDof() ];
    }

    M_cols = new int* [M_numElements];

    for ( int i_elem = 0; i_elem <  M_numElements; i_elem++ )
    {
    	M_cols[i_elem] = new int [ M_referenceFE->nbDof() ];
    }
}
//=========================================================================
void
FastAssembler::allocateSpace_SUPG( CurrentFE* fe )
{
	M_useSUPG = true;

	M_vals_supg = new double**** [M_numElements];

	for ( int i_elem = 0; i_elem <  M_numElements; i_elem++ )
	{
		M_vals_supg[i_elem] = new double*** [ 3 ];
		for ( int k = 0; k <  3; k++ )
		{
			M_vals_supg[i_elem][k] = new double** [ 3 ];
			for ( int z = 0; z < 3; z++ )
			{
				M_vals_supg[i_elem][k][z] = new double* [ M_referenceFE->nbDof() ];
				for ( int i = 0; i <  M_referenceFE->nbDof(); i++ )
				{
					M_vals_supg[i_elem][k][z][i] = new double [ M_referenceFE->nbDof() ];
                    for ( int j = 0; j <  M_referenceFE->nbDof(); j++ )
                    {
                        M_vals_supg[i_elem][k][z][i][j] = 0.0;
                    }
                }
			}
		}
	}
    
    M_rows_tmp = new int* [M_numElements];
    
    for ( int i_elem = 0; i_elem <  M_numElements; i_elem++ )
    {
        M_rows_tmp[i_elem] = new int [ M_referenceFE->nbDof() ];
    }
    
    M_cols_tmp = new int* [M_numElements];
    
    for ( int i_elem = 0; i_elem <  M_numElements; i_elem++ )
    {
        M_cols_tmp[i_elem] = new int [ M_referenceFE->nbDof() ];
    }

    //-------------------------------------------------------------------------------------------------

    M_G = new double** [ M_numElements];
    M_g = new double* [ M_numElements];

    for ( int i = 0; i <  M_numElements; i++ )
    {
    	M_G[i] = new double* [ 3 ];
    	M_g[i] = new double [ 3 ];
    	for ( int j = 0; j < 3 ; j++ )
    	{
    		M_G[i][j] = new double [ 3 ];
    	}
    }

    //-------------------------------------------------------------------------------------------------

    for ( int i = 0; i < M_numElements; i++ )
    {
    	Real x1 = fe->cellNode(0,0); Real y1 = fe->cellNode(0,1); Real z1 = fe->cellNode(0,2);
    	Real x2 = fe->cellNode(1,0); Real y2 = fe->cellNode(1,1); Real z2 = fe->cellNode(1,2);
    	Real x3 = fe->cellNode(2,0); Real y3 = fe->cellNode(2,1); Real z3 = fe->cellNode(2,2);
    	Real x4 = fe->cellNode(3,0); Real y4 = fe->cellNode(3,1); Real z4 = fe->cellNode(3,2);

    	// Jacobian matrix
    	Real J11 = x2-x1;
    	Real J12 = x3-x1;
    	Real J13 = x4-x1;
    	Real J21 = y2-y1;
    	Real J22 = y3-y1;
    	Real J23 = y4-y1;
    	Real J31 = z2-z1;
    	Real J32 = z3-z1;
    	Real J33 = z4-z1;

    	Real detJ =  (x2-x1)*( (y3-y1) * (z4-z1) - (y4-y1) * (z3-z1) ) - (x3-x1) * ( (y2-y1)*(z4-z1) - (z2-z1)*(y4-y1) ) + (x4-x1) * ( (y2-y1)*(z3-z1) - (z2-z1)*(y3-y1) );

    	Real invJ11 = 1.0/detJ*(J22*J33-J32*J23);
    	Real invJ12 = 1.0/detJ*(J13*J32-J33*J12);
    	Real invJ13 = 1.0/detJ*(J12*J23-J22*J13);
    	Real invJ21 = 1.0/detJ*(J23*J31-J33*J21);
    	Real invJ22 = 1.0/detJ*(J11*J33-J31*J13);
    	Real invJ23 = 1.0/detJ*(J13*J21-J23*J11);
    	Real invJ31 = 1.0/detJ*(J21*J32-J31*J22);
    	Real invJ32 = 1.0/detJ*(J12*J31-J32*J11);
    	Real invJ33 = 1.0/detJ*(J11*J22-J21*J12);

    	M_G[i][0][0] = invJ11*invJ11 + invJ21*invJ21 + invJ31*invJ31;
    	M_G[i][0][1] = invJ11*invJ12 + invJ21*invJ22 + invJ31*invJ32;
    	M_G[i][0][2] = invJ11*invJ13 + invJ21*invJ23 + invJ31*invJ33;

    	M_G[i][1][0] = invJ12*invJ11 + invJ22*invJ21 + invJ32*invJ31;
    	M_G[i][1][1] = invJ12*invJ12 + invJ22*invJ22 + invJ32*invJ32;
    	M_G[i][1][2] = invJ12*invJ13 + invJ22*invJ23 + invJ32*invJ33;

    	M_G[i][2][0] = invJ13*invJ11 + invJ23*invJ21 + invJ33*invJ31;
    	M_G[i][2][1] = invJ13*invJ12 + invJ23*invJ22 + invJ33*invJ32;
    	M_G[i][2][2] = invJ13*invJ13 + invJ23*invJ23 + invJ33*invJ33;

    	M_g[i][0] = invJ11 + invJ21 + invJ31;
    	M_g[i][1] = invJ12 + invJ22 + invJ32;
    	M_g[i][2] = invJ13 + invJ23 + invJ33;
    }

}
//=========================================================================
void
FastAssembler::allocateSpace( const int& numElements, CurrentFE* fe, const fespacePtr_Type& fespace, const UInt* meshSub_elements )
{
	M_numElements = numElements;

	M_numScalarDofs = fespace->dof().numTotalDof();

	M_detJacobian = new double[ M_numElements ];

	M_invJacobian = new double** [ M_numElements ];

	//-------------------------------------------------------------------------------------------------

	M_invJacobian = new double** [ M_numElements];

	for ( int i = 0; i <  M_numElements; i++ )
	{
		M_invJacobian[i] = new double* [ 3 ];
		for ( int j = 0; j < 3 ; j++ )
		{
			M_invJacobian[i][j] = new double [ 3 ];
		}
	}

	for ( int i = 0; i < M_numElements; i++ )
	{
		fe->update( M_mesh->element (meshSub_elements[i]), UPDATE_DPHI );
		M_detJacobian[i] = fe->detJacobian(0);
		for ( int j = 0; j < 3; j++ )
		{
			for ( int k = 0; k < 3; k++ )
			{
				M_invJacobian[i][j][k] = fe->tInverseJacobian(j,k,0);
			}
		}
	}

	//-------------------------------------------------------------------------------------------------

	M_phi = new double* [ M_referenceFE->nbDof() ];
	for ( int i = 0; i < M_referenceFE->nbDof(); i++ )
	{
		M_phi[i] = new double [ M_qr->nbQuadPt() ];
	}

    // PHI REF
    for (UInt q (0); q < M_qr->nbQuadPt(); ++q)
    {
        for (UInt j (0); j < M_referenceFE->nbDof(); ++j)
        {
        	M_phi[j][q] =  M_referenceFE->phi (j, M_qr->quadPointCoor (q) );
        }
    }

    //-------------------------------------------------------------------------------------------------

    M_dphi = new double** [ M_referenceFE->nbDof() ];

    for ( int i = 0; i <  M_referenceFE->nbDof(); i++ )
    {
    	M_dphi[i] = new double* [ M_qr->nbQuadPt() ];
    	for ( int j = 0; j < M_qr->nbQuadPt() ; j++ )
    	{
    		M_dphi[i][j] = new double [ 3 ];
    	}
    }

    //DPHI REF
    for (UInt q (0); q < M_qr->nbQuadPt(); ++q)
    {
    	for (UInt i (0); i < M_referenceFE->nbDof(); ++i)
    	{
    		for (UInt j (0); j < 3; ++j)
    		{
    			M_dphi[i][q][j] = M_referenceFE->dPhi (i, j, M_qr->quadPointCoor (q) );
    		}
    	}
    }

    //-------------------------------------------------------------------------------------------------

    M_elements = new double* [ M_numElements ];

    for ( int i = 0; i <  M_numElements; i++ )
    {
        M_elements[i] = new double [ M_referenceFE->nbDof() ];
    }

    for ( int i = 0; i <  M_numElements; i++ )
    {
        for ( int j = 0; j <  M_referenceFE->nbDof(); j++ )
        {
            M_elements[i][j] =     fespace->dof().localToGlobalMap (meshSub_elements[i], j);
        }
    }

}

//=========================================================================
void
FastAssembler::assembleGradGrad_scalar( matrixPtr_Type& matrix )
{
    int ndof = M_referenceFE->nbDof();
    int NumQuadPoints = M_qr->nbQuadPt();

    double w_quad[NumQuadPoints];
    for ( int q = 0; q < NumQuadPoints ; q++ )
    {
    	w_quad[q] = M_qr->weight(q);
    }

    #pragma omp parallel firstprivate( w_quad, ndof, NumQuadPoints)
    {
        int i_el, i_elem, i_dof, q, d1, d2, i_test, i_trial;
        double integral;

        double dphi_phys[ndof][NumQuadPoints][3];

        // ELEMENTI
        #pragma omp for
        for ( i_el = 0; i_el <  M_numElements ; i_el++ )
        {
        	i_elem = i_el;

            // DOF
            for ( i_dof = 0; i_dof <  ndof; i_dof++ )
            {
                // QUAD
                for (  q = 0; q < NumQuadPoints ; q++ )
                {
                    // DIM 1
                    for ( d1 = 0; d1 < 3 ; d1++ )
                    {
                        dphi_phys[i_dof][q][d1] = 0.0;

                        // DIM 2
                        for ( d2 = 0; d2 < 3 ; d2++ )
                        {
                            dphi_phys[i_dof][q][d1] += M_invJacobian[i_elem][d1][d2] * M_dphi[i_dof][q][d2];
                        }
                    }
                }
            }

            // DOF - test
            for ( i_test = 0; i_test <  ndof; i_test++ )
            {
                M_rows[i_elem][i_test] = M_elements[i_elem][i_test];

                // DOF - trial
                for ( i_trial = 0; i_trial <  ndof; i_trial++ )
                {
                    M_cols[i_elem][i_trial] = M_elements[i_elem][i_trial];

                    integral = 0.0;
                    // QUAD
                    for ( q = 0; q < NumQuadPoints ; q++ )
                    {
                        // DIM 1
                        for ( d1 = 0; d1 < 3 ; d1++ )
                        {
                            integral += dphi_phys[i_test][q][d1] * dphi_phys[i_trial][q][d1]*w_quad[q];
                        }
                    }
                    M_vals[i_elem][i_test][i_trial] = integral * M_detJacobian[i_elem];
                }
            }
        }
    }

    for ( int k = 0; k < M_numElements; ++k )
    {
    	matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    }

}
//=========================================================================
void
FastAssembler::assembleGradGrad_vectorial( matrixPtr_Type& matrix )
{
    int ndof = M_referenceFE->nbDof();
    int NumQuadPoints = M_qr->nbQuadPt();

    double w_quad[NumQuadPoints];
    for ( int q = 0; q < NumQuadPoints ; q++ )
    {
    	w_quad[q] = M_qr->weight(q);
    }

    #pragma omp parallel firstprivate( w_quad, ndof, NumQuadPoints)
    {
        int i_el, i_elem, i_dof, q, d1, d2, i_test, i_trial;
        double integral;

        double dphi_phys[ndof][NumQuadPoints][3];

        // ELEMENTI
        #pragma omp for
        for ( i_el = 0; i_el <  M_numElements ; i_el++ )
        {
        	i_elem = i_el;

            // DOF
            for ( i_dof = 0; i_dof <  ndof; i_dof++ )
            {
                // QUAD
                for (  q = 0; q < NumQuadPoints ; q++ )
                {
                    // DIM 1
                    for ( d1 = 0; d1 < 3 ; d1++ )
                    {
                        dphi_phys[i_dof][q][d1] = 0.0;

                        // DIM 2
                        for ( d2 = 0; d2 < 3 ; d2++ )
                        {
                            dphi_phys[i_dof][q][d1] += M_invJacobian[i_elem][d1][d2] * M_dphi[i_dof][q][d2];
                        }
                    }
                }
            }

            // DOF - test
            for ( i_test = 0; i_test <  ndof; i_test++ )
            {
                M_rows[i_elem][i_test] = M_elements[i_elem][i_test];

                // DOF - trial
                for ( i_trial = 0; i_trial <  ndof; i_trial++ )
                {
                    M_cols[i_elem][i_trial] = M_elements[i_elem][i_trial];

                    integral = 0.0;
                    // QUAD
                    for ( q = 0; q < NumQuadPoints ; q++ )
                    {
                        // DIM 1
                        for ( d1 = 0; d1 < 3 ; d1++ )
                        {
                            integral += dphi_phys[i_test][q][d1] * dphi_phys[i_trial][q][d1]*w_quad[q];
                        }
                    }
                    M_vals[i_elem][i_test][i_trial] = integral * M_detJacobian[i_elem];
                }
            }
        }
    }

    for ( int k = 0; k < M_numElements; ++k )
    {
    	matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    }

    for ( UInt d1 = 1; d1 < 3 ; d1++ )
    {
    	for ( int k = 0; k < M_numElements; ++k )
    	{
    		for ( UInt i = 0; i <  ndof; i++ )
    		{
    			M_rows[k][i] += M_numScalarDofs;
    			M_cols[k][i] += M_numScalarDofs;
    		}
    		matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    	}
    }

}
//=========================================================================
void
FastAssembler::assembleMass_vectorial( matrixPtr_Type& matrix )
{
    int ndof = M_referenceFE->nbDof();
    int NumQuadPoints = M_qr->nbQuadPt();

    double w_quad[NumQuadPoints];
    for ( int q = 0; q < NumQuadPoints ; q++ )
    {
    	w_quad[q] = M_qr->weight(q);
    }

    #pragma omp parallel firstprivate( w_quad, ndof, NumQuadPoints)
    {
        int i_el, i_elem, i_dof, q, d1, d2, i_test, i_trial;
        double integral;

        double dphi_phys[ndof][NumQuadPoints][3];

        // ELEMENTI
        #pragma omp for
        for ( i_el = 0; i_el <  M_numElements ; i_el++ )
        {
        	i_elem = i_el;

            // DOF - test
            for ( i_test = 0; i_test <  ndof; i_test++ )
            {
                M_rows[i_elem][i_test] = M_elements[i_elem][i_test];

                // DOF - trial
                for ( i_trial = 0; i_trial <  ndof; i_trial++ )
                {
                    M_cols[i_elem][i_trial] = M_elements[i_elem][i_trial];

                    integral = 0.0;
                    // QUAD
                    for ( q = 0; q < NumQuadPoints ; q++ )
                    {
                    	integral += M_phi[i_test][q] * M_phi[i_trial][q]*w_quad[q];
                    }
                    M_vals[i_elem][i_test][i_trial] = integral * M_detJacobian[i_elem];
                }
            }
        }
    }

    for ( int k = 0; k < M_numElements; ++k )
    {
    	matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    }

    for ( UInt d1 = 1; d1 < 3 ; d1++ )
    {
    	for ( int k = 0; k < M_numElements; ++k )
    	{
    		for ( UInt i = 0; i <  ndof; i++ )
    		{
    			M_rows[k][i] += M_numScalarDofs;
    			M_cols[k][i] += M_numScalarDofs;
    		}
    		matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    	}
    }
}
//=========================================================================
void
FastAssembler::assembleMass_scalar( matrixPtr_Type& matrix )
{
    int ndof = M_referenceFE->nbDof();
    int NumQuadPoints = M_qr->nbQuadPt();

    double w_quad[NumQuadPoints];
    for ( int q = 0; q < NumQuadPoints ; q++ )
    {
    	w_quad[q] = M_qr->weight(q);
    }

    #pragma omp parallel firstprivate( w_quad, ndof, NumQuadPoints)
    {
        int i_el, i_elem, i_dof, q, d1, d2, i_test, i_trial;
        double integral;

        double dphi_phys[ndof][NumQuadPoints][3];

        // ELEMENTI
        #pragma omp for
        for ( i_el = 0; i_el <  M_numElements ; i_el++ )
        {
        	i_elem = i_el;

            // DOF - test
            for ( i_test = 0; i_test <  ndof; i_test++ )
            {
                M_rows[i_elem][i_test] = M_elements[i_elem][i_test];

                // DOF - trial
                for ( i_trial = 0; i_trial <  ndof; i_trial++ )
                {
                    M_cols[i_elem][i_trial] = M_elements[i_elem][i_trial];

                    integral = 0.0;
                    // QUAD
                    for ( q = 0; q < NumQuadPoints ; q++ )
                    {
                    	integral += M_phi[i_test][q] * M_phi[i_trial][q]*w_quad[q];
                    }
                    M_vals[i_elem][i_test][i_trial] = integral * M_detJacobian[i_elem];
                }
            }
        }
    }

    for ( int k = 0; k < M_numElements; ++k )
    {
    	matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    }

}
//=========================================================================
void
FastAssembler::assembleConvective( matrixPtr_Type& matrix, const vector_Type& u_h )
{
	assembleConvective( *matrix,  u_h );
}
//=========================================================================
void
FastAssembler::assembleConvective( matrix_Type& matrix, const vector_Type& u_h )
{
    int ndof = M_referenceFE->nbDof();
    int NumQuadPoints = M_qr->nbQuadPt();
    int ndof_vec = M_referenceFE->nbDof()*3;

    double w_quad[NumQuadPoints];
    for ( int q = 0; q < NumQuadPoints ; q++ )
    {
    	w_quad[q] = M_qr->weight(q);
    }

    #pragma omp parallel firstprivate( w_quad, ndof, NumQuadPoints)
    {
        int i_elem, i_dof, q, d1, d2, i_test, i_trial, e_idof;
        double integral;

        double dphi_phys[ndof][NumQuadPoints][3];

        double uhq[3][NumQuadPoints];

        // ELEMENTI,
        #pragma omp for
        for ( i_elem = 0; i_elem <  M_numElements; i_elem++ )
        {
            // DOF
            for ( i_dof = 0; i_dof <  ndof; i_dof++ )
            {
                // QUAD
                for (  q = 0; q < NumQuadPoints ; q++ )
                {
                    // DIM 1
                    for ( d1 = 0; d1 < 3 ; d1++ )
                    {
                        dphi_phys[i_dof][q][d1] = 0.0;

                        // DIM 2
                        for ( d2 = 0; d2 < 3 ; d2++ )
                        {
                            dphi_phys[i_dof][q][d1] += M_invJacobian[i_elem][d1][d2] * M_dphi[i_dof][q][d2];
                        }
                    }
                }
            }

            // QUAD
            for (  q = 0; q < NumQuadPoints ; q++ )
            {
            	for ( d1 = 0; d1 < 3 ; d1++ )
            	{
            		uhq[d1][q] = 0.0;
            		for ( i_dof = 0; i_dof <  ndof; i_dof++ )
            		{
            			e_idof =  M_elements[i_elem][i_dof] + d1*M_numScalarDofs  ;
            			uhq[d1][q] += u_h[e_idof] * M_phi[i_dof][q];
            			//printf("\n u_h[%d] = %f",  e_idof, u_h[e_idof]);
            		}
            	}
            }

            // DOF - test
            for ( i_test = 0; i_test <  ndof; i_test++ )
            {
                M_rows[i_elem][i_test] = M_elements[i_elem][i_test];

                // DOF - trial
                for ( i_trial = 0; i_trial <  ndof; i_trial++ )
                {
                    M_cols[i_elem][i_trial] = M_elements[i_elem][i_trial];

                    integral = 0.0;
                    // QUAD
                    for ( q = 0; q < NumQuadPoints ; q++ )
                    {
                        // DIM 1
                        for ( d1 = 0; d1 < 3 ; d1++ )
                        {
                            integral += uhq[d1][q] * dphi_phys[i_trial][q][d1] * M_phi[i_test][q] * w_quad[q];
                        }
                    }
                    M_vals[i_elem][i_test][i_trial] = integral *  M_detJacobian[i_elem];
                }
            }
        }
    }

    for ( int k = 0; k < M_numElements; ++k )
    {
    	matrix.matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    }

    for ( UInt d1 = 1; d1 < 3 ; d1++ )
    {
    	for ( int k = 0; k < M_numElements; ++k )
    	{
    		for ( UInt i = 0; i <  ndof; i++ )
    		{
    			M_rows[k][i] += M_numScalarDofs;
    			M_cols[k][i] += M_numScalarDofs;
    		}
    		matrix.matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
    	}
    }
}
//=========================================================================
// NAVIER STOKES MATRICES
//=========================================================================
void
FastAssembler::NS_constant_terms_00( matrixPtr_Type& matrix )
{
	int ndof = M_referenceFE->nbDof();
	int NumQuadPoints = M_qr->nbQuadPt();

	double w_quad[NumQuadPoints];
	for ( int q = 0; q < NumQuadPoints ; q++ )
	{
		w_quad[q] = M_qr->weight(q);
	}

	#pragma omp parallel firstprivate( w_quad, ndof, NumQuadPoints)
	{
		int i_el, i_elem, i_dof, q, d1, d2, i_test, i_trial;
		double integral;

		double dphi_phys[ndof][NumQuadPoints][3];

		// ELEMENTI
		#pragma omp for
		for ( i_el = 0; i_el <  M_numElements ; i_el++ )
		{
			i_elem = i_el;

			// DOF
			for ( i_dof = 0; i_dof <  ndof; i_dof++ )
			{
				// QUAD
				for (  q = 0; q < NumQuadPoints ; q++ )
				{
					// DIM 1
					for ( d1 = 0; d1 < 3 ; d1++ )
					{
						dphi_phys[i_dof][q][d1] = 0.0;

						// DIM 2
						for ( d2 = 0; d2 < 3 ; d2++ )
						{
							dphi_phys[i_dof][q][d1] += M_invJacobian[i_elem][d1][d2] * M_dphi[i_dof][q][d2];
						}
					}
				}
			}

			// DOF - test
			for ( i_test = 0; i_test <  ndof; i_test++ )
			{
				M_rows[i_elem][i_test] = M_elements[i_elem][i_test];

				// DOF - trial
				for ( i_trial = 0; i_trial <  ndof; i_trial++ )
				{
					M_cols[i_elem][i_trial] = M_elements[i_elem][i_trial];

					integral = 0.0;
					// QUAD
					for ( q = 0; q < NumQuadPoints ; q++ )
					{
						integral += M_phi[i_test][q] * M_phi[i_trial][q]*w_quad[q]; // MASS
						// DIM 1
						for ( d1 = 0; d1 < 3 ; d1++ )
						{
							integral += dphi_phys[i_test][q][d1] * dphi_phys[i_trial][q][d1]*w_quad[q]; // STIFFNESS
						}
					}
					M_vals[i_elem][i_test][i_trial] = integral * M_detJacobian[i_elem];
				}
			}
		}
	}

	for ( int k = 0; k < M_numElements; ++k )
	{
		matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
	}

	for ( UInt d1 = 1; d1 < 3 ; d1++ )
	{
		for ( int k = 0; k < M_numElements; ++k )
		{
			for ( UInt i = 0; i <  ndof; i++ )
			{
				M_rows[k][i] += M_numScalarDofs;
				M_cols[k][i] += M_numScalarDofs;
			}
			matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows[k], ndof, M_cols[k], M_vals[k], Epetra_FECrsMatrix::ROW_MAJOR);
		}
	}
}
//=========================================================================
void
FastAssembler::assemble_SUPG_block00( matrixPtr_Type& matrix, const vector_Type& u_h )
{
	int ndof = M_referenceFE->nbDof();
	int NumQuadPoints = M_qr->nbQuadPt();
	int ndof_vec = M_referenceFE->nbDof()*3;

	double w_quad[NumQuadPoints];
	for ( int q = 0; q < NumQuadPoints ; q++ )
	{
		w_quad[q] = M_qr->weight(q);
	}

	#pragma omp parallel firstprivate( w_quad, ndof, NumQuadPoints)
	{
		int i_elem, i_dof, q, d1, d2, i_test, i_trial, e_idof;
		double integral, integral_test, integral_trial;

		double dphi_phys[ndof][NumQuadPoints][3];

		double uhq[3][NumQuadPoints];

		// ELEMENTI,
		#pragma omp for
		for ( i_elem = 0; i_elem <  M_numElements; i_elem++ )
		{
			// DOF
			for ( i_dof = 0; i_dof <  ndof; i_dof++ )
			{
				// QUAD
				for (  q = 0; q < NumQuadPoints ; q++ )
				{
					// DIM 1
					for ( d1 = 0; d1 < 3 ; d1++ )
					{
						dphi_phys[i_dof][q][d1] = 0.0;

						// DIM 2
						for ( d2 = 0; d2 < 3 ; d2++ )
						{
							dphi_phys[i_dof][q][d1] += M_invJacobian[i_elem][d1][d2] * M_dphi[i_dof][q][d2];
						}
					}
				}
			}
            
			// QUAD
			for (  q = 0; q < NumQuadPoints ; q++ )
			{
				for ( d1 = 0; d1 < 3 ; d1++ )
				{
					uhq[d1][q] = 0.0;
					for ( i_dof = 0; i_dof <  ndof; i_dof++ )
					{
						e_idof =  M_elements[i_elem][i_dof] + d1*M_numScalarDofs  ;
						uhq[d1][q] += u_h[e_idof] * M_phi[i_dof][q];
					}
				}
			}

			// DOF - test
			for ( i_test = 0; i_test <  ndof; i_test++ )
			{
				M_rows[i_elem][i_test] = M_elements[i_elem][i_test];
                
				// DOF - trial
				for ( i_trial = 0; i_trial <  ndof; i_trial++ )
				{
					M_cols[i_elem][i_trial] = M_elements[i_elem][i_trial];

					integral = 0.0;
					// QUAD
					for ( q = 0; q < NumQuadPoints ; q++ )
					{
						integral_test = 0;
						integral_trial = 0;

						// DIM 1
						for ( d1 = 0; d1 < 3 ; d1++ )
						{
							integral_test += uhq[d1][q] * dphi_phys[i_test][q][d1];   // w grad(phi_i)
							integral_trial += uhq[d1][q] * dphi_phys[i_trial][q][d1]; // w grad(phi_j) terzo indice dphi_phys e' derivata in x,y,z
						}
						integral += (integral_test * integral_trial + integral_test * M_phi[i_trial][q] ) * w_quad[q];
                        // above, the term "integral_test * M_phi[i_trial][q]" is the one which comes from (w grad(phi_i), phi_j)
					}
                    M_vals_supg[i_elem][0][0][i_test][i_trial] = integral *  M_detJacobian[i_elem]; // (w grad(phi_i), w grad(phi_j) )
                    M_vals_supg[i_elem][1][1][i_test][i_trial] = integral *  M_detJacobian[i_elem];
                    M_vals_supg[i_elem][2][2][i_test][i_trial] = integral *  M_detJacobian[i_elem];

					for ( d1 = 0; d1 < 3; d1++ )
					{
						for ( d2 = 0; d2 < 3; d2++ )
						{
							integral = 0.0;
							// QUAD
							for ( q = 0; q < NumQuadPoints ; q++ )
							{
								integral += dphi_phys[i_test][q][d1] * dphi_phys[i_trial][q][d2] * w_quad[q]; // div(phi_i) * div(phi_j)
							}
                            M_vals_supg[i_elem][d1][d2][i_test][i_trial] += integral *  M_detJacobian[i_elem];
						}
					}

				}
			}
		}
	}

    for ( UInt d1 = 0; d1 < 3 ; d1++ ) // row index
	{
		for ( UInt d2 = 0; d2 < 3 ; d2++ ) // column
		{
			for ( int k = 0; k < M_numElements; ++k )
			{
				for ( UInt i = 0; i <  ndof; i++ )
				{
					M_rows_tmp[k][i] = M_rows[k][i] + d1 * M_numScalarDofs;
					M_cols_tmp[k][i] = M_cols[k][i] + d2 * M_numScalarDofs;
				}
				matrix->matrixPtr()->InsertGlobalValues ( ndof, M_rows_tmp[k], ndof, M_cols_tmp[k], M_vals_supg[k][d1][d2], Epetra_FECrsMatrix::ROW_MAJOR);
			}
        }
	}
}


void
FastAssembler::assemble_SUPG_block11( matrixPtr_Type& matrix )
{
    assembleGradGrad_scalar ( matrix );
}




