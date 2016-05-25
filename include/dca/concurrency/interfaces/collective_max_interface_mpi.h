// Copyright (C) 2009-2016 ETH Zurich
// Copyright (C) 2007?-2016 Center for Nanophase Materials Sciences, ORNL
// All rights reserved.
//
// See LICENSE.txt for terms of usage.
// See CITATION.txt for citation guidelines if you use this code for scientific publications.
//
// Author: Peter Staar (peter.w.j.staar@gmail.com)
//
// Description

#ifndef DCA_CONCURRENCY_COLLECTIVE_MAX_INTERFACE_MPI_H
#define DCA_CONCURRENCY_COLLECTIVE_MAX_INTERFACE_MPI_H

#include "dca/concurrency/interfaces/collective_max_interface.h"
#include <mpi.h>
#include "dca/concurrency/interfaces/type_map_interface_mpi.h"
#include "dca/concurrency/interfaces/processor_grouping_interface_mpi.h"

namespace dca {
namespace concurrency {
// dca::concurrency::

template <>
class collective_max_interface<MPI_LIBRARY> {
public:
  collective_max_interface(processor_grouping<MPI_LIBRARY>& grouping_ref);
  ~collective_max_interface();

  template <typename scalar_type>
  void max(scalar_type& value);

private:
  processor_grouping<MPI_LIBRARY>& grouping;
};

collective_max_interface<MPI_LIBRARY>::collective_max_interface(
    processor_grouping<MPI_LIBRARY>& grouping_ref)
    : grouping(grouping_ref) {}

collective_max_interface<MPI_LIBRARY>::~collective_max_interface() {}

template <typename scalar_type>
void collective_max_interface<MPI_LIBRARY>::max(scalar_type& value) {
  scalar_type result;

  MPI_Allreduce(&value, &result, type_map_interface<MPI_LIBRARY, scalar_type>::factor(),
                type_map_interface<MPI_LIBRARY, scalar_type>::value(), MPI_MAX, grouping.get());

  value = result;
}

}  // concurrency
}  // dca

#endif  // DCA_CONCURRENCY_COLLECTIVE_MAX_INTERFACE_MPI_H
