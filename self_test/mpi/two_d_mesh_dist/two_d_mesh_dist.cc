//LIC// ====================================================================
//LIC// This file forms part of oomph-lib, the object-oriented, 
//LIC// multi-physics finite-element library, available 
//LIC// at http://www.oomph-lib.org.
//LIC// 
//LIC// Copyright (C) 2006-2024 Matthias Heil and Andrew Hazel
//LIC// 
//LIC// This library is free software; you can redistribute it and/or
//LIC// modify it under the terms of the GNU Lesser General Public
//LIC// License as published by the Free Software Foundation; either
//LIC// version 2.1 of the License, or (at your option) any later version.
//LIC// 
//LIC// This library is distributed in the hope that it will be useful,
//LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
//LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//LIC// Lesser General Public License for more details.
//LIC// 
//LIC// You should have received a copy of the GNU Lesser General Public
//LIC// License along with this library; if not, write to the Free Software
//LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//LIC// 02110-1301  USA.
//LIC// 
//LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
//LIC// 
//LIC//====================================================================
//Driver for a simple 2D Poisson problem with adaptive mesh refinement

// General mpi header
#include "mpi.h"

//Generic routines
#include "generic.h"

// The Poisson equations
#include "poisson.h"

// The mesh 
#include "meshes/rectangular_quadmesh.h"


using namespace std;

using namespace oomph;


//====================================================================
/// Upgraded mesh
//====================================================================
template<class ELEMENT>
class MyRefineableRectangularQuadMesh : 
 public RefineableRectangularQuadMesh<ELEMENT>
{

public:

 /// constructor
 MyRefineableRectangularQuadMesh(const unsigned& n_x,
                                 const unsigned& n_y,
                                 const double &l_x,
                                 const double& l_y) :
  RectangularQuadMesh<ELEMENT>(n_x,n_y,l_x,l_y),
  RefineableRectangularQuadMesh<ELEMENT>(n_x,n_y,l_x,l_y)
  {}

};



//===== start_of_namespace=============================================
/// Namespace for exact solution for Poisson equation with "sharp step" 
//=====================================================================
namespace TanhSolnForPoisson
{

 /// Parameter for steepness of "step"
 double Alpha=5.0;

 /// Parameter for angle Phi of "step"
 double TanPhi=1.0;

 /// Exact solution as a Vector
 void get_exact_u(const Vector<double>& x, Vector<double>& u)
 {
  u[0]=tanh(1.0-Alpha*(TanPhi*x[0]-x[1]));
 }

 /// Source function required to make the solution above an exact solution 
 void get_source(const Vector<double>& x, double& source)
 {
  source = 2.0*tanh(-1.0+Alpha*(TanPhi*x[0]-x[1]))*
   (1.0-pow(tanh(-1.0+Alpha*(TanPhi*x[0]-x[1])),2.0))*
   Alpha*Alpha*TanPhi*TanPhi+2.0*tanh(-1.0+Alpha*(TanPhi*x[0]-x[1]))*
   (1.0-pow(tanh(-1.0+Alpha*(TanPhi*x[0]-x[1])),2.0))*Alpha*Alpha;
 }
 
} // end of namespace



/// ///////////////////////////////////////////////////////////////////
/// ///////////////////////////////////////////////////////////////////
/// ///////////////////////////////////////////////////////////////////

//====== start_of_problem_class=======================================
/// 2D Poisson problem on rectangular domain, discretised 
/// with refineable 2D QPoisson elements. The specific type
/// of element is specified via the template parameter.
//====================================================================
template<class ELEMENT> 
class RefineablePoissonProblem : public Problem
{

public:

 /// Constructor: Pass pointer to source functions
 RefineablePoissonProblem(
  PoissonEquations<2>::PoissonSourceFctPt source_fct_pt);

 /// Destructor. Empty
 ~RefineablePoissonProblem() {};

