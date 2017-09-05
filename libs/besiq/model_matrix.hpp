#ifndef __MODEL_MATRIX_H__
#define __MODEL_MATRIX_H__

#include <armadillo>

#include <plink/snp_row.hpp>

class model_matrix
{
    public:
        virtual ~model_matrix(){ };
        /**
         * Updates the model matrix with the following genotypes.
         */
        virtual void update_matrix(const snp_row &row1, const snp_row &row2, arma::uvec &missing) = 0;

        /**
         * Returns the model matrix.
         */
        virtual const arma::mat &get_alt() = 0;
        virtual const arma::mat &get_null() = 0;
        virtual size_t num_df() = 0;
        virtual size_t num_alt() = 0;
        virtual size_t num_null() = 0;
};

class general_matrix : public model_matrix
{
public:
    general_matrix(const arma::mat &cov, size_t n, size_t num_null, size_t num_alt);
    virtual ~general_matrix();
    virtual const arma::mat &get_alt();
    virtual const arma::mat &get_null();
    virtual size_t num_df();
    virtual size_t num_alt();
    virtual size_t num_null();
    virtual void update_matrix(const snp_row &row1, const snp_row &row2, arma::uvec &missing) = 0;

protected:
    arma::mat m_alt;
    arma::mat m_null;
    size_t m_num_alt;
    size_t m_num_null;
};

class additive_matrix : public general_matrix
{
public:
    additive_matrix(const arma::mat &cov, size_t n);
    void update_matrix(const snp_row &row1, const snp_row &row2, arma::uvec &missing);
};

class tukey_matrix : public general_matrix
{
public:
    tukey_matrix(const arma::mat &cov, size_t n);
    void update_matrix(const snp_row &row1, const snp_row &row2, arma::uvec &missing);
};

class factor_matrix : public general_matrix
{
public:
    factor_matrix(const arma::mat &cov, size_t n);
    void update_matrix(const snp_row &row1, const snp_row &row2, arma::uvec &missing);
};

class noia_matrix : public general_matrix
{
public:
    noia_matrix(const arma::mat &cov, size_t n);
    void update_matrix(const snp_row &row1, const snp_row &row2, arma::uvec &missing);
};

typedef enum 
{
    DOM_DOM = 0,
    REC_DOM = 1,
    DOM_REC = 2,
    REC_REC = 3
} separate_mode_t;

class separate_matrix : public general_matrix
{
public:
    separate_matrix(const arma::mat &cov, size_t n, separate_mode_t mode);
    void update_matrix(const snp_row &row1, const snp_row &row2, arma::uvec &missing);
private:
    int m_snp1_threshold;
    int m_snp2_threshold;
};

model_matrix *make_model_matrix(const std::string &type, const arma::mat &cov, size_t n);

class env_matrix
{
public:
    env_matrix(const arma::mat &cov, size_t n);
    const arma::mat &get_alt();
    void update_matrix(const snp_row &row, const arma::vec &env, arma::uvec &missing);
    
private:
    arma::mat m_alt;
};

#endif /* __MODEL_MATRIX_H__ */
