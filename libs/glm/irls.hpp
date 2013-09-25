#ifndef __IRLS_H__
#define __IRLS_H__

#include <armadillo>

#include <glm/models/glm_model.hpp>

/**
 * Contains additional statistics about the estimated betas.
 */
struct irls_info
{
    /**
    * Standard error of estimated beta.
    */
    arma::vec se_beta;

    /**
    * P-value for each beta, based on Wald test.
    */
    arma::vec p_value;

    /**
    * Number of iterations.
    */
    unsigned int num_iters;

    /**
     * Estimated mean value.
     */
    arma::vec mu;

    /**
     * Log likelihood of the model.
     */
    double logl;

    /**
    * True if converged, false otherwise.
    */
    bool converged;
};

/**
 * Maximum number of iterations in the IRLS algorithm.
 */
static const int IRLS_MAX_ITERS = 25;

/**
 * Smallest change in likelihood before terminating the IRLS algorithm.
 */
static double IRLS_TOLERANCE = 10e-8;

/**
 * Solves the weighted least square problem:
 *   
 *   W*X*W*b = X*W*y
 *
 * The algorithm uses the singular value decomposition, to
 * compute the solution b:
 *   
 *   b = V * S^-1 * U^t sqrt( w ) * y
 *
 * where wX = U S V^t
 *
 * @param X The design matrix.
 * @param y The right hand side.
 * @param w The weight for each observation.
 *
 * @return The vector b that minimizes the weighted least squares problem.
 */
arma::vec weighted_least_squares(const arma::mat &X, const arma::vec &y, const arma::vec &w);

/**
 * Compute the adjusted dependent variates in the Iteratively reweighted
 * least squares algorithm.
 *
 * @param eta The linearized parameter.
 * @param mu The mean value parameter.
 * @param mu_eta The derivative of mu with respect to eta.
 * @param y The observations.
 *
 * @return The adjusted dependent variates.
 */
arma::vec compute_z(const arma::vec &eta, const arma::vec &mu, const arma::vec &mu_eta, const arma::vec &y);

/**
 * Compute the weight vector that is used in one iteration
 * in the Iteratively reweighted least squares algorithm.
 *
 * @param var Variance of each observation.
 * @param mu_eta The derivative of mu with respect to eta.
 *
 * @return A weight vector.
 */
arma::vec compute_w(const arma::vec &var, const arma::vec& mu_eta);

/**
 * This function performs the iteratively reweighted
 * least squares algorithm to estimate beta coefficients
 * of a genearlized linear model.
 *
 * @param X The design matrix (caller is responsible for
 *          adding an intercept).
 * @param y The observations.
 * @param model The GLM model to estimate.
 * @param output Output statistics of the estimated betas.
 *
 * @return Estimated beta coefficients.
 */
arma::vec irls(const arma::mat &X, const arma::vec &y, const glm_model &model, irls_info &output);

/**
 * This function performs the iteratively reweighted
 * least squares algorithm to estimate beta coefficients
 * of a genearlized linear model.
 *
 * @param X The design matrix (caller is responsible for
 *          adding an intercept).
 * @param y The observations.
 * @param missing Identifies missing sampels by 1 and non-missing by 0.
 * @param model The GLM model to estimate.
 * @param output Output statistics of the estimated betas.
 *
 * @return Estimated beta coefficients.
 */
arma::vec irls(const arma::mat &X, const arma::vec &y, const arma::uvec &missing, const glm_model &model, irls_info &output);

#endif /* End of __IRLS_H__ */
