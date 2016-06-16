/**
 * @file complete_incremental_termination.hpp
 * @author Sumedh Ghaisas
 *
 * Termination policy used in AMF (Alternating Matrix Factorization).
 *
 * This file is part of mlpack 2.0.2.
 *
 * mlpack is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * mlpack is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details (LICENSE.txt).
 *
 * You should have received a copy of the GNU General Public License along with
 * mlpack.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MLPACK_METHODS_AMF_COMPLETE_INCREMENTAL_TERMINATION_HPP
#define MLPACK_METHODS_AMF_COMPLETE_INCREMENTAL_TERMINATION_HPP

namespace mlpack {
namespace amf {

/**
 * This class acts as a wrapper for basic termination policies to be used by
 * SVDCompleteIncrementalLearning. This class calls the wrapped class functions
 * after every n calls to main class functions where n is the number of non-zero
 * entries in the matrix being factorized. This is necessary for
 * SVDCompleteIncrementalLearning, because otherwise IsConverged() is called
 * after every point, which is very slow.
 *
 * @see AMF, SVDCompleteIncrementalLearning
 */
template<class TerminationPolicy>
class CompleteIncrementalTermination
{
 public:
  /**
   * Empty constructor.
   *
   * @param tPolicy object of wrapped class.
   */
  CompleteIncrementalTermination(
      TerminationPolicy tPolicy = TerminationPolicy()) :
      tPolicy(tPolicy) { }

  /**
   * Initializes the termination policy before stating the factorization.
   *
   * @param V Input matrix to be factorized.
   */
  template<class MatType>
  void Initialize(const MatType& V)
  {
    tPolicy.Initialize(V);

    // Get the number of non-zero entries.
    incrementalIndex = arma::accu(V != 0);
    iteration = 0;
  }

  /**
   * Initializes the termination policy before stating the factorization.  This
   * is a specialization for sparse matrices.
   *
   * @param V Input matrix to be factorized.
   */
  void Initialize(const arma::sp_mat& V)
  {
    tPolicy.Initialize(V);

    // Get number of non-zero entries
    incrementalIndex = V.n_nonzero;
    iteration = 0;
  }

  /**
   * Check if termination criterion is met, if the current iteration means that
   * each point has been visited.
   *
   * @param W Basis matrix of output.
   * @param H Encoding matrix of output.
   */
  bool IsConverged(arma::mat& W, arma::mat& H)
  {
    // Increment iteration count.
    iteration++;

    // If iteration count is multiple of incremental index, return wrapped class
    // function.
    if (iteration % incrementalIndex == 0)
      return tPolicy.IsConverged(W, H);
    else
      return false;
  }

  //! Get current value of residue
  const double& Index() const { return tPolicy.Index(); }

  //! Get current iteration count
  const size_t& Iteration() const { return iteration; }

  //! Access upper limit of iteration count.
  const size_t& MaxIterations() const { return tPolicy.MaxIterations(); }
  //! Modify maximum number of iterations.
  size_t& MaxIterations() { return tPolicy.MaxIterations(); }

  //! Access the wrapped termination policy.
  const TerminationPolicy& TPolicy() const { return tPolicy; }
  //! Modify the wrapped termination policy.
  TerminationPolicy& TPolicy() { return tPolicy; }

 private:
  //! Wrapped termination policy.
  TerminationPolicy tPolicy;

  //! Number of iterations after which wrapped termination policy will be
  //! called.
  size_t incrementalIndex;
  //! Current iteration number.
  size_t iteration;
}; // class CompleteIncrementalTermination

} // namespace amf
} // namespace mlpack

#endif // MLPACK_METHODS_AMF_COMPLETE_INCREMENTAL_TERMINATION_HPP

