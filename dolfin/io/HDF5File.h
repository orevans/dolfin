// Copyright (C) 2012 Chris N. Richardson
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
//
// Modified by Garth N. Wells, 2012
//
// First added:  2012-05-22
// Last changed: 2013-06-03

#ifndef __DOLFIN_HDF5FILE_H
#define __DOLFIN_HDF5FILE_H

#ifdef HAS_HDF5

#include <string>
#include <utility>
#include <vector>

#include <boost/multi_array.hpp>

#include "dolfin/common/Timer.h"
#include "dolfin/common/Variable.h"
#include "dolfin/mesh/Mesh.h"

#include "HDF5Interface.h"

namespace dolfin
{

  class Function;
  class GenericVector;
  class LocalMeshData;
  class GenericDofMap;

  class HDF5File : public Variable
  {
  public:

    /// Constructor. file_mode should "a" (append), "w" (write) ot "r"
    /// (read).
    HDF5File(const std::string filename, const std::string file_mode,
             bool use_mpiio=true);

    /// Destructor
    ~HDF5File();

    /// Write Vector to file in a format suitable for re-reading
    void write(const GenericVector& x, const std::string name);

    /// Write Mesh to file in a format suitable for re-reading
    void write(const Mesh& mesh, const std::string name);

    /// Write Mesh of given cell dimension to file
    /// in a format suitable for re-reading
    void write(const Mesh& mesh, const std::size_t cell_dim,
               const std::string name);

    /// Write Function to file
    /// in a format suitable for re-reading
    void write(const Function& u, const std::string name);
    void read(Function& u, const std::string name);

    /// Write MeshFunction to file
    /// in a format suitable for re-reading
    void write(const MeshFunction<std::size_t>& meshfunction,
               const std::string name);

    /// Write MeshFunction to file
    /// in a format suitable for re-reading
    void write(const MeshFunction<int>& meshfunction, const std::string name);

    /// Write MeshFunction to file
    /// in a format suitable for re-reading
    void write(const MeshFunction<double>& meshfunction,
               const std::string name);

    /// Read vector from file
    void read(GenericVector& x, const std::string dataset_name,
              const bool use_partition_from_file=true);

    /// Read Mesh from file
    void read(Mesh& mesh, const std::string name);

    /// Read MeshFunction from file
    void read(MeshFunction<std::size_t>& meshfunction, const std::string name);

    /// Read MeshFunction from file
    void read(MeshFunction<int>& meshfunction, const std::string name);

    /// Read MeshFunction from file
    void read(MeshFunction<double>& meshfunction, const std::string name);

    /// Check if dataset exists in HDF5 file
    bool has_dataset(const std::string dataset_name) const;

    /// Flush buffered I/O to disk
    void flush();

  private:

    // Friend
    friend class XDMFFile;
    friend class TimeSeriesHDF5;

    // Get cell owners for a set of cells. 
    // Returns (process, local index) pairs
    std::vector<std::pair<std::size_t, std::size_t> >
      cell_owners(const Mesh&mesh, const std::vector<std::size_t> cells);
 
    // Generate two vectors, in the range of "vector_range" of the global DOFs.
    // global_cells is a list of cells which point to the DOF (non-unique)
    // and remote_local_dofi is the pertinent local_dof of the cell.
    // input_cells is a list of cells held on this process, and
    // input_cell_dofs/x_cell_dofs list their local_dofs.
    
    void map_gdof_to_cell(const std::vector<std::size_t>& input_cells, 
                          const std::vector<dolfin::la_index>& input_cell_dofs,
                          const std::vector<std::size_t>& x_cell_dofs,
                          const std::pair<dolfin::la_index, dolfin::la_index> 
                          vector_range,
                          std::vector<std::size_t>& global_cells,
                          std::vector<std::size_t>& remote_local_dofi);

    // Given the cell dof index specified 
    // as (process, local_cell_index, local_cell_dof_index)
    // get the global_dof index from that location, and return it for all 
    // DOFs in the range of "vector_range"
    void get_global_dof(
         const std::vector<std::pair<std::size_t, std::size_t> >& cell_ownership, 
         const std::vector<std::size_t>& remote_local_dofi,
         const std::pair<std::size_t, std::size_t> vector_range,
         const GenericDofMap& dofmap,
         std::vector<dolfin::la_index>& global_dof);

    // Read a mesh and repartition (if running in parallel)
    void read_mesh_repartition(Mesh &input_mesh,
                               const std::string coordinates_name,
                               const std::string topology_name);

    // Convert LocalMeshData into a Mesh, when running serially
    void build_local_mesh(Mesh &mesh, const LocalMeshData& mesh_data) const;

    // Get description of cells to be written to file
    const std::string cell_type(const std::size_t cell_dim, const Mesh& mesh);

    // Write a MeshFunction to file
    template <typename T>
    void write_mesh_function(const MeshFunction<T>& meshfunction,
                             const std::string name);

    // Read a MeshFunction from file
    template <typename T>
    void read_mesh_function(MeshFunction<T>& meshfunction,
                            const std::string name);

    // Write contiguous data to HDF5 data set. Data is flattened into
    // a 1D array, e.g. [x0, y0, z0, x1, y1, z1] for a vector in 3D
    template <typename T>
    void write_data(const std::string dataset_name,
                    const std::vector<T>& data,
                    const std::vector<std::size_t> global_size);

    // Search dataset names for one beginning with search_term
    static std::string search_list(const std::vector<std::string>& list,
                                   const std::string& search_term);

    // Reorder vertices into global index order, so they can be saved
    // correctly for HDF5 mesh output
    std::vector<double>
      reorder_vertices_by_global_indices(const Mesh& mesh) const;

    // Reorder data values of type double into global index order
    // Shape of 2D array is given in global_size
    void reorder_values_by_global_indices(const Mesh& mesh,
                               std::vector<double>& data,
                               std::vector<std::size_t>& global_size) const;

    // HDF5 file descriptor/handle
    bool hdf5_file_open;
    hid_t hdf5_file_id;

    // Parallel mode
    const bool mpi_io;
  };

  //---------------------------------------------------------------------------
  template <typename T>
  void HDF5File::write_data(const std::string dataset_name,
                            const std::vector<T>& data,
                            const std::vector<std::size_t> global_size)
  {
    dolfin_assert(hdf5_file_open);

    dolfin_assert(global_size.size() > 0);

    // Get number of 'items'
    std::size_t num_local_items = 1;
    for (std::size_t i = 1; i < global_size.size(); ++i)
      num_local_items *= global_size[i];
    num_local_items = data.size()/num_local_items;

    // Compute offset
    const std::size_t offset = MPI::global_offset(num_local_items, true);
    std::pair<std::size_t, std::size_t> range(offset, offset + num_local_items);

    const bool chunking = parameters["chunking"];
    // Write data to HDF5 file
    HDF5Interface::write_dataset(hdf5_file_id, dataset_name, data,
                                 range, global_size, mpi_io, chunking);
  }
  //---------------------------------------------------------------------------

}

#endif
#endif
