// Author:  https://github.com/Tuxonomics
// Created: Aug, 2018
//
// Matrix abstraction for BLAS/LAPACK routines in C99. All matrices are of
// row-major ordering.

#include "mkl.h"


#define EPS 1E-10

#ifndef MAT_DECL



#define DEFAULT_MAJOR CblasRowMajor
#define NO_TRANS CblasNoTrans
#define TRANS CblasTrans
#define UPPER CblasUpper
#define LOWER CblasLower


// TODO(jonas): how to handle inf and nan? LAPACK routines do not handle them
// TODO(jonas): intel-based default allocator and arena for alignment
// TODO(jonas): make use of scratch buffer to support even larger matrices, and
// to not alter the input
// TODO(jonas): QR decomposition

#define MAT_DECL(type) typedef struct type##Mat type##Mat; \
    struct type##Mat { \
        u32 dim0; \
        u32 dim1; \
        type *data; \
    };

#define MAT_MAKE(type) type##Mat \
    type##MatMake(Allocator al, u32 dim0, u32 dim1) \
    { \
        type##Mat m; \
        m.dim0  = dim0; \
        m.dim1  = dim1; \
        m.data  = (type *) Alloc(al, dim0 * dim1 * sizeof(type)); \
        return m; \
    }

#define MAT_FREE(type) void \
    type##MatFree(Allocator al, type##Mat *m) \
    { \
        ASSERT(m->data); \
        Free(al, m->data); \
    }

#define MAT_PRINTLONG(type, baseFun) void \
    type##MatPrintLong(type##Mat m, const char* name) \
    { \
        printf("Mat (%s): {\n", name); \
        printf("\t.dim0 = %u\n", m.dim0); \
        printf("\t.dim1 = %u\n", m.dim1); \
        printf("\t.data = {\n\n"); \
        u32 dim; \
        for (u32 i=0; i<m.dim0; ++i) { \
            for (u32 j=0; j<m.dim1; ++j) { \
                dim = i*m.dim1 + j; \
                printf("[%d]\n", dim); \
                baseFun(m.data[dim]); \
                printf("\n"); \
            } \
            printf("\n"); \
        } \
        printf("\n\t}\n"); \
        printf("}\n"); \
    }

#define MAT_PRINT(type, baseFun) void \
    type##MatPrint(type##Mat m, const char* name) \
    { \
        printf("Mat (%s): {\n", name); \
        printf("\t.dim0 = %u\n", m.dim0); \
        printf("\t.dim1 = %u\n", m.dim1); \
        printf("\t.data = {\n\n"); \
        u32 dim; \
        for (u32 i=0; i<m.dim0; ++i) { \
            for (u32 j=0; j<m.dim1; ++j) { \
                dim = i*m.dim1 + j; \
                printf("[%d] ", dim); \
                baseFun(m.data[dim]); \
                printf("  "); \
            } \
            printf("\n"); \
        } \
        printf("\n\t}\n"); \
        printf("}\n"); \
    }

#define MATPRINT(type, x) type##MatPrint(x, #x)

#define MAT_EQUAL(type, equalFun) b32 \
    type##MatEqual( type##Mat a, type##Mat b, f64 eps ) \
    { \
        if ( a.dim0 != b.dim0 ) \
            return 0; \
        if ( a.dim1 != b.dim1 ) \
            return 0; \
        \
        for ( u32 i=0; i<(a.dim0 * a.dim1); ++i ) { \
            if ( ! equalFun( a.data[i], b.data[i], eps ) ) \
                return 0; \
        } \
        return 1; \
    }

#define MAT_ZERO(type) type##Mat \
    type##MatZeroMake(Allocator al, u32 dim0, u32 dim1) \
    { \
        type##Mat m = type##MatMake(al, dim0, dim1); \
        memset(m.data, 0, dim0 * dim1 * sizeof(type)); \
        return m; \
    }

#define MAT_SET(type) void \
    type##MatSet( type##Mat m, type val ) \
    { \
        for ( u32 i=0; i<(m.dim0 * m.dim1); ++i ) { \
            m.data[i] = val; \
        } \
    }

#define MAT_SETELEMENT(type) Inline void \
    type##MatSetElement(type##Mat m, u32 dim0, u32 dim1, type val) \
    { \
        ASSERT( m.data ); \
        ASSERT( dim0 <= m.dim0 ); \
        ASSERT( dim1 <= m.dim1 ); \
        m.data[dim0 * m.dim1 + dim1] = val; \
    }

