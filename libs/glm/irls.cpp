#include <iostream>
#include <cfloat>

#include <armadillo>

#include <glm/models/glm_model.hpp>
#include <glm/models/links/glm_link.hpp>
#include <glm/irls.hpp>
#include <dcdflib/libdcdf.hpp>

using namespace arma;

void
set_missing_to_zero(const uvec &missing, vec &w)
{   
    for(int i = 0; i < w.size( ); i++)
    {
        if( missing[ i ] == 1 )
        {
            w[ i ] = 0.0;
        }
    }
}

vec
chi_square_cdf(const vec &x, unsigned int df)
{
    vec p = ones<vec>( x.n_elem );
    for(int i = 0; i < x.n_elem; i++)
    {
        p[ i ] = chi_square_cdf( x[ i ], df );
    }

    return p;
}

vec
weighted_least_squares(const mat &X, const vec &y, const vec &w, bool fast_inversion)
{
    /* A = sqrt( w ) * X */
    mat A = diagmat( sqrt( w ) ) * X;

    /* ty = sqrt( w ) * y */
    vec ty = y % sqrt( w );

    vec beta;
    if( fast_inversion )
    {
        if( solve( beta, A, ty, solve_opts::fast + solve_opts::no_approx ) )
        {
            return beta;
        }
        else
        {
            return vec( );
        }
    }
    else
    {
        if( solve( beta, A, ty ) )
        {
            return beta;
        }
        else
        {
            return vec( );
        }
    }
}

vec
compute_z(const vec &eta, const vec &mu, const vec &mu_eta, const vec &y)
{
    return eta + mu_eta % ( y - mu );
}

vec
compute_w(const vec &var, const vec& mu_eta)
{
    return 1.0 / ( var % ( mu_eta % mu_eta ) );
}

vec
init_beta(const mat &X, const vec&y, const uvec &missing, const glm_model &model, bool fast_inversion = false)
{
    vec eta = model.get_link( ).eta( (y + 0.5) / 3.0 );

    return weighted_least_squares( X, eta, ones<vec>( missing.n_elem ) - missing, fast_inversion );
}

vec
irls(const mat &X, const vec &y, const uvec &missing, const glm_model &model, glm_info &output, bool fast_inversion)
{
    const glm_link &link = model.get_link( );
    vec b = init_beta( X, y, missing, model );
    vec w( X.n_rows );
    vec z( X.n_rows );
    vec eta = X * b;
    vec mu = link.mu( eta );

    vec mu_eta = link.mu_eta( mu );

    int num_iter = 0;
    double old_logl = -DBL_MAX;
    double logl = model.likelihood( mu, y, missing );
    bool invalid_mu = false;
    bool inverse_fail = false;
    vec b_old = b;
    bool first_attempt = true;
    while( num_iter < IRLS_MAX_ITERS && ! ( fabs( logl - old_logl ) / ( 0.1 + fabs( logl ) ) < IRLS_TOLERANCE ) )
    {
        w = compute_w( model.var( mu ), mu_eta );
        z = compute_z( eta, mu, mu_eta, y );
        set_missing_to_zero( missing, w );
        b = weighted_least_squares( X, z, w, fast_inversion );
        if( b.n_elem <= 0 )
        {
            inverse_fail = true;
            break;
        }

compute_eta: 
        eta = X * b;
        mu = link.mu( eta );
        mu_eta = link.mu_eta( mu );

        if( !model.valid_mu( mu ) )
        {
            if( first_attempt )
            {
                /* Try a smaller step */
                b = 0.5*b_old + 0.5*b;
                first_attempt = false;
                goto compute_eta;
            }
            else
            {
                invalid_mu = true;
                break;
            }
        }

        old_logl = logl;
        b_old = b;
        logl = model.likelihood( mu, y, missing );

        num_iter++;
    }

    if( num_iter < IRLS_MAX_ITERS && !invalid_mu && !inverse_fail )
    {
        mat I = X.t( ) * diagmat( w ) * X;
        mat C;
        if( I.is_finite( ) && inv( C, I ) )
        {
            float dispersion = model.dispersion( mu, y, missing, b.n_elem );
            output.se_beta = sqrt( model.dispersion( mu, y, missing, dispersion ) * diagvec( C ) );
            output.num_iters = num_iter;
            output.converged = true;
            output.success = true;
            output.mu = mu;
            output.logl = model.likelihood( mu, y, missing, dispersion );
            
            vec wald_z = b / output.se_beta;
            vec chi2_value = wald_z % wald_z;
            output.p_value = -1.0 * ones<vec>( chi2_value.n_elem );
            for(int i = 0; i < chi2_value.n_elem; i++)
            {
                try
                {
                    output.p_value[ i ] = 1.0 - chi_square_cdf( chi2_value[ i ], 1 );
                }
                catch(bad_domain_value &e)
                {
                    continue;
                }
            }
        }
        else
        {
            output.success = false;
        }
    }
    else
    {   
        output.num_iters = num_iter;
        output.converged = false;
        output.success = false;
    }

    return b;
}

vec
irls(const mat &X, const vec &y, const glm_model &model, glm_info &output, bool fast_inversion)
{
    uvec missing = zeros<uvec>( y.n_elem );
    return irls( X, y, missing, model, output );
}