 /// Update the problem specs before solve: Reset boundary conditions
 /// to the values from the tanh solution.
 void actions_before_newton_solve();

 /// Update the problem after solve (empty)
 void actions_after_newton_solve(){}

 /// Doc the solution.
 void doc_solution(DocInfo& doc_info);

 /// Overloaded version of the problem's access function to 
 /// the mesh. Recasts the pointer to the base Mesh object to 
 /// the actual mesh type.
 MyRefineableRectangularQuadMesh<ELEMENT>* mesh_pt() 
  {
   return dynamic_cast<MyRefineableRectangularQuadMesh<ELEMENT>*>(
    Problem::mesh_pt());
  }

private:

 /// Pointer to source function
 PoissonEquations<2>::PoissonSourceFctPt Source_fct_pt;

}; // end of problem class




//=====start_of_constructor===============================================
/// Constructor for Poisson problem: Pass pointer to 
/// source function.
//========================================================================
template<class ELEMENT>
RefineablePoissonProblem<ELEMENT>::RefineablePoissonProblem(
 PoissonEquations<2>::PoissonSourceFctPt source_fct_pt)
       :  Source_fct_pt(source_fct_pt)
{ 

 // Setup mesh

 // # of elements in x-direction
 unsigned n_x=4;

 // # of elements in y-direction
 unsigned n_y=2;

 // Domain length in x-direction
 double l_x=2.0;

 // Domain length in y-direction
 double l_y=1.0;

 // Build and assign mesh
 Problem::mesh_pt() = 
  new MyRefineableRectangularQuadMesh<ELEMENT>(n_x,n_y,l_x,l_y);

 // Create/set error estimator
 mesh_pt()->spatial_error_estimator_pt()=new Z2ErrorEstimator;
  
 // Set the boundary conditions for this problem: All nodes are
 // free by default -- only need to pin the ones that have Dirichlet 
 // conditions here
 unsigned num_bound = mesh_pt()->nboundary();
 for(unsigned ibound=0;ibound<num_bound;ibound++)
  {
   unsigned num_nod= mesh_pt()->nboundary_node(ibound);
   for (unsigned inod=0;inod<num_nod;inod++)
    {
     mesh_pt()->boundary_node_pt(ibound,inod)->pin(0); 
    }
  } // end loop over boundaries
 
 // Complete the build of all elements so they are fully functional 

 // Loop over the elements to set up element-specific 
 // things that cannot be handled by the (argument-free!) ELEMENT 
 // constructor: Pass pointer to source function
 unsigned n_element = mesh_pt()->nelement();
 for(unsigned i=0;i<n_element;i++)
  {
   // Upcast from GeneralsedElement to the present element
   ELEMENT *el_pt = dynamic_cast<ELEMENT*>(mesh_pt()->element_pt(i));

   //Set the source function pointer
   el_pt->source_fct_pt() = Source_fct_pt;
  }

 // Setup equation numbering scheme
 oomph_info <<"Number of equations: " << assign_eqn_numbers() << std::endl; 

} // end of constructor


//========================================start_of_actions_before_newton_solve===
/// Update the problem specs before solve: (Re-)set boundary conditions
/// to the values from the tanh solution.
//========================================================================
template<class ELEMENT>
void RefineablePoissonProblem<ELEMENT>::actions_before_newton_solve()
{
 // How many boundaries are there?
 unsigned num_bound = mesh_pt()->nboundary();
 
 //Loop over the boundaries
 for(unsigned ibound=0;ibound<num_bound;ibound++)
  {
   // How many nodes are there on this boundary?
   unsigned num_nod=mesh_pt()->nboundary_node(ibound);

   // Loop over the nodes on boundary
   for (unsigned inod=0;inod<num_nod;inod++)
    {
     // Get pointer to node
     Node* nod_pt=mesh_pt()->boundary_node_pt(ibound,inod);

     // Extract nodal coordinates from node:
     Vector<double> x(2);
     x[0]=nod_pt->x(0);
     x[1]=nod_pt->x(1);

     // Compute the value of the exact solution at the nodal point
     Vector<double> u(1);
     TanhSolnForPoisson::get_exact_u(x,u);

     // Assign the value to the one (and only) nodal value at this node
     nod_pt->set_value(0,u[0]);
    }
  } 
}  // end of actions before solve




