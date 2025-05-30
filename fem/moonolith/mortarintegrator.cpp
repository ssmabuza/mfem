// Copyright (c) 2010-2025, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. All Rights reserved. See files
// LICENSE and NOTICE for details. LLNL-CODE-806117.
//
// This file is part of the MFEM library. For more information and source code
// availability visit https://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the BSD-3 license. We welcome feedback and contributions, see file
// CONTRIBUTING.md for details.

#include "../../config/config.hpp"

#ifdef MFEM_USE_MOONOLITH

#include "mortarintegrator.hpp"

namespace mfem
{

void L2MortarIntegrator::AssembleElementMatrix(
   const FiniteElement &trial, const IntegrationRule &trial_ir,
   ElementTransformation &trial_Trans, const FiniteElement &test,
   const IntegrationRule &test_ir, ElementTransformation &test_Trans,
   DenseMatrix &elmat)
{
   int tr_nd = trial.GetDof();
   int te_nd = test.GetDof();
   double w;

   Vector shape, te_shape;

   elmat.SetSize(te_nd, tr_nd);
   shape.SetSize(tr_nd);
   te_shape.SetSize(te_nd);

   elmat = 0.0;
   for (int i = 0; i < test_ir.GetNPoints(); i++)
   {
      const IntegrationPoint &trial_ip = trial_ir.IntPoint(i);
      const IntegrationPoint &test_ip = test_ir.IntPoint(i);
      test_Trans.SetIntPoint(&test_ip);

      trial.CalcShape(trial_ip, shape);
      test.CalcShape(test_ip, te_shape);

      w = test_Trans.Weight() * test_ip.weight;

      te_shape *= w;
      AddMultVWt(te_shape, shape, elmat);
   }
}

BilinearFormIntegrator * L2MortarIntegrator::newBFormIntegrator() const { return new MassIntegrator(); }

void VectorL2MortarIntegrator::AssembleElementMatrix(
   const FiniteElement &trial, const IntegrationRule &trial_ir,
   ElementTransformation &trial_Trans, const FiniteElement &test,
   const IntegrationRule &test_ir, ElementTransformation &test_Trans,
   DenseMatrix &elmat)
{
   if (test.GetRangeType() == FiniteElement::SCALAR && VQ)
   {
      // assume test is scalar FE and trial is vector FE
      int dim = test.GetDim();
      int trial_dof = trial.GetDof();
      int test_dof = test.GetDof();
      double w;

      if (MQ)
         mfem_error("VectorFEMassIntegrator::AssembleElementMatrix2(...)\n"
                    "   is not implemented for tensor materials");

#ifdef MFEM_THREAD_SAFE
      DenseMatrix trial_vshape(trial_dof, dim);
      Vector shape(test_dof);
      Vector D(dim);
#else
      trial_vshape.SetSize(trial_dof, dim);
      shape.SetSize(test_dof);
      D.SetSize(dim);
#endif

      elmat.SetSize(test_dof, trial_dof);

      elmat = 0.0;
      for (int i = 0; i < test_ir.GetNPoints(); i++)
      {
         const IntegrationPoint &trial_ip = trial_ir.IntPoint(i);
         const IntegrationPoint &test_ip = test_ir.IntPoint(i);

         trial_Trans.SetIntPoint(&trial_ip);
         test_Trans.SetIntPoint(&test_ip);

         trial.CalcVShape(trial_Trans, trial_vshape);
         test.CalcShape(test_ip, shape);

         w = test_ip.weight * test_Trans.Weight();
         VQ->Eval(D, test_Trans, test_ip);
         D *= w;

         for (int d = 0; d < dim; d++)
         {
            for (int j = 0; j < test_dof; j++)
            {
               for (int k = 0; k < trial_dof; k++)
               {
                  elmat(j, k) += D[d] * shape(j) * trial_vshape(k, d);
               }
            }
         }
      }
   }
   else if (test.GetRangeType() == FiniteElement::SCALAR)
   {
      // assume test is scalar FE and trial is vector FE
      int dim = test.GetDim();
      int trial_dof = trial.GetDof();
      int test_dof = test.GetDof();
      double w;

      if (VQ || MQ)
         mfem_error("VectorFEMassIntegrator::AssembleElementMatrix2(...)\n"
                    "   is not implemented for vector/tensor permeability");

#ifdef MFEM_THREAD_SAFE
      DenseMatrix trial_vshape(trial_dof, dim);
      Vector shape(test_dof);
#else
      trial_vshape.SetSize(trial_dof, dim);
      shape.SetSize(test_dof);
#endif

      elmat.SetSize(dim * test_dof, trial_dof);

      elmat = 0.0;
      for (int i = 0; i < test_ir.GetNPoints(); i++)
      {
         const IntegrationPoint &trial_ip = trial_ir.IntPoint(i);
         const IntegrationPoint &test_ip = test_ir.IntPoint(i);

         trial_Trans.SetIntPoint(&trial_ip);
         test_Trans.SetIntPoint(&test_ip);

         trial.CalcVShape(trial_Trans, trial_vshape);
         test.CalcShape(test_ip, shape);

         w = test_ip.weight * test_Trans.Weight();

         if (Q)
         {
            w *= Q->Eval(test_Trans, test_ip);
         }

         for (int d = 0; d < dim; d++)
         {
            for (int j = 0; j < test_dof; j++)
            {
               for (int k = 0; k < trial_dof; k++)
               {
                  elmat(d * test_dof + j, k) += w * shape(j) * trial_vshape(k, d);
               }
            }
         }
      }
   }
   else
   {
      // assume both test and trial are vector FE
      int dim = test.GetDim();
      int trial_dof = trial.GetDof();
      int test_dof = test.GetDof();
      double w;

      if (VQ || MQ)
         mfem_error("VectorFEMassIntegrator::AssembleElementMatrix2(...)\n"
                    "   is not implemented for vector/tensor permeability");

#ifdef MFEM_THREAD_SAFE
      DenseMatrix trial_vshape(trial_dof, dim);
      DenseMatrix test_vshape(test_dof, dim);
#else
      trial_vshape.SetSize(trial_dof, dim);
      test_vshape.SetSize(test_dof, dim);
#endif

      elmat.SetSize(test_dof, trial_dof);

      elmat = 0.0;
      for (int i = 0; i < test_ir.GetNPoints(); i++)
      {
         const IntegrationPoint &trial_ip = trial_ir.IntPoint(i);
         const IntegrationPoint &test_ip = test_ir.IntPoint(i);

         trial_Trans.SetIntPoint(&trial_ip);
         test_Trans.SetIntPoint(&test_ip);

         trial.CalcVShape(trial_Trans, trial_vshape);
         test.CalcVShape(test_Trans, test_vshape);

         w = test_ip.weight * test_Trans.Weight();
         if (Q)
         {
            w *= Q->Eval(test_Trans, test_ip);
         }

         for (int d = 0; d < dim; d++)
         {
            for (int j = 0; j < test_dof; j++)
            {
               for (int k = 0; k < trial_dof; k++)
               {
                  elmat(j, k) += w * test_vshape(j, d) * trial_vshape(k, d);
               }
            }
         }
      }
   }
}

BilinearFormIntegrator * VectorL2MortarIntegrator::newBFormIntegrator() const { return new VectorFEMassIntegrator(); }


void LagrangeVectorL2MortarIntegrator::AssembleElementMatrix(
   const FiniteElement &trial,
   const IntegrationRule &trial_ir,
   ElementTransformation &trial_Trans,
   const FiniteElement &test,
   const IntegrationRule &test_ir,
   ElementTransformation &test_Trans,
   DenseMatrix &elmat)
{
   int tr_nd = trial.GetDof();
   int te_nd = test.GetDof();

   double norm;

   // If vdim is not set, set it to the space dimension
   vdim = (vdim == -1) ? test_Trans.GetSpaceDim() : vdim;

#ifdef MFEM_THREAD_SAFE
   Vector D;
   Vector vec;
   DenseMatrix K;
   Vector test_shape;
   Vector trial_shape;
   DenseMatrix partelmat;
   DenseMatrix mcoeff;
#endif

   elmat.SetSize(te_nd*vdim, tr_nd*vdim);
   trial_shape.SetSize(tr_nd);
   test_shape.SetSize(te_nd);
   partelmat.SetSize(te_nd, tr_nd);

   if (VQ)
   {
      vec.SetSize(vdim);
   }
   else if (MQ)
   {
      mcoeff.SetSize(vdim);
   }

   elmat = 0.0;
   for (int s = 0; s < test_ir.GetNPoints(); s++)
   {
      trial.CalcShape(trial_ir.IntPoint(s), trial_shape);
      test.CalcShape(test_ir.IntPoint(s), test_shape);

      test_Trans.SetIntPoint(&test_ir.IntPoint(s));
      norm = test_ir.IntPoint(s).weight * test_Trans.Weight();

      MultVWt(test_shape, trial_shape, partelmat);

      if (VQ)
      {
         VQ->Eval(vec, test_Trans, test_ir.IntPoint(s));
         for (int k = 0; k < vdim; k++)
         {
            elmat.AddMatrix(norm*vec(k), partelmat, te_nd*k, tr_nd*k);
         }
      }
      else if (MQ)
      {
         MQ->Eval(mcoeff, test_Trans, test_ir.IntPoint(s));
         for (int i = 0; i < vdim; i++)
            for (int j = 0; j < vdim; j++)
            {
               elmat.AddMatrix(norm*mcoeff(i,j), partelmat, te_nd*i, tr_nd*j);
            }
      }
      else
      {
         if (Q)
         {
            norm *= Q->Eval(test_Trans, test_ir.IntPoint(s));
         }
         partelmat *= norm;
         for (int k = 0; k < vdim; k++)
         {
            elmat.AddMatrix(partelmat, te_nd*k, tr_nd*k);
         }
      }
   }
}

BilinearFormIntegrator * LagrangeVectorL2MortarIntegrator::newBFormIntegrator()
const { return new VectorMassIntegrator(); }

} // namespace mfem

#endif // MFEM_USE_MOONOLITH
