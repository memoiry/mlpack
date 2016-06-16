/**
 * @file linear_kernel.hpp
 * @author Wei Guan
 * @author James Cline
 * @author Ryan Curtin
 *
 * Implementation of the linear kernel (just the standard dot product).
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
#ifndef MLPACK_CORE_KERNELS_LINEAR_KERNEL_HPP
#define MLPACK_CORE_KERNELS_LINEAR_KERNEL_HPP

#include <mlpack/core.hpp>

namespace mlpack {
namespace kernel {

/**
 * The simple linear kernel (dot product).  For any two vectors @f$ x @f$ and
 * @f$ y @f$,
 *
 * @f[
 * K(x, y) = x^T y
 * @f]
 *
 * This kernel has no parameters and therefore the evaluation can be static.
 */
class LinearKernel
{
 public:
  /**
   * This constructor does nothing; the linear kernel has no parameters to
   * store.
   */
  LinearKernel() { }

  /**
   * Simple evaluation of the dot product.  This evaluation uses Armadillo's
   * dot() function.
   *
   * @tparam VecTypeA Type of first vector (should be arma::vec or
   *      arma::sp_vec).
   * @tparam VecTypeB Type of second vector (arma::vec / arma::sp_vec).
   * @param a First vector.
   * @param b Second vector.
   * @return K(a, b).
   */
  template<typename VecTypeA, typename VecTypeB>
  static double Evaluate(const VecTypeA& a, const VecTypeB& b)
  {
    return arma::dot(a, b);
  }

  //! Serialize the kernel (it has no members... do nothing).
  template<typename Archive>
  void Serialize(Archive& /* ar */, const unsigned int /* version */) { }
};

} // namespace kernel
} // namespace mlpack

#endif
