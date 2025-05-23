// LIC// ====================================================================
// LIC// This file forms part of oomph-lib, the object-oriented,
// LIC// multi-physics finite-element library, available
// LIC// at http://www.oomph-lib.org.
// LIC//
// LIC// Copyright (C) 2006-2024 Matthias Heil and Andrew Hazel
// LIC//
// LIC// This library is free software; you can redistribute it and/or
// LIC// modify it under the terms of the GNU Lesser General Public
// LIC// License as published by the Free Software Foundation; either
// LIC// version 2.1 of the License, or (at your option) any later version.
// LIC//
// LIC// This library is distributed in the hope that it will be useful,
// LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
// LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// LIC// Lesser General Public License for more details.
// LIC//
// LIC// You should have received a copy of the GNU Lesser General Public
// LIC// License along with this library; if not, write to the Free Software
// LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// LIC// 02110-1301  USA.
// LIC//
// LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
// LIC//
// LIC//====================================================================
// Header file for general linear elasticity elements

// Include guards to prevent multiple inclusion of the header
#ifndef OOMPH_PML_TIME_HARMONIC_LINEAR_ELASTICITY_ELEMENTS_HEADER
#define OOMPH_PML_TIME_HARMONIC_LINEAR_ELASTICITY_ELEMENTS_HEADER

// Config header generated by autoconfig
#ifdef HAVE_CONFIG_H
#include <oomph-lib-config.h>
#endif


#ifdef OOMPH_HAS_MPI
#include "mpi.h"
#endif

#include <complex>
#include "math.h"

// OOMPH-LIB headers
#include "../generic/Qelements.h"
#include "../generic/mesh.h"
#include "../generic/hermite_elements.h"
#include "./pml_time_harmonic_elasticity_tensor.h"
#include "../generic/projection.h"
#include "../generic/nodes.h"
#include "../generic/oomph_utilities.h"
#include "../generic/pml_meshes.h"
#include "../generic/pml_mapping_functions.h"

// The meshes (needed for building of pml meshes!)
// Include template files to avoid unnecessary re-compilation
// (*.template.h files get included indirectly).
//#include "../meshes/triangle_mesh.template.cc"
//#include "../meshes/rectangular_quadmesh.template.cc"

// Why not just to include the *.h files, Just as all other files
// #include "../meshes/triangle_mesh.template.h"
// #include "../meshes/rectangular_quadmesh.template.h"