#define MAT_GETELEMENT(type) Inline type \
    type##MatGetElement(type##Mat m, u32 dim0, u32 dim1) \
    { \
        ASSERT( dim0 <= m.dim0 ); \
        ASSERT( dim1 <= m.dim1 ); \
        return m.data[dim0 * m.dim1 + dim1]; \
    }

#define MAT_COPY(type, blas_prefix) Inline void \
    type##MatCopy( type##Mat src, type##Mat dst ) \
    { \
        ASSERT( (src.dim0 * src.dim1) == (dst.dim0 * dst.dim1) ); \
        cblas_##blas_prefix##copy( src.dim0 * src.dim1, src.data, 1, dst.data, 1); \
    }

#define MAT_SETCOL(type) Inline void \
    type##MatSetCol(type##Mat m, u32 dim, type##Mat newCol) \
    { \
        ASSERT( m.data ); \
        ASSERT( newCol.dim0 * newCol.dim1 == m.dim0 ); \
        \
        u32 idx = dim*m.dim1; \
        memcpy( &m.data[idx], newCol.data, sizeof( type ) * m.dim0 ); \
    }

#define MAT_GETCOL(type) Inline void \
    type##MatGetCol(type##Mat m, u32 dim, type##Mat newCol) \
    { \
        ASSERT( m.data ); \
        ASSERT( newCol.dim0 * newCol.dim1 == m.dim0 ); \
        \
        u32 idx = dim*m.dim1; \
        memcpy( newCol.data, &m.data[idx], sizeof( type ) * m.dim0 ); \
    }

#define MAT_SETROW(type) Inline void \
    type##MatSetRow(type##Mat m, u32 dim, type##Mat newCol) \
    { \
        ASSERT( m.data ); \
        ASSERT( newCol.dim0 * newCol.dim1 == m.dim0 ); \
        \
        for ( u32 i=0; i<m.dim1; ++i ) { \
            m.data[dim * m.dim1 + i] = newCol.data[i]; \
        } \
    }

#define MAT_GETROW(type) Inline void \
    type##MatGetRow(type##Mat m, u32 dim, type##Mat newCol) \
    { \
        ASSERT( m.data ); \
        ASSERT( newCol.dim0 * newCol.dim1 == m.dim0 ); \
        \
        for ( u32 i=0; i<m.dim1; ++i ) { \
            newCol.data[i] = m.data[dim * m.dim1 + i]; \
        } \
    }

/* vector euclidean norm */
#define MAT_VNORM(type, blas_prefix) Inline type \
    type##MatVNorm(type##Mat m) \
    { \
        ASSERT( m.dim0 == 1 || m.dim1 == 1 ); \
         \
        return cblas_##blas_prefix##nrm2( MAX(m.dim0, m.dim1), m.data, 1 ); \
    }

/* matrix scaling */
#define MAT_SCALE(type, blas_prefix) Inline void \
    type##MatScale( type##Mat m, type val, type##Mat dst ) \
    { \
        ASSERT( m.dim0 == dst.dim0 && m.dim1 == dst.dim1 ); \
        type##MatCopy( m, dst ); \
        cblas_##blas_prefix##scal( (dst.dim0 * dst.dim1), val, dst.data, 1); \
    }

/* matrix addition */
#define MAT_ADD(type, fun) Inline void \
    type##MatAdd( type##Mat a, type##Mat b, type##Mat c ) /* c = a + b */ \
    { \
        ASSERT(a.dim0 == b.dim0 && a.dim1 == b.dim1 && a.dim0 == c.dim0 && a.dim1 == c.dim1); \
        \
        for ( u32 i=0; i<(a.dim0*a.dim1); ++i ) { \
            c.data[i] = fun( a.data[i], b.data[i] ); \
        } \
    }

/* matrix subtraction */
#define MAT_SUB(type, fun) Inline void \
    type##MatSub( type##Mat a, type##Mat b, type##Mat c ) /* c = a - b */ \
    { \
        ASSERT(a.dim0 == b.dim0 && a.dim1 == b.dim1 && a.dim0 == c.dim0 && a.dim1 == c.dim1); \
        \
        for ( u32 i=0; i<(a.dim0*a.dim1); ++i ) { \
            c.data[i] = fun( a.data[i], b.data[i] ); \
        } \
    }

