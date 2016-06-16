/**
 * @file kmeans_impl.hpp
 * @author Parikshit Ram (pram@cc.gatech.edu)
 * @author Ryan Curtin
 *
 * Implementation for the K-means method for getting an initial point.
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
#include "kmeans.hpp"

#include <mlpack/core/metrics/lmetric.hpp>
#include <mlpack/core/util/sfinae_utility.hpp>

namespace mlpack {
namespace kmeans {

/**
 * This gives us a GivesCentroids object that we can use to tell whether or not
 * an InitialPartitionPolicy returns centroids or point assignments.
 */
HAS_MEM_FUNC(Cluster, GivesCentroidsCheck);

/**
 * 'value' is true if the InitialPartitionPolicy class has a member
 * Cluster(const arma::mat& data, const size_t clusters, arma::mat& centroids).
 */
template<typename InitialPartitionPolicy>
struct GivesCentroids
{
  static const bool value =
    // Non-static version.
    GivesCentroidsCheck<InitialPartitionPolicy,
        void(InitialPartitionPolicy::*)(const arma::mat&,
                                        const size_t,
                                        arma::mat&)>::value ||
    // Static version.
    GivesCentroidsCheck<InitialPartitionPolicy,
        void(*)(const arma::mat&, const size_t, arma::mat&)>::value;
};

//! Call the initial partition policy, if it returns assignments.  This returns
//! 'true' to indicate that assignments were given.
template<typename MatType,
         typename InitialPartitionPolicy>
bool GetInitialAssignmentsOrCentroids(
    InitialPartitionPolicy& ipp,
    const MatType& data,
    const size_t clusters,
    arma::Row<size_t>& assignments,
    arma::mat& /* centroids */,
    const typename boost::disable_if_c<
        GivesCentroids<InitialPartitionPolicy>::value == true>::type* = 0)
{
  ipp.Cluster(data, clusters, assignments);

  return true;
}

//! Call the initial partition policy, if it returns centroids.  This returns
//! 'false' to indicate that assignments were not given.
template<typename MatType,
         typename InitialPartitionPolicy>
bool GetInitialAssignmentsOrCentroids(
    InitialPartitionPolicy& ipp,
    const MatType& data,
    const size_t clusters,
    arma::Row<size_t>& /* assignments */,
    arma::mat& centroids,
    const typename boost::enable_if_c<
        GivesCentroids<InitialPartitionPolicy>::value == true>::type* = 0)
{
  ipp.Cluster(data, clusters, centroids);

  return false;
}

/**
 * Construct the K-Means object.
 */
template<typename MetricType,
         typename InitialPartitionPolicy,
         typename EmptyClusterPolicy,
         template<class, class> class LloydStepType,
         typename MatType>
KMeans<
    MetricType,
    InitialPartitionPolicy,
    EmptyClusterPolicy,
    LloydStepType,
    MatType>::
KMeans(const size_t maxIterations,
       const MetricType metric,
       const InitialPartitionPolicy partitioner,
       const EmptyClusterPolicy emptyClusterAction) :
    maxIterations(maxIterations),
    metric(metric),
    partitioner(partitioner),
    emptyClusterAction(emptyClusterAction)
{
  // Nothing to do.
}

/**
 * Perform k-means clustering on the data, returning a list of cluster
 * assignments.  This just forward to the other function, which returns the
 * centroids too.  If this is properly inlined, there shouldn't be any
 * performance penalty whatsoever.
 */
template<typename MetricType,
         typename InitialPartitionPolicy,
         typename EmptyClusterPolicy,
         template<class, class> class LloydStepType,
         typename MatType>
inline void KMeans<
    MetricType,
    InitialPartitionPolicy,
    EmptyClusterPolicy,
    LloydStepType,
    MatType>::