//===============start_of_doc=============================================
/// Doc the solution
//========================================================================
template<class ELEMENT>
void RefineablePoissonProblem<ELEMENT>::doc_solution(DocInfo& doc_info)
{ 

 ofstream some_file;
 char filename[100];

 // Number of plot points: npts x npts
 unsigned npts=5;

 // Output solution 
 sprintf(filename,"%s/soln%i_on_proc%i.dat",doc_info.directory().c_str(),
         doc_info.number(),this->communicator_pt()->my_rank());
 some_file.open(filename);
 mesh_pt()->output(some_file,npts);
 some_file.close();

 doc_info.number()++;


} // end of doc




//===== start_of_main=====================================================
/// Test mesh distribution in parallel, starting with n_uniform_first 
/// uniform refinments before starting the distribution. Then
/// refine and unrefine uniformly as specified. Re-distribute
/// after every uniform refinement if requested by the boolean flag.
//========================================================================
void parallel_test(const unsigned& n_refine_first, 
                   const unsigned& n_refine_once_distributed, 
                   const bool& redistribute,
                   const bool& solve,
                   const bool& doc)
{

 oomph_info 
  << std::endl  << std::endl
  << "Running with " << n_refine_first << " initial serial refinements and \n"
  << "with " << n_refine_once_distributed 
  << " subsequent distributed refinements"
  << std::endl << std::endl;

 double av_efficiency; 
 double max_efficiency;
 double min_efficiency;
 double av_number_halo_nodes;
 unsigned max_number_halo_nodes;
 unsigned min_number_halo_nodes;
 
 char filename[200];
 std::ofstream some_file;

 DocInfo doc_info;
 doc_info.set_directory("RESLT");

 DocInfo quiet_doc_info;
 quiet_doc_info.set_directory("RESLT");

    
 // Create the problem
 //-------------------
 RefineablePoissonProblem<RefineableQPoissonElement<2,3> >* problem_pt=
  new RefineablePoissonProblem<RefineableQPoissonElement<2,3> >(
   &TanhSolnForPoisson::get_source);

 // Storage for number of processors and current rank
 unsigned n_proc=problem_pt->communicator_pt()->nproc();
 unsigned my_rank=problem_pt->communicator_pt()->my_rank();

 // Initial number of dofs in base mesh
 unsigned n_dof_orig=problem_pt->ndof();
 unsigned n_dof=problem_pt->ndof();
 

 // Start with a few rounds of uniform refinement
 //----------------------------------------------
 clock_t t_start = clock();
 for (unsigned i=0;i<n_refine_first;i++)
  {
   // Refine uniformly
   problem_pt->refine_uniformly();
  } 
 clock_t t_end = clock();
 

 // Doc stats
 if (my_rank==0)
  {
   double t_tot=double(t_end-t_start)/CLOCKS_PER_SEC;
   oomph_info << "Time for initial adaptation [ndof=" << problem_pt->ndof() 
              << "]: " << t_tot << std::endl;
 
   n_dof=problem_pt->ndof();
   sprintf(
    filename,
    "%s/two_d_initial_refinement_nrefinement_%i_ndofbefore_%i_ndofafter_%i.dat",
    doc_info.directory().c_str(),
    n_refine_first,
    n_dof_orig,
    n_dof);
   some_file.open(filename);
   some_file << n_refine_first << " " 
             << n_dof_orig << " " 
             << n_dof << " " 
             << t_tot << " "
             << std::endl;
   some_file.close();    
 }


 
 // Now distribute the problem
 //---------------------------
 t_start = clock();
 if (doc)
  {
   std::ifstream input_file;
   std::ofstream output_file;

   // Get the partition to be used from file
   unsigned n_partition=problem_pt->mesh_pt()->nelement();
   Vector<unsigned> element_partition(n_partition);
   sprintf(filename,"two_d_mesh_dist_partition.dat");
   input_file.open(filename);
   std::string input_string;
   for (unsigned e=0;e<n_partition;e++)
    {
     getline(input_file,input_string,'\n');
     element_partition[e]=atoi(input_string.c_str());
    }

   Vector<unsigned> out_element_partition;
   bool report_stats=false;
   out_element_partition=problem_pt->distribute(element_partition,
                                                doc_info,report_stats);

   sprintf(filename,"out_two_d_mesh_dist_partition.dat");
   output_file.open(filename);
   for (unsigned e=0;e<n_partition;e++)
    {
     output_file << out_element_partition[e] << std::endl;
    }
  }
 else
  {
   problem_pt->distribute();
  }
 t_end = clock();
 

 // Assess the quality of the distribution
 //---------------------------------------
 problem_pt->mesh_pt()->get_halo_node_stats(av_number_halo_nodes,
                                            max_number_halo_nodes,
                                            min_number_halo_nodes);
 problem_pt->mesh_pt()->get_efficiency_of_mesh_distribution
  (av_efficiency,max_efficiency,min_efficiency);
 

 // Doc stats
 if (my_rank==0)
  {
   double t_tot=double(t_end-t_start)/CLOCKS_PER_SEC;
   oomph_info << "Time for problem distribution [ndof=" << problem_pt->ndof() 
              << "]: " << t_tot << std::endl;

   sprintf(
    filename,
    "%s/two_d_initial_distr_np_%i_nrefinement_%i_ndof_%i.dat",
    doc_info.directory().c_str(),
    n_proc,
    n_refine_first,
    n_dof);
   some_file.open(filename);
   some_file << n_proc << " " 
             << n_dof << " " 
             << t_tot << " "
             << av_number_halo_nodes << " " 
             << max_number_halo_nodes << " " 
             << min_number_halo_nodes  << " " 
             << av_efficiency << " " 
             << max_efficiency << " " 
             << min_efficiency << " " 
             << std::endl;
   some_file.close();    
  }


 // Check things
 //-------------
 problem_pt->check_halo_schemes(quiet_doc_info);


 // Solve?
 //-------
 if (solve)
  {

#ifdef OOMPH_HAS_TRILINOS

  // Notify...
  oomph_info << "Using the TRILINOS solver" << std::endl;

  // Create a Trilinos solver 
  TrilinosAztecOOSolver* linear_solver_pt = new TrilinosAztecOOSolver;

  // Create the ML preconditioner
  TrilinosMLPreconditioner* preconditioner_pt = new TrilinosMLPreconditioner;

  // Set preconditioner pointer
  linear_solver_pt->preconditioner_pt() = preconditioner_pt;

  // Choose solver type (GMRES)
  linear_solver_pt->solver_type()=TrilinosAztecOOSolver::GMRES;

  // Set linear solver
  problem_pt->linear_solver_pt() = linear_solver_pt;
  linear_solver_pt->disable_doc_time();
    
#endif

   // Solve
   problem_pt->newton_solve();
   
   if (doc)
    {
     // Doc the solution
     problem_pt->doc_solution(doc_info);
    }
  }


 

 // Now refine the distributed problem uniformly
 //---------------------------------------------
 for (unsigned i=0;i<n_refine_once_distributed;i++)
  {
   // Number of dofs before uniform refinement
   n_dof_orig=problem_pt->ndof();

   // Refine uniformly
   //-----------------
   t_start = clock();   
   problem_pt->refine_uniformly();
   t_end = clock();
 

   // Assess efficiency
   //------------------
   problem_pt->mesh_pt()->get_halo_node_stats(av_number_halo_nodes,
                                              max_number_halo_nodes,
                                              min_number_halo_nodes);
   problem_pt->mesh_pt()->get_efficiency_of_mesh_distribution
    (av_efficiency,max_efficiency,min_efficiency);
 

   if (my_rank==0)
    {
     double t_tot=double(t_end-t_start)/CLOCKS_PER_SEC;
     oomph_info << "Time for distributed adaptation [ndof=" 
                << problem_pt->ndof() 
                << "]: " << t_tot << std::endl;

     // Number of dofs achieved after uniform refinement
     n_dof=problem_pt->ndof();

     // Snippet indicating if  redistribution of haloes is used or not
     string snippet="";
     if (!redistribute) snippet="out";

     sprintf(
      filename,
      "%s/two_d_distributed_refinement_np_%i_ninitialrefinement_%i_ntotalrefinement_%i_ndof_%i_with%sredistribution.dat",
      doc_info.directory().c_str(),
      n_proc,
      n_refine_first,
      n_refine_first+i+1,
      n_dof,
      snippet.c_str());
     some_file.open(filename);
     some_file << n_proc << " " 
               << n_dof_orig << " " 
               << n_dof << " " 
               << t_tot << " "
               << av_number_halo_nodes << " " 
               << max_number_halo_nodes << " " 
               << min_number_halo_nodes  << " " 
               << av_efficiency << " " 
               << max_efficiency << " " 
               << min_efficiency << " " 
               << std::endl;
     some_file.close();   

    }


 
   // Redistribute (if required)
   //---------------------------
   if (redistribute)
    {
      
      t_start = clock();   
      bool report_stats=false;
      doc_info.disable_doc();
      problem_pt->prune_halo_elements_and_nodes(doc_info, 
                                                report_stats);
      t_end = clock();
      double t_tot=double(t_end-t_start)/CLOCKS_PER_SEC;
     

      if (my_rank==0)
       {

        oomph_info << "Av., min. max. efficiency before redistribution " 
                   << av_efficiency << " " 
                   << min_efficiency << " " 
                   << max_efficiency << " " 
                   << std::endl;
        sprintf(
         filename,
         "%s/two_d_redistribution_np_%i_ninitialrefinement_%i_ntotalrefinement_%i_ndof_%i.dat",
         doc_info.directory().c_str(),
         n_proc,
         n_refine_first,
         n_refine_first+i+1,
         n_dof);
        some_file.open(filename);
        some_file << n_proc << " " 
                  << n_dof << " " 
                  << t_tot << " "
                  << av_number_halo_nodes << " " 
                  << max_number_halo_nodes << " " 
                  << min_number_halo_nodes  << " " 
                  << av_efficiency << " " 
                  << max_efficiency << " " 
                  << min_efficiency << " " 
                  << std::endl;
       }


      // Re-assess efficiency
      //----------------------
      problem_pt->mesh_pt()->get_halo_node_stats(av_number_halo_nodes,
                                                 max_number_halo_nodes,
                                                 min_number_halo_nodes);
      problem_pt->mesh_pt()->
       get_efficiency_of_mesh_distribution(av_efficiency,
                                           max_efficiency,
                                           min_efficiency);
      
      
      if (my_rank==0)
       {
        oomph_info << "Time for redistribution  [ndof=" 
                   << problem_pt->ndof() 
                   << "]: " << t_tot << std::endl;


        some_file << n_proc << " " 
                  << n_dof << " " 
                  << t_tot << " "
                  << av_number_halo_nodes << " " 
                  << max_number_halo_nodes << " " 
                  << min_number_halo_nodes  << " " 
                  << av_efficiency << " " 
                  << max_efficiency << " " 
                  << min_efficiency << " " 
                  << std::endl;
        some_file.close();   
       }
            
      problem_pt->mesh_pt()->
       get_efficiency_of_mesh_distribution(av_efficiency,
                                           max_efficiency,
                                           min_efficiency);
      oomph_info << "Av., min., max. efficiency after redistribution " 
                 << av_efficiency << " " 
                 << min_efficiency << " " 
                 << max_efficiency << " " 
                 << std::endl;
    }
   
     
   // Check things
   //-------------
   problem_pt->check_halo_schemes(quiet_doc_info);


   // Solve again
   //------------
   if (solve)
    {
     // Solve again
     problem_pt->newton_solve();
     
     if (doc)
      {
       // Doc the solution
       problem_pt->doc_solution(doc_info);
      }
    }

 }

 
 // Unrefine the distributed problem uniformly (only if not redistributed)
 //-----------------------------------------------------------------------
 if (!redistribute)
  {
   // Number of dofs before unrefinement
   n_dof_orig=problem_pt->ndof();
   
   
   t_start = clock();
   problem_pt->unrefine_uniformly();
   t_end = clock();
   
   if (my_rank==0)
    {
     double t_tot=double(t_end-t_start)/CLOCKS_PER_SEC;
     oomph_info << "Time for distributed unrefinement [ndof=" 
                << problem_pt->ndof() 
                << "]: " << t_tot << std::endl;
     
     // Number of dofs after unrefinement
     n_dof=problem_pt->ndof();
     
     sprintf(
      filename,
      "%s/two_d_distributed_unrefinement_np_%i_ninitialrefinement_%i_ntotalrefinement_%i_ndof_%i.dat",
      doc_info.directory().c_str(),
      n_proc,
      n_refine_first,
      n_refine_first+n_refine_once_distributed,
      n_dof);
     some_file.open(filename);
     some_file << n_proc << " " 
               << n_dof_orig << " " 
               << n_dof << " " 
               << t_tot << " "
               << av_number_halo_nodes << " " 
               << max_number_halo_nodes << " " 
               << min_number_halo_nodes  << " " 
               << av_efficiency << " " 
               << max_efficiency << " " 
               << min_efficiency << " " 
               << std::endl;
     some_file.close();   
     
    }
   
      
   // Check things
   problem_pt->check_halo_schemes(quiet_doc_info);
   
   // Solve again
   if (solve)
    {
     // Solve again
     problem_pt->newton_solve();
     
     
     if (doc)
      {
       // Doc the solution
       problem_pt->doc_solution(doc_info);
      }
    }
  }

}