/* matrix operation with element-wise multiplication / division */
#define MAT_ELOP(type, suffix, fun) void \
    type##MatEl##suffix( type##Mat a, type##Mat b, type##Mat c ) /* c = a .* b */ \
    { \
        ASSERT(a.dim0 == b.dim0 && a.dim1 == b.dim1 && a.dim0 == c.dim0 && a.dim1 == c.dim1); \
        \
        for ( u32 i=0; i<(a.dim0*a.dim1); ++i ) { \
            c.data[i] = fun( a.data[i], b.data[i] ); \
        } \
    }

/* matrix multiplicaton */
#define MAT_MUL(type, blas_prefix) Inline void \
    type##MatMul( type##Mat a, type##Mat b, type##Mat c ) /* c = a * b */ \
    { \
        ASSERT(a.dim0 == c.dim0 && a.dim1 == b.dim0 && b.dim1 == c.dim1); \
         \
        /* lda, ldb, ldc - leading dimensions of matrices: number of columns in matrices */ \
        cblas_##blas_prefix##gemm ( \
            DEFAULT_MAJOR, NO_TRANS, NO_TRANS, \
            a.dim0, b.dim1, a.dim1, \
            1.0, \
            a.data, a.dim1, \
            b.data, b.dim1, \
            0, \
            c.data, b.dim1 \
        ); \
    }

/* matrix trace */
#define MAT_TRACE(type) type \
    type##MatTrace( type##Mat m ) /* out = trace(m) */ \
    { \
        ASSERT( m.dim0 == m.dim1 ); \
        type res = 0; \
        for ( u32 i = 0; i < m.dim0; ++i ) { \
            res += m.data[ i * (m.dim0 + 1) ]; \
        } \
        return res; \
    }

/* matrix transpose */
// TODO(jonas): use cache-aware transposition
#define MAT_T(type) void \
    type##MatT(type##Mat m, type##Mat dst) \
    { \
        ASSERT( m.dim0 == dst.dim1 && m.dim1 == dst.dim0 ); \
        \
        for ( u32 i=0; i<m.dim0; ++i ) { \
            for ( u32 j=0; j<m.dim1; ++j ) { \
                dst.data[ i * m.dim0 + j ] = m.data[ j * dst.dim0 + i ]; \
            } \
        } \
    }

/* matrix inverse */
// TODO(jonas): use scratch buffer
#define MAT_INV(type, lapack_prefix) i32 \
    type##MatInv(type##Mat m, type##Mat dst) \
    {\
        ASSERT( m.dim0 == m.dim1 && m.dim0 == dst.dim0 && m.dim1 == dst.dim1 ); \
        \
        type##Mat tmp = type##MatMake( DefaultAllocator, m.dim0, m.dim1 ); \
        type##MatCopy( m, tmp ); \
        u32 n = tmp.dim0; \
        i32 ipiv[n+1]; \
        \
        i32 ret = LAPACKE_##lapack_prefix##getrf( \
            DEFAULT_MAJOR, n, n, tmp.data, n, ipiv \
        ); \
        \
        if (ret !=0) { \
            return ret; \
        } \
        \
        ret = LAPACKE_dgetri( \
            DEFAULT_MAJOR, n, tmp.data, n, ipiv \
        ); \
        type##MatCopy( tmp, dst ); \
        type##MatFree( DefaultAllocator, &tmp ); \
        \
        return ret; \
    }

// TODO(jonas): maybe use QR decomposition for determinant to improve accuracy
// TODO(jonas): switch to scratch buffer
#define MAT_DET(type, lapack_prefix) type \
    type##MatDet(type##Mat m) \
    {\
        ASSERT( m.dim0 == m.dim1 ); \
        \
        type##Mat tmp = type##MatMake( DefaultAllocator, m.dim0, m.dim0 ); \
        type##MatCopy( m, tmp ); \
        \
        u32 n = tmp.dim0; \
        i32 ipiv[n+1]; \
        \
        i32 ret1 = LAPACKE_##lapack_prefix##getrf( \
            DEFAULT_MAJOR, n, n, tmp.data, n, ipiv \
        ); \
        \
        if (ret1 !=0) { \
            return 0; \
        } \
        type det = 1; \
        for ( u32 i=0; i<n; ++i ) { \
            det *= tmp.data[ i*n + i]; \
        } \
        u32 j; \
        f64 detp= 1; \
        for ( j=0; j<n; ++j ) { \
            if ( j+1 != ipiv[j] ) { \
                detp=-detp; \
            } \
        } \
        \
        type##MatFree( DefaultAllocator, &tmp ); \
        \
        return det * detp; \
    }

