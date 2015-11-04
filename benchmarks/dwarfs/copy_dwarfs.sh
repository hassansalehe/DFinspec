#!/bin/bash


rm -rf _bins
mkdir _bins
mkdir _bins/adf _bins/omp _bins/seq _bins/tbb _bins/tbb_flow


# ADF

DIR=_bins/adf/

BINS="branch_bound/branch_bound_adf "
BINS+="combinatorial_logic/combinatorial_logic_adf "
BINS+="dense_algebra/dense_algebra_adf "
BINS+="finite_state_machine/finite_state_machine_adf "
BINS+="graph_models/graph_models_adf "
BINS+="map_reduce/map_reduce_adf "
BINS+="sparse_algebra/sparse_algebra_adf "
BINS+="spectral_methods/spectral_methods_adf "
BINS+="structured_grid/structured_grid_adf "
BINS+="unstructured_grid/unstructured_grid_adf "

cp $BINS $DIR

# OPENMP

DIR=_bins/openmp/

BINS="branch_bound/branch_bound_omp "
BINS+="combinatorial_logic/combinatorial_logic_omp "
BINS+="dense_algebra/dense_algebra_omp "
BINS+="finite_state_machine/finite_state_machine_omp "
BINS+="graph_models/graph_models_omp "
BINS+="map_reduce/map_reduce_omp "
BINS+="sparse_algebra/sparse_algebra_omp "
BINS+="spectral_methods/spectral_methods_omp "
BINS+="structured_grid/structured_grid_omp "
BINS+="unstructured_grid/unstructured_grid_omp "

cp $BINS $DIR


# SEQUENTIAL

DIR=_bins/sequential/

BINS="branch_bound/branch_bound_seq "
BINS+="combinatorial_logic/combinatorial_logic_seq "
BINS+="dense_algebra/dense_algebra_seq "
BINS+="finite_state_machine/finite_state_machine_seq "
BINS+="graph_models/graph_models_seq "
BINS+="map_reduce/map_reduce_seq "
BINS+="sparse_algebra/sparse_algebra_seq "
BINS+="spectral_methods/spectral_methods_seq "
BINS+="structured_grid/structured_grid_seq "
BINS+="unstructured_grid/unstructured_grid_seq "

cp $BINS $DIR

# TBB

DIR=_bins/tbb/

BINS="branch_bound/branch_bound_tbb "
BINS+="combinatorial_logic/combinatorial_logic_tbb "
BINS+="dense_algebra/dense_algebra_tbb "
BINS+="finite_state_machine/finite_state_machine_tbb "
BINS+="graph_models/graph_models_tbb "
BINS+="map_reduce/map_reduce_tbb "
BINS+="sparse_algebra/sparse_algebra_tbb "
BINS+="spectral_methods/spectral_methods_tbb "
BINS+="structured_grid/structured_grid_tbb "
BINS+="unstructured_grid/unstructured_grid_tbb "

cp $BINS $DIR

# TBB_FLOW

DIR=_bins/tbb_flow/

BINS="branch_bound/branch_bound_tbb_flow "
BINS+="combinatorial_logic/combinatorial_logic_tbb_flow "
BINS+="dense_algebra/dense_algebra_tbb_flow "
BINS+="finite_state_machine/finite_state_machine_tbb_flow "
BINS+="graph_models/graph_models_tbb_flow "
BINS+="map_reduce/map_reduce_tbb_flow "
BINS+="sparse_algebra/sparse_algebra_tbb_flow "
BINS+="spectral_methods/spectral_methods_tbb_flow "
BINS+="structured_grid/structured_grid_tbb_flow "
BINS+="unstructured_grid/unstructured_grid_tbb_flow "

cp $BINS $DIR