Cluster(const MatType& data,
        const size_t clusters,
        arma::Row<size_t>& assignments,
        const bool initialGuess)
{
  arma::mat centroids(data.n_rows, clusters);
  Cluster(data, clusters, assignments, centroids, initialGuess);
}

/**
 * Perform k-means clustering on the data, returning a list of cluster
 * assignments and the centroids of each cluster.
 */
template<typename MetricType,
         typename InitialPartitionPolicy,
         typename EmptyClusterPolicy,
         template<class, class> class LloydStepType,
         typename MatType>
void KMeans<
    MetricType,
    InitialPartitionPolicy,
    EmptyClusterPolicy,
    LloydStepType,
    MatType>::
Cluster(const MatType& data,
        const size_t clusters,
        arma::mat& centroids,
        const bool initialGuess)
{
  // Make sure we have more points than clusters.
  if (clusters > data.n_cols)
    Log::Warn << "KMeans::Cluster(): more clusters requested than points given."
        << std::endl;
  else if (clusters == 0)
    Log::Warn << "KMeans::Cluster(): zero clusters requested.  This probably "
        << "isn't going to work.  Brace for crash." << std::endl;

  // Check validity of initial guess.
  if (initialGuess)
  {
    if (centroids.n_cols != clusters)
      Log::Fatal << "KMeans::Cluster(): wrong number of initial cluster "
        << "centroids (" << centroids.n_cols << ", should be " << clusters
        << ")!" << std::endl;

    if (centroids.n_rows != data.n_rows)
      Log::Fatal << "KMeans::Cluster(): initial cluster centroids have wrong "
        << " dimensionality (" << centroids.n_rows << ", should be "
        << data.n_rows << ")!" << std::endl;
  }

  // Use the partitioner to come up with the partition assignments and calculate
  // the initial centroids.
  if (!initialGuess)
  {
    // The GetInitialAssignmentsOrCentroids() function will call the appropriate
    // function in the InitialPartitionPolicy to return either assignments or
    // centroids.  We prefer centroids, but if assignments are returned, then we
    // have to calculate the initial centroids for the first iteration.
    arma::Row<size_t> assignments;
    bool gotAssignments = GetInitialAssignmentsOrCentroids(partitioner, data,
        clusters, assignments, centroids);
    if (gotAssignments)
    {
      // The partitioner gives assignments, so we need to calculate centroids
      // from those assignments.
      arma::Row<size_t> counts;
      counts.zeros(clusters);
      centroids.zeros(data.n_rows, clusters);
      for (size_t i = 0; i < data.n_cols; ++i)
      {
        centroids.col(assignments[i]) += arma::vec(data.col(i));
        counts[assignments[i]]++;
      }

      for (size_t i = 0; i < clusters; ++i)
        if (counts[i] != 0)
          centroids.col(i) /= counts[i];
    }
  }

  // Counts of points in each cluster.
  arma::Col<size_t> counts(clusters);

  size_t iteration = 0;

  LloydStepType<MetricType, MatType> lloydStep(data, metric);
  arma::mat centroidsOther;
  double cNorm;

  do
  {
    // We have two centroid matrices.  We don't want to copy anything, so,
    // depending on the iteration number, we use a different centroid matrix...
    if (iteration % 2 == 0)
      cNorm = lloydStep.Iterate(centroids, centroidsOther, counts);
    else
      cNorm = lloydStep.Iterate(centroidsOther, centroids, counts);

    // If we are not allowing empty clusters, then check that all of our
    // clusters have points.
    for (size_t i = 0; i < clusters; i++)
    {
      if (counts[i] == 0)
      {
        Log::Info << "Cluster " << i << " is empty.\n";
        if (iteration % 2 == 0)
          emptyClusterAction.EmptyCluster(data, i, centroids, centroidsOther,
              counts, metric, iteration);
        else
          emptyClusterAction.EmptyCluster(data, i, centroidsOther, centroids,
              counts, metric, iteration);
      }
    }

    iteration++;
    Log::Info << "KMeans::Cluster(): iteration " << iteration << ", residual "
        << cNorm << ".\n";
    if (std::isnan(cNorm) || std::isinf(cNorm))
      cNorm = 1e-4; // Keep iterating.

  } while (cNorm > 1e-5 && iteration != maxIterations);

  // If we ended on an even iteration, then the centroids are in the
  // centroidsOther matrix, and we need to steal its memory (steal_mem() avoids
  // a copy if possible).
  if ((iteration - 1) % 2 == 0)
    centroids.steal_mem(centroidsOther);

  if (iteration != maxIterations)
  {
    Log::Info << "KMeans::Cluster(): converged after " << iteration
        << " iterations." << std::endl;
  }
  else
  {
    Log::Info << "KMeans::Cluster(): terminated after limit of " << iteration
        << " iterations." << std::endl;
  }
  Log::Info << lloydStep.DistanceCalculations() << " distance calculations."
      << std::endl;
}