/* scale input vector by covariance matrix m, m can be upper triangular */
// TODO(jonas): switch to scratch buffer
#define MAT_COVSCALE(type, lapack_prefix) void \
    type##MatCovScale( type##Mat cov, type##Mat x, type##Mat dst ) \
    { \
        ASSERT( cov.dim0 == cov.dim1 ); \
        ASSERT( x.dim0 == 1 || x.dim1 == 1 ); \
        ASSERT( cov.dim0 == x.dim0 ); \
        ASSERT( x.dim0 == dst.dim0 && dst.dim1 == 1 ); \
        \
        type##Mat tmpC = type##MatMake( DefaultAllocator, cov.dim0, cov.dim0 ); \
        type##Mat tmpX = type##MatMake( DefaultAllocator, x.dim0,   x.dim1 ); \
        type##MatCopy( cov, tmpC ); \
        type##MatCopy( x, tmpX ); \
        \
        LAPACKE_##lapack_prefix##potrf( \
            DEFAULT_MAJOR, 'U', tmpC.dim0, tmpC.data, tmpC.dim0 \
        ); \
        \
        cblas_##lapack_prefix##trmv( \
            DEFAULT_MAJOR, CblasUpper, NO_TRANS, CblasNonUnit, \
            tmpC.dim0, tmpC.data, tmpC.dim0, \
            tmpX.data, 1 \
        ); \
        \
        type##MatCopy( tmpX, dst ); \
        \
        type##MatFree( DefaultAllocator, &tmpC ); \
        type##MatFree( DefaultAllocator, &tmpX ); \
    }

/* in-place covariance scaling of input vector */
#define MAT_COVSCALEIP(type, lapack_prefix) void \
    type##MatCovScaleIP( type##Mat cov, type##Mat x ) \
    { \
        ASSERT( cov.dim0 == cov.dim1 ); \
        ASSERT( x.dim0 == 1 || x.dim1 == 1 ); \
        ASSERT( cov.dim0 == x.dim0 ); \
        \
        type##Mat tmpC = type##MatMake( DefaultAllocator, cov.dim0, cov.dim0 ); \
        type##MatCopy( cov, tmpC ); \
        \
        LAPACKE_##lapack_prefix##potrf( \
            DEFAULT_MAJOR, 'U', tmpC.dim0, tmpC.data, tmpC.dim0 \
        ); \
        \
        cblas_##lapack_prefix##trmv( \
            DEFAULT_MAJOR, CblasUpper, NO_TRANS, CblasNonUnit, \
            tmpC.dim0, tmpC.data, tmpC.dim0, \
            x.data, 1 \
        ); \
        \
        type##MatFree( DefaultAllocator, &tmpC ); \
    }


#endif


MAT_DECL(f64);
MAT_MAKE(f64);
MAT_FREE(f64);
MAT_PRINTLONG(f64, f64Print);
MAT_PRINT(f64, f64Print);
MAT_EQUAL(f64, f64Equal);
MAT_ZERO(f64);
MAT_SET(f64);
MAT_SETELEMENT(f64);
MAT_GETELEMENT(f64);
MAT_COPY(f64, d);
MAT_SETCOL(f64);
MAT_GETCOL(f64);
MAT_SETROW(f64);
MAT_GETROW(f64);

MAT_SCALE(f64, d);             // tested
MAT_VNORM(f64, d);             // tested
MAT_ADD(f64, f64Add);          // tested
MAT_SUB(f64, f64Sub);          // tested
MAT_MUL(f64, d);               // tested
MAT_ELOP(f64, Mul, f64Mul);    // tested
MAT_ELOP(f64, Div, f64Div);    // tested
MAT_TRACE(f64);                // tested
MAT_T(f64);                    // tested
MAT_INV(f64, d);               // tested
MAT_DET(f64, d);               // tested

MAT_COVSCALE(f64, d);          // tested
MAT_COVSCALEIP(f64, d);        // tested


