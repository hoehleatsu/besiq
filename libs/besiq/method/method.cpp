#include <algorithm>

#include <plink/plink_file.hpp>
#include <besiq/method/method.hpp>
#include <besiq/io/pairfile.hpp>
#include <besiq/io/resultfile.hpp>

void run_method(method_type &method, genotype_matrix_ptr genotypes, pairfile &pairs, resultfile &result)
{
    std::vector<std::string> method_header = method.init( );
    method_header.push_back( "N" );
    result.set_header( method_header );

    float *output = new float[ method_header.size( ) ];
    double threshold = method.get_data( )->threshold;
    
    std::pair<std::string, std::string> pair;
    while( pairs.read( pair ) )
    {
        snp_row const *row1 = genotypes->get_row( pair.first );
        snp_row const *row2 = genotypes->get_row( pair.second );

        if( row1 == NULL || row2 == NULL )
        {
            continue;
        }

        std::fill( output, output + method_header.size( ), result_get_missing( ) );

        double statistic = method.run( *row1, *row2, output );
        if( threshold != -9 && (statistic == -9 || statistic > threshold) )
        {
            continue;
        }

        output[ method_header.size( ) - 1 ] = method.num_ok_samples( *row1, *row2 );

        result.write( pair, output );
    }

    delete[] output;
}
