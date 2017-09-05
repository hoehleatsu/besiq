#include <dcdflib/libdcdf.hpp>
#include <besiq/method/wald_lm_method.hpp>
#include <besiq/stats/snp_count.hpp>

wald_lm_method::wald_lm_method(method_data_ptr data, bool unequal_var)
: method_type::method_type( data ),
  m_unequal_var( unequal_var )
{
    m_weight = arma::ones<arma::vec>( data->phenotype.n_elem );
    m_missing = get_data( )->missing;
    m_pheno = get_data()->phenotype;
}

std::vector<std::string>
wald_lm_method::init()
{
    std::vector<std::string> header;

    header.push_back( "LR" );
    header.push_back( "P" );
    header.push_back( "df" );

    return header;
}

arma::mat
wald_lm_method::get_last_C()
{
    return m_C;
}

arma::vec
wald_lm_method::get_last_beta()
{
    return m_beta;
}


double
wald_lm_method::run(const snp_row &row1, const snp_row &row2, float *output)
{
    arma::mat suf = arma::zeros<arma::mat>( 3, 3 );
    arma::mat suf2 = arma::zeros<arma::mat>( 3, 3 );
    arma::mat n = arma::zeros<arma::mat>( 3, 3 );

    for(int i = 0; i < row1.size( ); i++)
    {
        unsigned char snp1 = row1[ i ];
        unsigned char snp2 = row2[ i ];
        
        if( snp1 == 3 || snp2 == 3 || m_missing[ i ] == 1 )
        {
            continue;
        }

        double pheno = m_pheno[ i ];
        n( snp1, snp2 ) += 1;
        suf( snp1, snp2 ) += pheno;
        suf2( snp1, snp2 ) += pheno * pheno;
    }

    /* Calculate residual and estimate sigma^2 */
    arma::mat resid = arma::zeros<arma::mat>( 3, 3 );
    arma::mat mu = arma::zeros<arma::mat>( 3, 3 );
    double num_samples = 0.0;
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if( n( i, j ) < METHOD_SMALLEST_CELL_SIZE_NORMAL )
            {
                continue;
            }

            resid( i, j ) = ( suf2( i, j ) - suf( i, j ) * suf( i, j ) / n( i, j ) );
            mu( i, j ) = suf( i, j ) / n( i, j );
            num_samples += n( i, j );
        }
    }
    set_num_ok_samples( (size_t)num_samples );

    arma::mat sigma2 = arma::zeros<arma::mat>( 3, 3 );
    if( !m_unequal_var )
    {
        sigma2 = sigma2 + arma::accu( resid ) / ( num_samples - 9 );
    }
    else
    {
        for(int i = 0; i < 3; i++)
        {
            for(int j = 0; j < 3; j++)
            {
                if( n( i, j ) > 9 )
                {
                    sigma2( i, j ) = resid( i, j ) / ( n( i, j ) - 9 );
                }
            }
        }
    }

    /* Find valid parameters and estimate beta */
    int num_valid = 0;
    arma::uvec valid( 4 );
    m_beta = arma::zeros<arma::vec>( 4 );
    int i_map[] = { 1, 1, 2, 2 };
    int j_map[] = { 1, 2, 1, 2 };
    for(int i = 0; i < 4; i++)
    {
        int c_i = i_map[ i ];
        int c_j = j_map[ i ];
        if( n( 0, 0 ) >= METHOD_SMALLEST_CELL_SIZE_NORMAL &&
            n( 0, c_j ) >= METHOD_SMALLEST_CELL_SIZE_NORMAL &&
            n( c_i, 0 ) >= METHOD_SMALLEST_CELL_SIZE_NORMAL &&
            n( c_i, c_j) >= METHOD_SMALLEST_CELL_SIZE_NORMAL )
        {
            valid[ num_valid ] = i;
            m_beta[ num_valid ] = mu( 0, 0 ) - mu( 0, c_j ) - mu( c_i, 0 ) + mu( c_i, c_j );
            num_valid++;
        }
    }
    if( num_valid <= 0 )
    {
        return -9;
    }

    valid.resize( num_valid );
    m_beta.resize( num_valid );

    /* Construct covariance matrix */ 
    m_C = arma::zeros<arma::mat>( num_valid, num_valid );
    for(int iv = 0; iv < num_valid; iv++)
    {
        int i = valid[ iv ];
        int c_i = i_map[ i ];
        int c_j = j_map[ i ];

        for(int jv = 0; jv < num_valid; jv++)
        {
            int j = valid[ jv ];
            int o_i = i_map[ j ];
            int o_j = j_map[ j ];

            int same_row = c_i == o_i;
            int same_col = c_j == o_j;
            int in_cell = i == j;

            m_C( iv, jv ) = ( sigma2( 0, 0 ) / n( 0, 0 ) + same_col * sigma2( 0, c_j ) / n( 0, c_j ) + same_row * sigma2( c_i, 0 ) / n( c_i, 0 ) + in_cell * sigma2( c_i, c_j ) / n( c_i, c_j ) );
        }
    }

    arma::mat Cinv( num_valid, num_valid );
    if( !inv( Cinv, m_C ) )
    {
        return -9;
    }
    
    /* Test if b != 0 with Wald test */
    double chi = dot( m_beta, Cinv * m_beta );
    output[ 0 ] = chi;
    output[ 1 ] = 1.0 - chi_square_cdf( chi, num_valid );
    output[ 2 ] = valid.n_elem;

    return output[ 1 ];
}
