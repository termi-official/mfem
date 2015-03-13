// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443211. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the MFEM library. For more information and source code
// availability see http://mfem.googlecode.com.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#include "_hypre_parcsr_mv.h"

namespace mfem
{
namespace internal
{

/** Parallel essential BC elimination, adapted from
    hypre_ParCSRMatrixEliminateRowsCols */
HYPRE_Int hypre_ParCSRMatrixEliminateBC(hypre_ParCSRMatrix *A,
                                        HYPRE_Int nrows_to_eliminate,
                                        HYPRE_Int *rows_to_eliminate);

} } // namespace mfem::internal