//===== start_of_main=====================================================
/// Driver code for 2D Poisson problem
//========================================================================
int main(int argc, char **argv)
{

#ifdef OOMPH_HAS_MPI
 // Setup MPI helpers
 MPI_Helpers::init(argc,argv);
#endif

 // Store command line arguments
 CommandLineArgs::setup(argc,argv);
 

 // Default min. number of refinements before we distribute
 unsigned n_refine_first_min=2;

 // Default max. number of refinements before we distribute
 unsigned n_refine_first_max=2;

 // Default min. number of uniform refinements after distribution
 unsigned n_uniform_min=3; 

 // Default max. number of uniform refinements after distribution
 unsigned n_uniform_max=3; 

 // Redistribute after uniform refinement?
 bool redistribute=true;
                 

        
 if (CommandLineArgs::Argc==1)
  {
   std::cout << "Using default settings for flags" << std::endl;
  }
 else if (CommandLineArgs::Argc==6)
  {

    // Min. number of refinements before we distribute
    n_refine_first_min=atoi(argv[1]);

    // Max. number of refinements before we distribute
    n_refine_first_max=atoi(argv[2]);

    // Min. number of uniform refinements after distribution
    n_uniform_min=atoi(argv[3]); 

    // Max. number of uniform refinements after distribution
    n_uniform_max=atoi(argv[4]); 

    // Redistribute after uniform refinement?
    redistribute=atoi(argv[5]); 
  }
 else
  {
   oomph_info 
    << "Wrong number of input args. Specify none or five " 
    << "(n_refine_first_min/max, n_uniform_min/max, redistributon flag)" 
    << std::endl;
   exit(1);
  }



 // Tell us what we're going to do
 oomph_info << "Run with  n_refine_first_min/max, n_uniform_min/max, redistribute: " 
            << n_refine_first_min << " "
            << n_refine_first_max << " "
            << n_uniform_min   << " " 
            << n_uniform_max   << " " 
            << redistribute  << " " 
            << std::endl;
 

 // Solve and self test?
 bool solve=true;
 bool doc=true;

 // Write headers for analysis
 ofstream some_file;
 some_file.open("RESLT/two_d_initial_refinement_header.dat");
 some_file << "VARIABLES= \"n<SUB>refine first</SUB>\"," 
           << "\"n<SUB>dof orig</SUB>\","
           << "\"n<SUB>dof</SUB>\","
           << "\"t<SUB>serial adapt</SUB>\""
           << std::endl;
 some_file.close();    


 some_file.open("RESLT/two_d_initial_distr_header.dat");
 some_file << "VARIABLES= \"n_<SUB>p</SUB>\","
           << "\"n<SUB>dof</SUB>\","
           << "\"t<SUB>distr</SUB>\","
           << "\"average n<SUB>halo nodes</SUB>\","
           << "\"max. n<SUB>halo nodes</SUB>\","
           << "\"min. n<SUB>halo nodes</SUB>\","
           << "\"average efficiency\","
           << "\"max. efficiency\","
           << "\"min. efficiency\""
           << std::endl;
 some_file.close();    


 some_file.open("RESLT/two_d_distributed_refinement_header.dat");
 some_file << "VARIABLES= \"n_<SUB>p</SUB>\","
           << "\"n<SUB>dof orig</SUB>\","
           << "\"n<SUB>dof</SUB>\","
           << "\"t<SUB>distributed refinement</SUB>\","
           << "\"average n<SUB>halo nodes</SUB>\","
           << "\"max. n<SUB>halo nodes</SUB>\","
           << "\"min. n<SUB>halo nodes</SUB>\","
           << "\"average efficiency\","
           << "\"max. efficiency\","
           << "\"min. efficiency\""
           << std::endl;
 some_file.close();   


 some_file.open("RESLT/two_d_redistribution_header.dat");
 some_file << "VARIABLES= \"n_<SUB>p</SUB>\","
           << "\"n<SUB>dof</SUB>\","
           << "\"t<SUB>redistribution</SUB>\","
           << "\"average n<SUB>halo nodes</SUB>\","
           << "\"max. n<SUB>halo nodes</SUB>\","
           << "\"min. n<SUB>halo nodes</SUB>\","
           << "\"average efficiency\","
           << "\"max. efficiency\","
           << "\"min. efficiency\""
           << std::endl;
 some_file.close();   



 some_file.open("RESLT/two_d_distributed_unrefinement_header.dat");
 some_file << "VARIABLES= \"n_<SUB>p</SUB>\","
           << "\"n<SUB>dof orig</SUB>\","
           << "\"n<SUB>dof</SUB>\","
           << "\"t<SUB>distributed unrefinement</SUB>\","
           << "\"average n<SUB>halo nodes</SUB>\","
           << "\"max. n<SUB>halo nodes</SUB>\","
           << "\"min. n<SUB>halo nodes</SUB>\","
           << "\"average efficiency\","
           << "\"max. efficiency\","
           << "\"min. efficiency\""
           << std::endl;
 some_file.close();   


 // Test the procedures in parallel mode
 for (unsigned i=n_refine_first_min;i<=n_refine_first_max;i++)

  {
   for (unsigned j=n_uniform_min;j<=n_uniform_max;j++)
    {
     parallel_test(i,j,redistribute,solve,doc);
    }
  }

#ifdef OOMPH_HAS_MPI
 MPI_Helpers::finalize();
#endif

}