#if TEST
void test_scale()
{
    f64Mat a = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat b = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat c = f64MatMake( DefaultAllocator, 3, 3 );
    
    for ( u32 i=0; i<9; ++i ) {
        a.data[i] = (f64) i;
        b.data[i] = (f64) 2*i;
    }
    
    f64MatScale( a, 2, c );
    
    TEST_ASSERT( f64MatEqual( c, b, EPS ) );
}
#endif


#if TEST
void test_vnorm()
{
    f64Mat f = f64MatMake( DefaultAllocator, 9, 1 );
    
    f.data[0] = 0.3306201; f.data[1] = 0.6187407; f.data[2] = 0.6796355;
    f.data[3] = 0.4953877; f.data[4] = 0.9147741; f.data[5] = 0.3992435;
    f.data[6] = 0.5875585; f.data[7] = 0.4554847; f.data[8] = 0.8567403;
    
    TEST_ASSERT( f64Equal( f64MatVNorm(f), 1.86610966, 1E-7 ) );
    
    f64MatFree( DefaultAllocator, &f );
}
#endif


#if TEST
void test_add_sub_mul()
{
    f64Mat a = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat b = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat c = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat e = f64MatMake( DefaultAllocator, 3, 3 );
    
    for ( u32 i=0; i<9; ++i ) {
        a.data[i] = (f64) i;
        b.data[i] = (f64) 2*i;
    }
    
    // add
    f64MatAdd( a, b, c );

    e.data[0] = 0;  e.data[1] = 3;  e.data[2] = 6;
    e.data[3] = 9;  e.data[4] = 12; e.data[5] = 15;
    e.data[6] = 18; e.data[7] = 21; e.data[8] = 24;

    TEST_ASSERT( f64MatEqual( c, e, EPS ) );
    
    // sub
    f64MatSub( a, b, c );

    e.data[0] = 0;  e.data[1] = -1; e.data[2] = -2;
    e.data[3] = -3; e.data[4] = -4; e.data[5] = -5;
    e.data[6] = -6; e.data[7] = -7; e.data[8] = -8;

    TEST_ASSERT( f64MatEqual( c, e, EPS ) );
    
    // mul
    f64MatMul( a, b, c );

    e.data[0] = 30;  e.data[1] = 36;  e.data[2] = 42;
    e.data[3] = 84;  e.data[4] = 108; e.data[5] = 132;
    e.data[6] = 138; e.data[7] = 180; e.data[8] = 222;

    TEST_ASSERT( f64MatEqual( c, e, EPS ) );
    
    f64MatFree( DefaultAllocator, &a );
    f64MatFree( DefaultAllocator, &b );
    f64MatFree( DefaultAllocator, &c );
    f64MatFree( DefaultAllocator, &e );
}
#endif


#if TEST
void test_element_wise_ops()
{
    f64Mat a = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat b = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat c = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat e = f64MatMake( DefaultAllocator, 3, 3 );
    
    a.data[0] = 1;   a.data[1] = 0;   a.data[2] = 0.5;
    a.data[3] = 0;   a.data[4] = 0.5; a.data[5] = 0;
    a.data[6] = 0.5; a.data[7] = 0;   a.data[8] = 2;
    
    f64MatSet( b, 2.0 );
    
    // add
    f64MatElMul( a, b, c );
    
    e.data[0] = 2;  e.data[1] = 0; e.data[2] = 1;
    e.data[3] = 0;  e.data[4] = 1; e.data[5] = 0;
    e.data[6] = 1;  e.data[7] = 0; e.data[8] = 4;
    
    TEST_ASSERT( f64MatEqual( c, e, 1E-4 ) );
    
    // sub
    f64MatElDiv( a, b, c );
    
    e.data[0] = 0.5;  e.data[1] = 0;    e.data[2] = 0.25;
    e.data[3] = 0;    e.data[4] = 0.25; e.data[5] = 0;
    e.data[6] = 0.25; e.data[7] = 0;    e.data[8] = 1;
    
    TEST_ASSERT( f64MatEqual( c, e, EPS ) );
    
    f64MatFree( DefaultAllocator, &a );
    f64MatFree( DefaultAllocator, &b );
    f64MatFree( DefaultAllocator, &c );
    f64MatFree( DefaultAllocator, &e );
}
#endif


