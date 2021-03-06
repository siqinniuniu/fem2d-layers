#include "acoustic2d.h"
#include "parameters.h"
#include "petscksp.h"
#include "fem/auxiliary_functions.h"
#include "fem/finite_element.h"
#include "analytic_functions.h"
#include "fem/result.h"
#include "fem/math_functions.h"
#include "layer.h"
#include "block_of_layers.h"
#include <iostream>
#include <algorithm>
#include <fstream>

#define WATCH_RHS

using namespace fem;


void Acoustic2D::solve_triangles()
{
  require(_param->FE_ORDER == 1, "This fe order is not implemented (" + d2s(_param->FE_ORDER) + ")");

  // read the fine triangular mesh from the file
  _fmesh.read(_param->MESH_FILE);

#if defined(DEBUG)
  std::cout << "n_nodes = " << _fmesh.n_vertices() << std::endl;
#endif

  FiniteElement fe(_param->FE_ORDER);

  DoFHandler dof_handler(&_fmesh);
  dof_handler.distribute_dofs(fe, CG);

  // create sparse format based on the distribution of degrees of freedom.
  // since we use first order basis functions, and then
  // all dofs are associated with the mesh vertices,
  // sparse format is based on connectivity of the mesh vertices
  CSRPattern csr_pattern;
  csr_pattern.make_sparse_format(dof_handler, CG);

  expect(csr_pattern.order() == dof_handler.n_dofs(), "Error");
#if defined(DEBUG)
  std::cout << "n_dofs = " << dof_handler.n_dofs() << std::endl;
#endif

  // allocate memory
  VecCreateSeq(PETSC_COMM_SELF, csr_pattern.order(), &_global_rhs);
  MatCreateSeqAIJ(PETSC_COMM_WORLD, csr_pattern.order(), csr_pattern.order(), 0, csr_pattern.nnz(), &_global_mass_mat);
  MatCreateSeqAIJ(PETSC_COMM_WORLD, csr_pattern.order(), csr_pattern.order(), 0, csr_pattern.nnz(), &_global_stiff_mat);

  // allocate the memory for local matrices and vectors
  double **local_mass_mat = new double*[Triangle::n_dofs_first];
  double **local_stiff_mat = new double*[Triangle::n_dofs_first];
  for (unsigned int i = 0; i < Triangle::n_dofs_first; ++i)
  {
    local_mass_mat[i] = new double[Triangle::n_dofs_first];
    local_stiff_mat[i] = new double[Triangle::n_dofs_first];
  }

  // fill up the array of coefficient a
  double *coef_alpha = new double[_fmesh.n_triangles()];
  double *coef_beta  = new double[_fmesh.n_triangles()];
  for (unsigned int cell = 0; cell < _fmesh.n_triangles(); ++cell)
  {
    const Triangle triangle = _fmesh.triangle(cell);
    if ((int)triangle.material_id() == _param->INCL_DOMAIN)
    {
      coef_alpha[cell] = _param->COEF_A_VALUES[1]; // coefficient in the inclusion
      coef_beta[cell]  = _param->COEF_B_VALUES[1];  // coefficient in the inclusion
    }
//    else if (_param->INCL_RADIUS > 1e-8) // if the radius of the inclusion is not zero
//    {
//      const Point incl_center(_param->INCL_CENTER_X, _param->INCL_CENTER_Y);
//      const Point tri_center = triangle.center(_fmesh.vertices());
//      if (norm(tri_center - incl_center) < _param->INCL_RADIUS) // the center of the triangle is inside the circular inclusion
//      {
//        coef_alpha[cell] = _param->COEF_ALPHA_2_VALUE; // coefficient in the inclusion
//        coef_beta[cell]  = _param->COEF_BETA_2_VALUE;  // coefficient in the inclusion
//      }
//    }
    else // in all other cases, it is the main domain
    {
      coef_alpha[cell] = _param->COEF_A_VALUES[0]; // coefficient in the main domain
      coef_beta[cell]  = _param->COEF_B_VALUES[0]; // coefficient in the main domain
    }
  }

  // assemble the matrices and the rhs vector
  for (unsigned int cell = 0; cell < _fmesh.n_triangles(); ++cell)
  {
    const Triangle triangle = _fmesh.triangle(cell);
    triangle.local_mass_matrix(coef_alpha[cell], local_mass_mat);
    triangle.local_stiffness_matrix(coef_beta[cell], local_stiff_mat);

    for (unsigned int i = 0; i < triangle.n_dofs(); ++i)
    {
      const unsigned int dof_i = triangle.dof(i);
      //VecSetValue(_global_rhs, dof_i, local_rhs_vec[i], ADD_VALUES);
      for (unsigned int j = 0; j < triangle.n_dofs(); ++j)
      {
        const unsigned int dof_j = triangle.dof(j);
        MatSetValue(_global_mass_mat, dof_i, dof_j, local_mass_mat[i][j], ADD_VALUES);
        MatSetValue(_global_stiff_mat, dof_i, dof_j, local_stiff_mat[i][j], ADD_VALUES);
      }
    }
  }

  delete[] coef_alpha;
  delete[] coef_beta;

  // free the memory
  for (unsigned int i = 0; i < Triangle::n_dofs_first; ++i)
  {
    delete[] local_mass_mat[i];
    delete[] local_stiff_mat[i];
  }
  delete[] local_mass_mat;
  delete[] local_stiff_mat;

  MatAssemblyBegin(_global_mass_mat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(_global_mass_mat, MAT_FINAL_ASSEMBLY);

  MatAssemblyBegin(_global_stiff_mat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(_global_stiff_mat, MAT_FINAL_ASSEMBLY);

  if (_param->TIME_SCHEME == EXPLICIT)
    solve_explicit_triangles(dof_handler, csr_pattern);
  else if(_param->TIME_SCHEME == CRANK_NICOLSON)
    solve_crank_nicolson(dof_handler, csr_pattern);
  else
    require(false, "Unknown time discretization scheme");


}




void Acoustic2D::solve_crank_nicolson(const DoFHandler &dof_handler, const CSRPattern &csr_pattern)
{
  require(false, "Not implemented");
}



void Acoustic2D::solve_explicit_triangles(const DoFHandler &dof_handler, const CSRPattern &csr_pattern)
{
  // create vectors
  Vec solution; // numerical solution on the current (n-th) time step
  Vec solution_1; // numerical solution on the previous ((n-1)-th) time step
  Vec solution_2; // numerical solution on the preprevious ((n-2)-th) time step
  //Vec exact_solution; // analytic solution

  VecDuplicate(_global_rhs, &solution);
  VecDuplicate(_global_rhs, &solution_1);
  VecDuplicate(_global_rhs, &solution_2);
  //VecDuplicate(_global_rhs, &exact_solution);

  require(_param->FE_ORDER == 1, "This fe order is not implemented (" + d2s(_param->FE_ORDER) + ")");

  const double dt = _param->TIME_STEP;

  // fill vectors with solution on the 0-th and 1-st time steps
  const InitialSolution init_solution;
  for (unsigned int d = 0; d < dof_handler.n_dofs(); ++d)
  {
    VecSetValue(solution_2, d, init_solution.value(dof_handler.dof(d), _param->TIME_BEG), INSERT_VALUES);
    VecSetValue(solution_1, d, init_solution.value(dof_handler.dof(d), _param->TIME_BEG + dt), INSERT_VALUES);
  }

  // make a SLAE rhs vector
  Vec system_rhs, temp;
  VecCreateSeq(PETSC_COMM_SELF, csr_pattern.order(), &system_rhs);
  VecDuplicate(system_rhs, &temp);

  // boundary nodes
  const std::vector<int> &b_nodes = _fmesh.boundary_vertices();

  // system matrix equal to global mass matrix
  Mat system_mat;
  MatConvert(_global_mass_mat, MATSAME, MAT_INITIAL_MATRIX, &system_mat); // allocate memory and copy values

  // impose Dirichlet boundary condition
  // with ones on diagonal
  MatZeroRows(system_mat, b_nodes.size(), &b_nodes[0], 1., solution, system_rhs); // change the matrix

  // SLAE solver
  KSP ksp;
  KSPCreate(PETSC_COMM_WORLD, &ksp);
  KSPSetOperators(ksp, system_mat, system_mat, SAME_PRECONDITIONER);
  KSPSetTolerances(ksp, 1e-12, 1e-30, 1e+5, 10000);

  double *local_rhs_vec = new double[Triangle::n_dofs_first];

  require(_param->N_TIME_STEPS > 2, "There is no time steps to perform: n_time_steps = " + d2s(_param->N_TIME_STEPS));
  for (unsigned int time_step = 2; time_step <= _param->N_TIME_STEPS; ++time_step)
  {
    const double time = _param->TIME_BEG + time_step * dt; // current time
    VecSet(system_rhs, 0.); // zeroing the system rhs vector
    VecSet(solution, 0.); // zeroing the solution vector

    // assemble some parts of system rhs vector
    const RHSFunction rhs_function(*_param);
    for (unsigned int cell = 0; cell < _fmesh.n_triangles(); ++cell)
    {
      Triangle triangle = _fmesh.triangle(cell);
      triangle.local_rhs_vector(rhs_function, _fmesh.vertices(), time - dt, local_rhs_vec); // rhs function on the previous time step
      for (unsigned int i = 0; i < triangle.n_dofs(); ++i)
      {
        const unsigned int dof_i = triangle.dof(i);
        VecSetValue(system_rhs, dof_i, local_rhs_vec[i], ADD_VALUES);
      }
    } // rhs part assembling

    MatMult(_global_stiff_mat, solution_1, temp);
    VecAXPBY(system_rhs, -dt*dt, dt*dt, temp);

    MatMult(_global_mass_mat, solution_2, temp);
    VecAXPY(system_rhs, -1., temp);

    MatMult(_global_mass_mat, solution_1, temp);
    VecAXPY(system_rhs, 2., temp);

    // impose Dirichlet boundary condition
    const BoundaryFunction boundary_function;
    for (unsigned int i = 0; i < b_nodes.size(); ++i)
      VecSetValue(system_rhs, b_nodes[i], boundary_function.value(_fmesh.vertex(b_nodes[i]), time), INSERT_VALUES); // change the rhs vector

    // solve the SLAE
    KSPSolve(ksp, system_rhs, solution);

    // reassign the solutions on the previuos time steps
    VecCopy(solution_1, solution_2);
    VecCopy(solution,   solution_1);

    // check solution
//    for (int i = 0; i < _fmesh.n_vertices(); ++i)
//    {
//      Point vert = _fmesh.vertex(i);
//      VecSetValue(exact_solution, i, an_solution(vert, time), INSERT_VALUES);
//    }
//    std::cout << "time step = " << time_step << " time = " << time << " relative error = " << rel_error(solution, exact_solution) << std::endl;

#if defined(WATCH_RHS)
    {
      Result res(&dof_handler);
      std::string fname = _param->VTU_DIR + "/rhs-" + d2s(time_step) + ".vtu";
      res.write_vtu(fname, system_rhs);
    }
#endif

    if ((_param->PRINT_VTU && (time_step % _param->VTU_STEP == 0)) || (time_step == _param->N_TIME_STEPS))
    {
      Result res(&dof_handler);
      std::string fname = _param->VTU_DIR + "/res-" + d2s(time_step) + ".vtu";
      res.write_vtu(fname, solution); //, exact_solution);
    }

    if ((_param->SAVE_SOL && (time_step % _param->SOL_STEP == 0)) || (time_step == _param->N_TIME_STEPS))
    {
      // extract data from PETSc vector
      std::vector<int> idx(csr_pattern.order());
      std::iota(idx.begin(), idx.end(), 0); // idx = { 0, 1, 2, 3, .... }
      std::vector<double> solution_values(csr_pattern.order());
      VecGetValues(solution, csr_pattern.order(), &idx[0], &solution_values[0]);
      // write the solution to the file
      std::string fname = _param->SOL_DIR + "/sol-" + d2s(time_step) + ".dat";
      std::ofstream out(fname);
      out.setf(std::ios::scientific);
      out.precision(16);
      //out << csr_pattern.order() << "\n";
      for (unsigned int i = 0; i < csr_pattern.order(); ++i)
        out << solution_values[i] << "\n";
      out.close();
    }

    if (_param->PRINT_INFO)
    {
      double rhs_norm;
      VecNorm(system_rhs, NORM_2, &rhs_norm);
      double system_rhs_norm;
      VecNorm(system_rhs, NORM_2, &system_rhs_norm);
      double norm;
      VecNorm(solution, NORM_2, &norm);
      std::cout.setf(std::ios::scientific);
      std::cout.precision(4);
      std::cout << "  step " << time_step << " norm " << norm << " rhs_norm " << rhs_norm
                << " sys_rhs_norm " << system_rhs_norm << std::endl;
    }

  } // time loop

  // extract data from PETSc vector
  std::vector<int> idx(csr_pattern.order());
  std::iota(idx.begin(), idx.end(), 0); // idx = { 0, 1, 2, 3, .... }
  std::vector<double> solution_values(csr_pattern.order());
  VecGetValues(solution, csr_pattern.order(), &idx[0], &solution_values[0]);
  const std::string sol_filename = stem(_param->MESH_FILE) + "_sol.dat";
  std::ofstream out(sol_filename.c_str());
  require(out, "File " + sol_filename + " can't be opened");
  out.setf(std::ios_base::scientific);
  out.precision(14);
  out << solution_values.size() << "\n";
  for (unsigned i = 0; i < solution_values.size(); ++i)
    out << solution_values[i] << "\n";
  out.close();

  KSPDestroy(&ksp);

  MatDestroy(&system_mat);

  delete[] local_rhs_vec;

  VecDestroy(&solution);
  VecDestroy(&solution_1);
  VecDestroy(&solution_2);
  //VecDestroy(&exact_solution);
  VecDestroy(&system_rhs);
  VecDestroy(&temp);
}
