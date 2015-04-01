import argparse
import random
from plinkio import plinkfile
from plink import generate
from math import sqrt

##
# Generates a normal phenotype for the given snp pair and mean
# matrix. 
#
# @param snp1 Genotype of first snp
# @param snp2 Genotype of second snp
# @param model Mean matrix as a length 9 list
# @param sd Common standard deviation
#
# @return Normal variable if not missing, None otherwise.
#
def generate_phenotype(snp1, snp2, model, sd):
    if snp1 != 3 and snp2 != 3:
        return random.normalvariate( model[ 3 * snp1 + snp2 ], sd )
    else:
        return None

def find_pair(loci, snp1, snp2):
    snp_to_index = dict( zip( map( lambda x: x.name, loci ), range( len( loci ) ) ) )
    return ( snp_to_index[ snp1 ], snp_to_index[ snp2 ] )

##
# Writes the normal phenotypes for two snps and a set of individuals to a file.
#
# @param sample_list List of plinkio.plinkfile.Sample.
# @param snp1_row List of genotypes for the first snp.
# @param snp2_row List of genotypes for the second snp.
# @param model Mean matrix as a length 9 list
# @param sd Common standard deviation
# @param output_file The phenotypes will be written to this file.
#
def write_phenotypes(sample_list, snp1_row, snp2_row, model, sd, output_file):
    output_file.write( "FID\tIID\tPheno\n" )

    num_samples = 0
    for sample, snp1, snp2 in zip( sample_list, snp1_row, snp2_row ):
        pheno = generate_phenotype( snp1, snp2, model, sd )
        pheno_str = "NA"
        if pheno != None:
            pheno_str = str( pheno )
            num_samples += 1

        output_file.write( "{0}\t{1}\t{2}\n".format( sample.fid, sample.iid, pheno_str ) )

    return num_samples

##
# Generates two distinct loci.
#
# @param loci List of locus.
#
# @return A list of two loci.
#
def generate_two_row_numbers(loci):
    loci_index = list( range( len( loci ) ) )
    random.shuffle( loci_index )

    return loci_index[:2]

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser( description='Generates interaction pairs from a given model.' )
    arg_parser.add_argument( 'plink_file', metavar='plink_file', type=str, help='Path to a plink file.' )
    arg_parser.add_argument( '--model', metavar='model', nargs=9, type=float, help='Space-separated list of floating point numbers that represents the mean matrix, specified row-wise from left to right.', default = None )
    arg_parser.add_argument( '--sd', metavar='sd', type=float, help='Common standard deviation', required = True )
    arg_parser.add_argument( '--random-model', metavar='random_model', nargs=2, type=generate.probability, help='Specifies the population penetrance and desired heritability (H^2) of the random models.', default = None )
    arg_parser.add_argument( '--pair', metavar='pair', nargs=2, type=str, help='Name of two SNPs for which the phenotype should be based on.', default = None )
    arg_parser.add_argument( '--out', metavar='output_file', help='Output phenotype file.', required = True )

    args = arg_parser.parse_args( )

    model = args.model
    if not args.model:
        if not args.random_model:
            model = [ -5.0 + 10.0 * random.random( ) for i in range( 9 ) ]
        else:
            p_d = args.random_model[ 0 ]
            H2 = args.random_model[ 1 ]

            model = [ random.normalvariate( p_d, sqrt( H2 * p_d * (1 - p_d) ) ) for i in range( 9 ) ]
    
    print model

    plink_file = plinkfile.open( args.plink_file ) 
    snp1_index, snp2_index = generate_two_row_numbers( plink_file.get_loci( ) )
    if args.pair:
        snp1_index, snp2_index = find_pair( plink_file.get_loci( ), args.pair[ 0 ], args.pair[ 1 ] )

    print plink_file.get_loci( )[ snp1_index ].name, plink_file.get_loci( )[ snp2_index ].name

    snp1_row = snp2_row = None
    for row_num, row in enumerate( plink_file ):
        if row_num == snp1_index:
            snp1_row = list( row )
        elif row_num == snp2_index:
            snp2_row = list( row )

    with open( args.out, "w" ) as output_file:
        number_of_samples = write_phenotypes( plink_file.get_samples( ), snp1_row, snp2_row, model, args.sd, output_file )
        print "Wrote {0} samples".format( number_of_samples ) 