/**
 * Perform k-means clustering on the data, returning a list of cluster
 * assignments and the centroids of each cluster.
 */
template<typename MetricType,
         typename InitialPartitionPolicy,
         typename EmptyClusterPolicy,
         template<class, class> class LloydStepType,
         typename MatType>
void KMeans<
    MetricType,
    InitialPartitionPolicy,
    EmptyClusterPolicy,
    LloydStepType,
    MatType>::
Cluster(const MatType& data,
        const size_t clusters,
        arma::Row<size_t>& assignments,
        arma::mat& centroids,
        const bool initialAssignmentGuess,
        const bool initialCentroidGuess)
{
  // Now, the initial assignments.  First determine if they are necessary.
  if (initialAssignmentGuess)
  {
    if (assignments.n_elem != data.n_cols)
      Log::Fatal << "KMeans::Cluster(): initial cluster assignments (length "
          << assignments.n_elem << ") not the same size as the dataset (size "
          << data.n_cols << ")!" << std::endl;

    // Calculate initial centroids.
    arma::Row<size_t> counts;
    counts.zeros(clusters);
    centroids.zeros(data.n_rows, clusters);
    for (size_t i = 0; i < data.n_cols; ++i)
    {
      centroids.col(assignments[i]) += arma::vec(data.col(i));
      counts[assignments[i]]++;
    }

    for (size_t i = 0; i < clusters; ++i)
      if (counts[i] != 0)
        centroids.col(i) /= counts[i];
  }

  Cluster(data, clusters, centroids,
      initialAssignmentGuess || initialCentroidGuess);

  // Calculate final assignments.
  assignments.set_size(data.n_cols);
  for (size_t i = 0; i < data.n_cols; ++i)
  {
    // Find the closest centroid to this point.
    double minDistance = std::numeric_limits<double>::infinity();
    size_t closestCluster = centroids.n_cols; // Invalid value.

    for (size_t j = 0; j < centroids.n_cols; j++)
    {
      const double distance = metric.Evaluate(data.col(i), centroids.col(j));

      if (distance < minDistance)
      {
        minDistance = distance;
        closestCluster = j;
      }
    }

    Log::Assert(closestCluster != centroids.n_cols);
    assignments[i] = closestCluster;
  }
}

template<typename MetricType,
         typename InitialPartitionPolicy,
         typename EmptyClusterPolicy,
         template<class, class> class LloydStepType,
         typename MatType>
template<typename Archive>
void KMeans<MetricType,
            InitialPartitionPolicy,
            EmptyClusterPolicy,
            LloydStepType,
            MatType>::Serialize(Archive& ar, const unsigned int /* version */)
{
  ar & data::CreateNVP(maxIterations, "max_iterations");
  ar & data::CreateNVP(metric, "metric");
  ar & data::CreateNVP(partitioner, "partitioner");
  ar & data::CreateNVP(emptyClusterAction, "emptyClusterAction");
}

} // namespace kmeans
} // namespace mlpack