namespace oomph
{
  //=======================================================================
  /// A base class for elements that solve the equations of time-harmonic linear
  /// elasticity in Cartesian coordinates.
  /// Combines a few generic functions that are shared by
  /// PMLTimeHarmonicLinearElasticityEquations
  /// and PMLTimeHarmonicLinearElasticityEquationsWithPressure
  /// (Note: The latter
  /// don't exist yet but will be written as soon as somebody needs them...)
  //=======================================================================
  template<unsigned DIM>
  class PMLTimeHarmonicLinearElasticityEquationsBase
    : public virtual PMLElementBase<DIM>,
      public virtual FiniteElement
  {
  public:
    /// Constructor: Set null pointers for constitutive law and for
    /// isotropic growth function. Set physical parameter values to
    /// default values, and set body force to zero.
    PMLTimeHarmonicLinearElasticityEquationsBase()
      : Elasticity_tensor_pt(0),
        Omega_sq_pt(&Default_omega_sq_value),
        Body_force_fct_pt(0)
    {
      Pml_mapping_pt =
        &PMLTimeHarmonicLinearElasticityEquationsBase::Default_pml_mapping;
    }

    /// Return the index at which the i-th real or imag unknown
    /// displacement component is stored. The default value is appropriate for
    /// single-physics problems:
    virtual inline std::complex<unsigned> u_index_time_harmonic_linear_elasticity(
      const unsigned i) const
    {
      return std::complex<unsigned>(i, i + DIM);
    }

    /// Compute vector of FE interpolated displacement u at local coordinate s
    void interpolated_u_time_harmonic_linear_elasticity(
      const Vector<double>& s, Vector<std::complex<double>>& disp) const
    {
      // Find number of nodes
      unsigned n_node = nnode();

      // Local shape function
      Shape psi(n_node);

      // Find values of shape function
      shape(s, psi);

      for (unsigned i = 0; i < DIM; i++)
      {
        // Index at which the nodal value is stored
        std::complex<unsigned> u_nodal_index =
          u_index_time_harmonic_linear_elasticity(i);

        // Initialise value of u
        disp[i] = std::complex<double>(0.0, 0.0);

        // Loop over the local nodes and sum
        for (unsigned l = 0; l < n_node; l++)
        {
          const std::complex<double> u_value(
            nodal_value(l, u_nodal_index.real()),
            nodal_value(l, u_nodal_index.imag()));

          disp[i] += u_value * psi[l];
        }
      }
    }

    /// Return FE interpolated displacement u[i] at local coordinate s
    std::complex<double> interpolated_u_time_harmonic_linear_elasticity(
      const Vector<double>& s, const unsigned& i) const
    {
      // Find number of nodes
      unsigned n_node = nnode();

      // Local shape function
      Shape psi(n_node);

      // Find values of shape function
      shape(s, psi);

      // Get nodal index at which i-th velocity is stored
      std::complex<unsigned> u_nodal_index =
        u_index_time_harmonic_linear_elasticity(i);

      // Initialise value of u
      std::complex<double> interpolated_u(0.0, 0.0);

      // Loop over the local nodes and sum
      for (unsigned l = 0; l < n_node; l++)
      {
        const std::complex<double> u_value(
          nodal_value(l, u_nodal_index.real()),
          nodal_value(l, u_nodal_index.imag()));

        interpolated_u += u_value * psi[l];
      }

      return (interpolated_u);
    }


    /// Function pointer to function that specifies the body force
    /// as a function of the Cartesian coordinates FCT(x,b) --
    /// x and b are  Vectors!
    typedef void (*BodyForceFctPt)(const Vector<double>& x,
                                   Vector<std::complex<double>>& b);

    /// Return the pointer to the elasticity_tensor
    PMLTimeHarmonicIsotropicElasticityTensor*& elasticity_tensor_pt()
    {
      return Elasticity_tensor_pt;
    }

    /// Access function to the entries in the elasticity tensor
    inline std::complex<double> E(const unsigned& i,
                                  const unsigned& j,
                                  const unsigned& k,
                                  const unsigned& l) const
    {
      return (*Elasticity_tensor_pt)(i, j, k, l);
    }

    /// Access function to nu in the elasticity tensor
    inline double nu() const
    {
      return Elasticity_tensor_pt->nu();
    }

    /// Access function for square of non-dim frequency
    const double& omega_sq() const
    {
      return *Omega_sq_pt;
    }

    /// Access function for square of non-dim frequency
    double*& omega_sq_pt()
    {
      return Omega_sq_pt;
    }

    /// Access function: Pointer to body force function
    BodyForceFctPt& body_force_fct_pt()
    {
      return Body_force_fct_pt;
    }

    /// Access function: Pointer to body force function (const version)
    BodyForceFctPt body_force_fct_pt() const
    {
      return Body_force_fct_pt;
    }

    /// Return the Cauchy stress tensor, as calculated
    /// from the elasticity tensor at specified local coordinate
    /// Virtual so separaete versions can (and must!) be provided
    /// for displacement and pressure-displacement formulations.
    virtual void get_stress(const Vector<double>& s,
                            DenseMatrix<std::complex<double>>& sigma) const = 0;

    /// Return the strain tensor
    void get_strain(const Vector<double>& s,
                    DenseMatrix<std::complex<double>>& strain) const;

    /// Evaluate body force at Eulerian coordinate x
    /// (returns zero vector if no body force function pointer has been set)
    inline void body_force(const Vector<double>& x,
                           Vector<std::complex<double>>& b) const
    {
      // If no function has been set, return zero vector
      if (Body_force_fct_pt == 0)
      {
        // Get spatial dimension of element
        unsigned n = dim();
        for (unsigned i = 0; i < n; i++)
        {
          b[i] = std::complex<double>(0.0, 0.0);
        }
      }
      else
      {
        // Get body force
        (*Body_force_fct_pt)(x, b);
      }
    }


    /// Pure virtual function in which we specify the
    /// values to be pinned (and set to zero) on the outer edge of
    /// the pml layer. All of them! Vector is resized internally.
    void values_to_be_pinned_on_outer_pml_boundary(
      Vector<unsigned>& values_to_pin)
    {
      values_to_pin.resize(DIM * 2);
      for (unsigned j = 0; j < DIM * 2; j++)
      {
        values_to_pin[j] = j;
      }
    }

    /// The number of "DOF types" that degrees of freedom in this element
    /// are sub-divided into: for now lump them all into one DOF type.
    /// Can be adjusted later
    unsigned ndof_types() const
    {
      return 1;
    }

    /// Create a list of pairs for all unknowns in this element,
    /// so that the first entry in each pair contains the global equation
    /// number of the unknown, while the second one contains the number
    /// of the "DOF types" that this unknown is associated with.
    /// (Function can obviously only be called if the equation numbering
    /// scheme has been set up.)
    void get_dof_numbers_for_unknowns(
      std::list<std::pair<unsigned long, unsigned>>& dof_lookup_list) const
    {
      // temporary pair (used to store dof lookup prior to being added
      // to list)
      std::pair<unsigned long, unsigned> dof_lookup;

      // number of nodes
      const unsigned n_node = this->nnode();

      // Integer storage for local unknown
      int local_unknown = 0;

      // Loop over the nodes
      for (unsigned n = 0; n < n_node; n++)
      {
        // Loop over dimension (real and imag)
        for (unsigned i = 0; i < 2 * DIM; i++)
        {
          // If the variable is free
          local_unknown = nodal_local_eqn(n, i);

          // ignore pinned values
          if (local_unknown >= 0)
          {
            // store dof lookup in temporary pair: First entry in pair
            // is global equation number; second entry is dof type
            dof_lookup.first = this->eqn_number(local_unknown);
            dof_lookup.second = 0;

            // add to list
            dof_lookup_list.push_front(dof_lookup);
          }
        }
      }
    }

    /// Compute pml coefficients at position x and integration point ipt.
    /// pml_inverse_jacobian_diagonal contains the diagonal terms from the
    /// inverse of the Jacobian of the PML transformation. These are used to
    /// transform derivatives in real x to derivatives in transformed space
    /// \f$\tilde x \f$. This can be interpreted as an anisotropic stiffness.
    /// pml_jacobian_det is the determinant of the Jacobian of the PML
    /// transformation, this allows us to transform volume integrals in
    /// transformed space to real space.
    /// This can be interpreted as a mass factor
    /// If the PML is not enabled via enable_pml, both default to 1.0.
    void compute_pml_coefficients(
      const unsigned& ipt,
      const Vector<double>& x,
      Vector<std::complex<double>>& pml_inverse_jacobian_diagonal,
      std::complex<double>& pml_jacobian_det)
    {
      /// The factors all default to 1.0 if the propagation
      /// medium is the physical domain (no PML transformation)
      for (unsigned k = 0; k < DIM; k++)
      {
        pml_inverse_jacobian_diagonal[k] = std::complex<double>(1.0, 0.0);
      }
      pml_jacobian_det = std::complex<double>(1.0, 0.0);

      // Only calculate PML factors if PML is enabled
      if (this->Pml_is_enabled)
      {
        /// Vector which points from the inner boundary to x
        Vector<double> nu(DIM);
        for (unsigned k = 0; k < DIM; k++)
        {
          nu[k] = x[k] - this->Pml_inner_boundary[k];
        }

        /// Vector which points from the inner boundary to the edge of the
        /// boundary
        Vector<double> pml_width(DIM);
        for (unsigned k = 0; k < DIM; k++)
        {
          pml_width[k] =
            this->Pml_outer_boundary[k] - this->Pml_inner_boundary[k];
        }

#ifdef PARANOID
        // Check if the Pml_mapping_pt is set
        if (this->Pml_mapping_pt == 0)
        {
          std::ostringstream error_message_stream;
          error_message_stream << "Pml_mapping_pt needs to be set "
                               << std::endl;

          throw OomphLibError(error_message_stream.str(),
                              OOMPH_CURRENT_FUNCTION,
                              OOMPH_EXCEPTION_LOCATION);
        }
#endif
        // Declare gamma_i vectors of complex numbers for PML weights
        Vector<std::complex<double>> pml_gamma(DIM);

        /// Calculate the square of the non-dimensional wavenumber
        double wavenumber_squared = 2.0 * (1.0 + this->nu()) * this->omega_sq();

        for (unsigned k = 0; k < DIM; k++)
        {
          // If PML is enabled in the respective direction
          if (this->Pml_direction_active[k])
          {
            std::complex<double> pml_gamma =
              Pml_mapping_pt->gamma(nu[k], pml_width[k], wavenumber_squared);

            // The diagonals of the INVERSE of the PML transformation jacobian
            // are 1/gamma
            pml_inverse_jacobian_diagonal[k] = 1.0 / pml_gamma;
            // To get the determinant, multiply all the diagonals together
            pml_jacobian_det *= pml_gamma;
          }
        }
      }
    }

    /// Return a pointer to the PML Mapping object
    PMLMapping*& pml_mapping_pt()
    {
      return Pml_mapping_pt;
    }

    /// Return a pointer to the PML Mapping object (const version)
    PMLMapping* const& pml_mapping_pt() const
    {
      return Pml_mapping_pt;
    }

    /// Static so that the class doesn't need to instantiate a new default
    /// everytime it uses it
    static ContinuousBermudezPMLMapping Default_pml_mapping;

  protected:
    /// Pointer to the elasticity tensor
    PMLTimeHarmonicIsotropicElasticityTensor* Elasticity_tensor_pt;

    /// Square of nondim frequency
    double* Omega_sq_pt;

    /// Pointer to body force function
    BodyForceFctPt Body_force_fct_pt;

    /// Static default value for square of frequency
    static double Default_omega_sq_value;

    /// Pointer to class which holds the pml mapping function, also known as
    /// gamma
    PMLMapping* Pml_mapping_pt;
  };


