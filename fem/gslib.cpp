﻿// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443211. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the MFEM library. For more information and source code
// availability see http://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#include "gslib.hpp"

#ifdef MFEM_USE_GSLIB

namespace mfem
{

IntegrationRules IntRulesGLL(0, Quadrature1D::GaussLobatto);

//  Constructor - sets up the GLL mesh and saves it
#ifdef MFEM_USE_MPI
findpts_gslib::findpts_gslib (MPI_Comm _comm)
#else
findpts_gslib::findpts_gslib ()
#endif
{
#ifdef MFEM_USE_MPI
   comm_init(&this->cc,_comm);
#else
   comm_init(&this->cc,0);
#endif
}

#ifdef MFEM_USE_MPI
void findpts_gslib::gslib_findpts_setup(ParFiniteElementSpace *pfes, ParMesh *pmesh, int QORDER,double bb_t, double newt_tol, int npt_max)
#else
void findpts_gslib::gslib_findpts_setup(FiniteElementSpace *pfes, Mesh *pmesh, int QORDER,double bb_t, double newt_tol, int npt_max)
#endif
{
   const int geom_type = pfes->GetFE(0)->GetGeomType();
   this->ir = IntRulesGLL.Get(geom_type, QORDER);
   dim = pmesh->Dimension();
   nel = pmesh->GetNE();
   qo = sqrt(ir.GetNPoints());
   if (dim==3) qo = cbrt(ir.GetNPoints());
   int nsp = pow(qo,dim);
   msz = nel*nsp;
   this->gllmesh.SetSize(dim*msz);

   int npt = nel*nsp;
#ifdef MFEM_USE_MPI
   ParGridFunction nodes(pfes);
#else
   GridFunction nodes(pfes);
#endif
   pmesh->GetNodes(nodes);

   int np = 0;
   DenseMatrix gllvals;
   DenseMatrix tr;
   for (int i = 0; i < nel; i++)
   {
      nodes.GetVectorValues(i,this->ir,gllvals,tr);
      for (int j = 0; j < nsp; j++)
      {
         for (int k = 0; k < dim; k++)
         {
            gllmesh[k*npt+np] = *(gllvals.GetData()+k+j*dim);
         }
         np = np+1;
      }
   }

   const int NE = nel, NR = qo;
   int ntot = pow(NR,dim)*NE;
   if (dim==2)
   {
      unsigned nr[2] = {NR,NR};
      unsigned mr[2] = {2*NR,2*NR};
      double *const elx[2] = {&gllmesh[0],&gllmesh[ntot]};
      this->fda=findpts_setup_2(&this->cc,elx,nr,NE,mr,bb_t,ntot,ntot,npt_max,newt_tol);
   }
   else
   {
      unsigned nr[3] = {NR,NR,NR};
      unsigned mr[3] = {2*NR,2*NR,2*NR};
      double *const elx[3] = {&gllmesh[0],&gllmesh[ntot],&gllmesh[2*ntot]};
      this->fdb=findpts_setup_3(&this->cc,elx,nr,NE,mr,bb_t,ntot,ntot,npt_max,newt_tol);
   }
}

#ifdef MFEM_USE_MPI
void findpts_gslib::gslib_findpts_setup(ParFiniteElementSpace *pfes, ParMesh *pmesh, int QORDER)
#else
void findpts_gslib::gslib_findpts_setup(FiniteElementSpace *pfes, Mesh *pmesh, int QORDER)
#endif
{
   gslib_findpts_setup(pfes,pmesh,QORDER,0.05,1.e-12,256);
}

// findpts - given x,y,z for "nxyz" points, it returns, e,p,r,s,t info
void findpts_gslib::gslib_findpts(Array<uint> *pcode,Array<uint> *pproc,Array<uint> *pel,Vector *pr,Vector *pd,Vector *xp,Vector *yp, Vector *zp, int nxyz)
{
   if (dim==2)
   {
      const double *xv_base[2];
      xv_base[0]=xp->GetData(), xv_base[1]=yp->GetData();
      unsigned xv_stride[2];
      xv_stride[0] = sizeof(double),xv_stride[1] = sizeof(double);
      findpts_2(
               pcode->GetData(),sizeof(uint),
               pproc->GetData(),sizeof(uint),
               pel->GetData(),sizeof(uint),
               pr->GetData(),sizeof(double)*dim,
               pd->GetData(),sizeof(double),
               xv_base,     xv_stride,
               nxyz,this->fda);
   }
   else
   {
      const double *xv_base[3];
      xv_base[0]=xp->GetData(), xv_base[1]=yp->GetData();xv_base[2]=zp->GetData();
      unsigned xv_stride[3];
      xv_stride[0] = sizeof(double),xv_stride[1] = sizeof(double),xv_stride[2] = sizeof(double);
      findpts_3(
               pcode->GetData(),sizeof(uint),
               pproc->GetData(),sizeof(uint),
               pel->GetData(),sizeof(uint),
               pr->GetData(),sizeof(double)*dim,
               pd->GetData(),sizeof(double),
               xv_base,     xv_stride,
               nxyz,this->fdb);
   }
}

void findpts_gslib::gslib_findpts(Array<uint> *pcode,Array<uint> *pproc,Array<uint> *pel,Vector *pr,Vector *pd,Vector *xyzp, int nxyz)
{
   if (this->dim==2)
   {
      const double *xv_base[2];
      xv_base[0]=xyzp->GetData(), xv_base[1]=xyzp->GetData()+nxyz;
      unsigned xv_stride[2];
      xv_stride[0] = sizeof(double),xv_stride[1] = sizeof(double);
      findpts_2(
               pcode->GetData(),sizeof(uint),
               pproc->GetData(),sizeof(uint),
               pel->GetData(),sizeof(uint),
               pr->GetData(),sizeof(double)*dim,
               pd->GetData(),sizeof(double),
               xv_base,     xv_stride,
               nxyz,this->fda);
   }
   else
   {
      const double *xv_base[3];
      xv_base[0]=xyzp->GetData(), xv_base[1]=xyzp->GetData()+nxyz;xv_base[2]=xyzp->GetData()+2*nxyz;
      unsigned xv_stride[3];
      xv_stride[0] = sizeof(double),xv_stride[1] = sizeof(double),xv_stride[2] = sizeof(double);
      findpts_3(
               pcode->GetData(),sizeof(uint),
               pproc->GetData(),sizeof(uint),
               pel->GetData(),sizeof(uint),
               pr->GetData(),sizeof(double)*dim,
               pd->GetData(),sizeof(double),
               xv_base,     xv_stride,
               nxyz,this->fdb);
   }
}

void findpts_gslib::gslib_findpts_eval(
      Vector *fieldout, Array<uint> *pcode, Array<uint>  *pproc, Array<uint>  *pel, Vector *pr,
      Vector *fieldin, int nxyz)
{
   if (dim==2)
   {
      findpts_eval_2(fieldout->GetData(),sizeof(double),
                     pcode->GetData(),sizeof(uint),
                     pproc->GetData(),sizeof(uint),
                     pel->GetData(),sizeof(uint),
                     pr->GetData(),sizeof(double)*dim,
                     nxyz,fieldin->GetData(),this->fda);
   }
   else
   {
      findpts_eval_3(fieldout->GetData(),sizeof(double),
                     pcode->GetData(),sizeof(uint),
                     pproc->GetData(),sizeof(uint),
                     pel->GetData(),sizeof(uint),
                     pr->GetData(),sizeof(double)*dim,
                     nxyz,fieldin->GetData(),this->fdb);
   }
}

#ifdef MFEM_USE_MPI
void findpts_gslib::gslib_findpts_eval(
            Vector *fieldout, Array<uint> *pcode, Array<uint>  *pproc, Array<uint>  *pel, Vector *pr,
            ParGridFunction *fieldin, int nxyz)
#else
void findpts_gslib::gslib_findpts_eval(
            Vector *fieldout, Array<uint> *pcode, Array<uint>  *pproc, Array<uint>  *pel, Vector *pr,
            GridFunction *fieldin, int nxyz)
#endif
{
// convert gridfunction to double field
   Vector fin(msz);
   int nsp = pow(qo,dim);
   int np = 0;
   for (int i = 0; i < nel; i++)
   {
      for (int j = 0; j < nsp; j++)
      {
        const IntegrationPoint &ip = this->ir.IntPoint(j);
        *(fin.GetData()+np) = fieldin->GetValue(i, ip);
        np = np+1;
      }
   }

   gslib_findpts_eval(fieldout,pcode,pproc,pel,pr,&fin,nxyz);
}

#ifdef MFEM_USE_MPI
void findpts_gslib::gf2vec(ParGridFunction *fieldin, Vector *fieldout)
#else
void findpts_gslib::gf2vec(GridFunction *fieldin, Vector *fieldout)
#endif
{
   int nsp = pow(qo,dim);
   int np = 0;
   for (int i = 0; i < nel; i++)
   {
      for (int j = 0; j < nsp; j++)
      {
        const IntegrationPoint &ip = this->ir.IntPoint(j);
        *(fieldout->GetData()+np) = fieldin->GetValue(i, ip);
        np = np+1;
      }
   }
}

void findpts_gslib::gslib_findpts_free ()
{
   if (dim==2)
   {
      findpts_free_2(this->fda);
   }
   else
   {
      findpts_free_3(this->fdb);
   }
}

} // namespace mfem

#endif // MFEM_USE_GSLIB