#if TEST
void test_trace()
{
    f64Mat c = f64MatZeroMake( DefaultAllocator, 3, 3 );
    
    for ( u32 i=0; i<c.dim0; ++i )
        c.data[ i * c.dim0 + i ] = (f64) i + 0.5;
    
    TEST_ASSERT( f64Equal( f64MatTrace(c), 4.5, EPS ) );
    
    f64MatFree( DefaultAllocator, &c );
}
#endif


#if TEST
void test_inverse()
{
    f64Mat c = f64MatZeroMake( DefaultAllocator, 3, 3 );
    f64Mat e = f64MatZeroMake( DefaultAllocator, 3, 3 );
    f64Mat f = f64MatMake( DefaultAllocator, 3, 3 );
    
    for ( u32 i=0; i<c.dim0; ++i )
        c.data[ i * c.dim0 + i ] = (f64) i + 0.5;
    
    f64MatInv( c, f );
    
    for ( u32 i=0; i<e.dim0; ++i )
        e.data[ i * e.dim0 + i ] = 1 / ((f64) i + 0.5);
    
    TEST_ASSERT( f64MatEqual( f, e, EPS ) );
    
    f64MatFree( DefaultAllocator, &c );
    f64MatFree( DefaultAllocator, &e );
    f64MatFree( DefaultAllocator, &f );
}
#endif


#if TEST
void test_transpose()
{
    f64Mat a = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat b = f64MatMake( DefaultAllocator, 3, 3 );
    f64Mat c = f64MatMake( DefaultAllocator, 3, 3 );
    
    for ( u32 i=0; i<9; ++i ) {
        a.data[i] = (f64) i;
    }
    for ( u32 i=0; i<a.dim0; ++i ) {
        for ( u32 j=0; j<a.dim1; ++j ) {
            b.data[ j*b.dim0 + i ] = a.data[ i*a.dim0 + j ];
        }
    }
    
    f64MatT( a, c );
    
    TEST_ASSERT( f64MatEqual( c, b, EPS ) );
}
#endif


#if TEST
void test_determinant()
{
    f64Mat f = f64MatMake( DefaultAllocator, 3, 3 );

    f.data[0] = 0.3306201;
    f.data[1] = 0.6187407;
    f.data[2] = 0.6796355;
    f.data[3] = 0.4953877;
    f.data[4] = 0.9147741;
    f.data[5] = 0.3992435;
    f.data[6] = 0.5875585;
    f.data[7] = 0.4554847;
    f.data[8] = 0.8567403;

    TEST_ASSERT( f64Equal( f64MatDet(f), -0.130408, 1E-6 ) );

    f64MatFree( DefaultAllocator, &f );
}
#endif


#if TEST
void test_covariance_scaling()
{
    f64Mat f = f64MatMake( DefaultAllocator, 3, 3 );
    
    f.data[0] = 1; f.data[1] = 0;   f.data[2] = 0.5;
    f.data[3] = 0; f.data[4] = 0.5; f.data[5] = 0;
    f.data[6] = 0; f.data[7] = 0;   f.data[8] = 2;
    
    f64Mat x       = f64MatMake( DefaultAllocator, 3, 1 );
    f64Mat xScaled = f64MatMake( DefaultAllocator, 3, 1 );

    x.data[0] = 1; x.data[1] = 2; x.data[2] = 3;

    f64MatCovScale( f, x, xScaled );

    x.data[0] = 2.500000; x.data[1] = 1.414214; x.data[2] = 3.968627;
    
    TEST_ASSERT( f64MatEqual( x, xScaled, 1E-5 ) );
    
    x.data[0] = 1; x.data[1] = 2; x.data[2] = 3;
    
    f64MatCovScaleIP( f, x );
    
    TEST_ASSERT( f64MatEqual( x, xScaled, 1E-5 ) );
}
#endif



MAT_DECL(i32);
MAT_MAKE(i32);
MAT_FREE(i32);
MAT_PRINT(i32, i32Print);


// TODO(jonas): library initialization

void InitializeMatrices( u32 numThreads, char threadScope )
{
    
    switch( threadScope ) {
        case 'g':
        case 'G': {
            MKL_Set_Num_Threads( numThreads );
            break;
        }
        case 'l':
        case 'L': {
            MKL_Set_Num_Threads_Local( numThreads );
            break;
        }
        default: {
            
        }
    }
    
}


#undef EPS