  /// ////////////////////////////////////////////////////////////////////
  /// ////////////////////////////////////////////////////////////////////
  /// ////////////////////////////////////////////////////////////////////


  //=======================================================================
  /// A class for elements that solve the equations of linear elasticity
  /// in cartesian coordinates.
  //=======================================================================
  template<unsigned DIM>
  class PMLTimeHarmonicLinearElasticityEquations
    : public PMLTimeHarmonicLinearElasticityEquationsBase<DIM>
  {
  public:
    ///  Constructor
    PMLTimeHarmonicLinearElasticityEquations() {}

    /// Number of values required at node n.
    unsigned required_nvalue(const unsigned& n) const
    {
      return 2 * DIM;
    }

    /// Return the residuals for the solid equations (the discretised
    /// principle of virtual displacements)
    void fill_in_contribution_to_residuals(Vector<double>& residuals)
    {
      fill_in_generic_contribution_to_residuals_time_harmonic_linear_elasticity(
        residuals, GeneralisedElement::Dummy_matrix, 0);
    }

    /// The jacobian is calculated by finite differences by default,
    /// We need only to take finite differences w.r.t. positional variables
    /// For this element
    void fill_in_contribution_to_jacobian(Vector<double>& residuals,
                                          DenseMatrix<double>& jacobian)
    {
      // Add the contribution to the residuals
      this
        ->fill_in_generic_contribution_to_residuals_time_harmonic_linear_elasticity(
          residuals, jacobian, 1);
    }

    /// Return the Cauchy stress tensor, as calculated
    /// from the elasticity tensor at specified local coordinate
    void get_stress(const Vector<double>& s,
                    DenseMatrix<std::complex<double>>& sigma) const;

    /// Output exact solution x,y,[z],u_r,v_r,[w_r],u_i,v_i,[w_i]
    void output_fct(std::ostream& outfile,
                    const unsigned& nplot,
                    FiniteElement::SteadyExactSolutionFctPt exact_soln_pt);

    /// Output: x,y,[z],u_r,v_r,[w_r],u_i,v_i,[w_i]
    void output(std::ostream& outfile)
    {
      unsigned n_plot = 5;
      output(outfile, n_plot);
    }

    /// Output: x,y,[z],u_r,v_r,[w_r],u_i,v_i,[w_i]
    void output(std::ostream& outfile, const unsigned& n_plot);


    /// C-style output: x,y,[z],u_r,v_r,[w_r],u_i,v_i,[w_i]
    void output(FILE* file_pt)
    {
      unsigned n_plot = 5;
      output(file_pt, n_plot);
    }

    /// Output: x,y,[z],u_r,v_r,[w_r],u_i,v_i,[w_i]
    void output(FILE* file_pt, const unsigned& n_plot);

    /// Output function for real part of full time-dependent solution
    /// constructed by adding the scattered field
    ///  u = Re( (u_r +i u_i) exp(-i omega t)
    /// at phase angle omega t = phi computed here, to the corresponding
    /// incoming wave specified via the function pointer.
    void output_total_real(
      std::ostream& outfile,
      FiniteElement::SteadyExactSolutionFctPt incoming_wave_fct_pt,
      const double& phi,
      const unsigned& nplot);


    /// Output function for real part of full time-dependent solution
    /// u = Re( (u_r +i u_i) exp(-i omega t))
    /// at phase angle omega t = phi.
    /// x,y,u   or    x,y,z,u at n_plot plot points in each coordinate
    /// direction
    void output_real(std::ostream& outfile,
                     const double& phi,
                     const unsigned& n_plot);

    /// Output function for imaginary part of full time-dependent
    /// solution u = Im( (u_r +i u_i) exp(-i omega t) ) at phase angle omega t =
    /// phi. x,y,u   or    x,y,z,u at n_plot plot points in each coordinate
    /// direction
    void output_imag(std::ostream& outfile,
                     const double& phi,
                     const unsigned& n_plot);


    /// Compute norm of solution: square of the L2 norm
    void compute_norm(double& norm);

    /// Get error against and norm of exact solution
    void compute_error(std::ostream& outfile,
                       FiniteElement::SteadyExactSolutionFctPt exact_soln_pt,
                       double& error,
                       double& norm);


    /// Dummy, time dependent error checker
    void compute_error(std::ostream& outfile,
                       FiniteElement::UnsteadyExactSolutionFctPt exact_soln_pt,
                       const double& time,
                       double& error,
                       double& norm)
    {
      std::ostringstream error_stream;
      error_stream << "There is no time-dependent compute_error() " << std::endl
                   << "for pml time harmonic linear elasticity elements"
                   << std::endl;
      throw OomphLibError(
        error_stream.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
    }

  private:
    /// Private helper function to compute residuals and (if requested
    /// via flag) also the Jacobian matrix.
    virtual void fill_in_generic_contribution_to_residuals_time_harmonic_linear_elasticity(
      Vector<double>& residuals, DenseMatrix<double>& jacobian, unsigned flag);
  };

  /// /////////////////////////////////////////////////////////////////////
  /// /////////////////////////////////////////////////////////////////////
  /// /////////////////////////////////////////////////////////////////////


  //===========================================================================
  /// An Element that solves the equations of linear elasticity
  /// in Cartesian coordinates, using QElements for the geometry
  //============================================================================
  template<unsigned DIM, unsigned NNODE_1D>
  class QPMLTimeHarmonicLinearElasticityElement
    : public virtual QElement<DIM, NNODE_1D>,
      public virtual PMLTimeHarmonicLinearElasticityEquations<DIM>
  {
  public:
    /// Constructor
    QPMLTimeHarmonicLinearElasticityElement()
      : QElement<DIM, NNODE_1D>(),
        PMLTimeHarmonicLinearElasticityEquations<DIM>()
    {
    }

    /// Output function
    void output(std::ostream& outfile)
    {
      PMLTimeHarmonicLinearElasticityEquations<DIM>::output(outfile);
    }

    /// Output function
    void output(std::ostream& outfile, const unsigned& n_plot)
    {
      PMLTimeHarmonicLinearElasticityEquations<DIM>::output(outfile, n_plot);
    }


    /// C-style output function
    void output(FILE* file_pt)
    {
      PMLTimeHarmonicLinearElasticityEquations<DIM>::output(file_pt);
    }

    /// C-style output function
    void output(FILE* file_pt, const unsigned& n_plot)
    {
      PMLTimeHarmonicLinearElasticityEquations<DIM>::output(file_pt, n_plot);
    }
  };


  //============================================================================
  /// FaceGeometry of a linear 2D
  /// QPMLTimeHarmonicLinearElasticityElement element
  //============================================================================
  template<>
  class FaceGeometry<QPMLTimeHarmonicLinearElasticityElement<2, 2>>
    : public virtual QElement<1, 2>
  {
  public:
    /// Constructor must call the constructor of the underlying solid element
    FaceGeometry() : QElement<1, 2>() {}
  };


  //============================================================================
  /// FaceGeometry of a quadratic 2D
  /// QPMLTimeHarmonicLinearElasticityElement element
  //============================================================================
  template<>
  class FaceGeometry<QPMLTimeHarmonicLinearElasticityElement<2, 3>>
    : public virtual QElement<1, 3>
  {
  public:
    /// Constructor must call the constructor of the underlying element
    FaceGeometry() : QElement<1, 3>() {}
  };


  //============================================================================
  /// FaceGeometry of a cubic 2D
  /// QPMLTimeHarmonicLinearElasticityElement element
  //============================================================================
  template<>
  class FaceGeometry<QPMLTimeHarmonicLinearElasticityElement<2, 4>>
    : public virtual QElement<1, 4>
  {
  public:
    /// Constructor must call the constructor of the underlying element
    FaceGeometry() : QElement<1, 4>() {}
  };


  //============================================================================
  /// FaceGeometry of a linear 3D
  /// QPMLTimeHarmonicLinearElasticityElement element
  //============================================================================
  template<>
  class FaceGeometry<QPMLTimeHarmonicLinearElasticityElement<3, 2>>
    : public virtual QElement<2, 2>
  {
  public:
    /// Constructor must call the constructor of the underlying element
    FaceGeometry() : QElement<2, 2>() {}
  };

  //============================================================================
  /// FaceGeometry of a quadratic 3D
  /// QPMLTimeHarmonicLinearElasticityElement element
  //============================================================================
  template<>
  class FaceGeometry<QPMLTimeHarmonicLinearElasticityElement<3, 3>>
    : public virtual QElement<2, 3>
  {
  public:
    /// Constructor must call the constructor of the underlying element
    FaceGeometry() : QElement<2, 3>() {}
  };


  //============================================================================
  /// FaceGeometry of a cubic 3D
  /// QPMLTimeHarmonicLinearElasticityElement element
  //============================================================================
  template<>
  class FaceGeometry<QPMLTimeHarmonicLinearElasticityElement<3, 4>>
    : public virtual QElement<2, 4>
  {
  public:
    /// Constructor must call the constructor of the underlying element
    FaceGeometry() : QElement<2, 4>() {}
  };

  /// /////////////////////////////////////////////////////////////////
  /// /////////////////////////////////////////////////////////////////
  /// /////////////////////////////////////////////////////////////////


  //==========================================================
  /// Time-harmonic linear elasticity upgraded to become projectable
  //==========================================================
  template<class TIME_HARMONIC_LINEAR_ELAST_ELEMENT>
  class ProjectablePMLTimeHarmonicLinearElasticityElement
    : public virtual ProjectableElement<TIME_HARMONIC_LINEAR_ELAST_ELEMENT>
  {
  public:
    /// Constructor [this was only required explicitly
    /// from gcc 4.5.2 onwards...]
    ProjectablePMLTimeHarmonicLinearElasticityElement() {}


    /// Specify the values associated with field fld.
    /// The information is returned in a vector of pairs which comprise
    /// the Data object and the value within it, that correspond to field fld.
    /// In the underlying time-harmonic linear elasticity elemements the
    /// real and complex parts of the displacements are stored
    /// at the nodal values
    Vector<std::pair<Data*, unsigned>> data_values_of_field(const unsigned& fld)
    {
      // Create the vector
      Vector<std::pair<Data*, unsigned>> data_values;

      // Loop over all nodes and extract the fld-th nodal value
      unsigned nnod = this->nnode();
      for (unsigned j = 0; j < nnod; j++)
      {
        // Add the data value associated with the velocity components
        data_values.push_back(std::make_pair(this->node_pt(j), fld));
      }

      // Return the vector
      return data_values;
    }

    /// Number of fields to be projected: 2*dim, corresponding to
    /// real and imag parts of the displacement components
    unsigned nfields_for_projection()
    {
      return 2 * this->dim();
    }

    /// Number of history values to be stored for fld-th field.
    /// (includes present value!)
    unsigned nhistory_values_for_projection(const unsigned& fld)
    {
#ifdef PARANOID
      if (fld > 3)
      {
        std::stringstream error_stream;
        error_stream << "Elements only store four fields so fld can't be"
                     << " " << fld << std::endl;
        throw OomphLibError(
          error_stream.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
      }
#endif
      return this->node_pt(0)->ntstorage();
    }

    /// Number of positional history values: Read out from
    /// positional timestepper
    /// (Note: count includes current value!)
    unsigned nhistory_values_for_coordinate_projection()
    {
      return this->node_pt(0)->position_time_stepper_pt()->ntstorage();
    }

    /// Return Jacobian of mapping and shape functions of field fld
    /// at local coordinate s
    double jacobian_and_shape_of_field(const unsigned& fld,
                                       const Vector<double>& s,
                                       Shape& psi)
    {
      unsigned n_dim = this->dim();
      unsigned n_node = this->nnode();
      DShape dpsidx(n_node, n_dim);

      // Call the derivatives of the shape functions and return
      // the Jacobian
      return this->dshape_eulerian(s, psi, dpsidx);
    }


    /// Return interpolated field fld at local coordinate s, at time
    /// level t (t=0: present; t>0: history values)
    double get_field(const unsigned& t,
                     const unsigned& fld,
                     const Vector<double>& s)
    {
      unsigned n_node = this->nnode();

#ifdef PARANOID
      unsigned n_dim = this->node_pt(0)->ndim();
#endif

      // Local shape function
      Shape psi(n_node);

      // Find values of shape function
      this->shape(s, psi);

      // Initialise value of u
      double interpolated_u = 0.0;

      // Sum over the local nodes at that time
      for (unsigned l = 0; l < n_node; l++)
      {
#ifdef PARANOID
        unsigned nvalue = this->node_pt(l)->nvalue();
        if (nvalue != 2 * n_dim)
        {
          std::stringstream error_stream;
          error_stream
            << "Current implementation only works for non-resized nodes\n"
            << "but nvalue= " << nvalue << "!= 2 dim = " << 2 * n_dim
            << std::endl;
          throw OomphLibError(error_stream.str(),
                              OOMPH_CURRENT_FUNCTION,
                              OOMPH_EXCEPTION_LOCATION);
        }
#endif
        interpolated_u += this->nodal_value(t, l, fld) * psi[l];
      }
      return interpolated_u;
    }


    /// Return number of values in field fld
    unsigned nvalue_of_field(const unsigned& fld)
    {
      return this->nnode();
    }


    /// Return local equation number of value j in field fld.
    int local_equation(const unsigned& fld, const unsigned& j)
    {
#ifdef PARANOID
      unsigned n_dim = this->node_pt(0)->ndim();
      unsigned nvalue = this->node_pt(j)->nvalue();
      if (nvalue != 2 * n_dim)
      {
        std::stringstream error_stream;
        error_stream
          << "Current implementation only works for non-resized nodes\n"
          << "but nvalue= " << nvalue << "!= 2 dim = " << 2 * n_dim
          << std::endl;
        throw OomphLibError(
          error_stream.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
      }
#endif
      return this->nodal_local_eqn(j, fld);
    }
  };


  //=======================================================================
  /// Face geometry for element is the same as that for the underlying
  /// wrapped element
  //=======================================================================
  template<class ELEMENT>
  class FaceGeometry<ProjectablePMLTimeHarmonicLinearElasticityElement<ELEMENT>>
    : public virtual FaceGeometry<ELEMENT>
  {
  public:
    FaceGeometry() : FaceGeometry<ELEMENT>() {}
  };


  //=======================================================================
  /// Face geometry of the Face Geometry for element is the same as
  /// that for the underlying wrapped element
  //=======================================================================
  template<class ELEMENT>
  class FaceGeometry<
    FaceGeometry<ProjectablePMLTimeHarmonicLinearElasticityElement<ELEMENT>>>
    : public virtual FaceGeometry<FaceGeometry<ELEMENT>>
  {
  public:
    FaceGeometry() : FaceGeometry<FaceGeometry<ELEMENT>>() {}
  };


  /// ///////////////////////////////////////////////////////////////////
  /// ///////////////////////////////////////////////////////////////////
  /// ///////////////////////////////////////////////////////////////////


  //=======================================================================
  /// Policy class defining the elements to be used in the actual
  /// PML layers. Same!
  //=======================================================================
  template<unsigned NNODE_1D>
  class PMLLayerElement<QPMLTimeHarmonicLinearElasticityElement<2, NNODE_1D>>
    : public virtual QPMLTimeHarmonicLinearElasticityElement<2, NNODE_1D>
  {
  public:
    /// Constructor: Call the constructor for the
    /// appropriate QElement
    PMLLayerElement() : QPMLTimeHarmonicLinearElasticityElement<2, NNODE_1D>()
    {
    }
  };


} // namespace oomph


#endif
