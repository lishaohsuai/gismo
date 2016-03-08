/** @file gsIntegerGridIterator.h

    @brief Provides iteration over integer or numeric points in a (hyper-)cube

    This file is part of the G+Smo library. 

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    
    Author(s): A. Mantzaflaris
*/

#pragma once


namespace gismo
{

/**
   \brief Specifies aliases for modes for gsGridIterator
*/
    enum gsGridIteratorMode
    {
        CUBE   = 0,
        BDR    = 1,
        VERTEX = 2,
        CWISE  = 3
    };
    
// note: default arguments are found in gsForwardDeclarations.h
template<typename Z, int mode, int d, bool> class gsGridIterator { };


/** 
    \brief Iterator over the Cartesian product of integer points in a
    tensor-product grid.
    
    The iteration is done in lexicographic order.

    - mode = 0 : iteration over [a, b) or [a, b]
    gsGridIterator<T,CUBE> grid(a,b);

    - mode = 1 : iteration over the boundry points of [a, b) or [a, b]
    gsGridIterator<T,BDR> grid(a,b);
        
    - mode = 2 : iteration over the vertices of [a, b) or [a, b]
    gsGridIterator<T,VERTEX> grid(a,b);

    
    The open or closed case is determined by a constructor flag.
    A third argument, eg.  gsGridIterator<T,CUBE,d> grid(a,b);
    specifies fixed size dimension \a d
    
    \note Iteration over the boundary including offsets is possible
    using the free functions in gsUtils/gsCombinatorics.h

    \tparam Z type of the integer coordinates of the index vector

    \tparam d statically known dimension, or dynamic dimension if d =
    -1 (default value)

    \tparam mode 0: all integer points in [a,b], 1: all points in
    [a,b), 2: vertices of cube [a,b]

    \ingroup Tensor
*/
template<class Z, int mode, int d>
class gsGridIterator<Z,mode,d,true>
{
public:
    typedef gsVector<Z,d> point;

public:

    /**
       \brief Empty constructor
     */
    gsGridIterator()
    {
        GISMO_STATIC_ASSERT(std::numeric_limits<Z>::is_integer,INCONSISTENT_INSTANTIZATION);
        GISMO_STATIC_ASSERT(mode > -1 && mode<3, INCONSISTENT_INSTANTIZATION);
    }

    /**
       \brief Constructor using lower and upper limits

       \param a lower limit (vertex of a cube)
       \param b upper limit (vertex of a cube)
       \param open if true, the iteration is performed for the points in $[a_i,b_i)$
     */
    gsGridIterator(point const & a, point const & b, bool open = true)
    { reset(a, b, open); }

    /**
       \brief Constructor using upper limit. The iteration starts from zero.

       \param b upper limit (vertex of a cube)
       \param open if true, the iteration is performed for the points in $[0,b_i)$
     */
    explicit gsGridIterator(point const & b, bool open = true)
    { reset(point::Zero(b.size()), b, open); }

    /**
       \brief Constructor using lower and upper limits

       \param ab Matrix with two columns, corresponding to lower and upper limits
       \param open if true, the iteration is performed for the points in $[a_i,b_i)$
     */
    explicit gsGridIterator(gsMatrix<Z,d,2> const & ab, bool open = true)
    { reset(ab.col(0), ab.col(1), open); }

    /**
       \brief Resets the iterator using new lower and upper limits

       \param a lower limit (vertex of a cube)
       \param b upper limit (vertex of a cube)
       \param open if true, the iteration is performed for the points in $[a_i,b_i)$
     */    
    inline void reset(point const & a, point const & b, bool open = true)
    {
        GISMO_ASSERT(a.rows() == b.rows(), "Invalid endpoint dimensions");
        m_low = m_cur = a;
        if (open) m_upp = b.array() - 1; else m_upp = b;
        m_dim = ( (m_low.array() <= m_upp.array()).all() ? a.rows() : 0 );
    }

    /**
       \brief Resets the iterator, so that a new iteration over the
       points may start
     */
    void reset() { reset(m_low,m_upp, false); }

    // See http://eigen.tuxfamily.org/dox-devel/group__TopicStructHavingEigenMembers.html
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF( (sizeof(point)%16)==0 );

public:

    operator bool() const {return 0!=m_dim;}
    
    const point & operator*() const {return m_cur;}

    const point * operator->() const {return &m_cur;}
    
    inline gsGridIterator & operator++()
    {
        // Multiple implementations according to mode
        switch (mode)
        {
        case 0: // ----------- Iteration over [m_low, m_upp]
        case 2: // iteration over vertices of the cube [m_low, m_upp]
            for (int i = 0; i != (d==-1?m_dim:d); ++i)
                if ( m_cur[i] != m_upp[i] )
                {
                    if (0==mode) ++m_cur[i]; else m_cur[i] = m_upp[i];
                    return *this;
                }
                else
                    m_cur[i] = m_low[i];
            m_dim = 0;//done
            return *this;

        case 1: // ----------- Iteration over boundary of [m_low, m_upp]
            for (int i = 0; i != (d==-1?m_dim:d); ++i)
            {        
                if ( m_cur[i] != m_upp[i] )
                {
                    if ( m_cur[i] == m_low[i] && ( i+1!=m_dim || m_dim==1) )
                    {
                        int c = i+1;
                        for (int j = c; j!=(d==-1?m_dim:d); ++j)
                            if ( (m_cur[j] == m_low[j]) || 
                                 (m_cur[j] == m_upp[j]) )
                                ++c;
                        
                        if ( c==1 )
                            m_cur[i] = m_upp[i];
                        else
                            ++m_cur[i];
                    }
                    else
                        m_cur[i]++;
                    
                    for (int k = i-1; k!=-1; --k)
                        m_cur[k] = m_low[k];
                    return *this;
                }
            }
            /*fall through to default*/
        default:
            m_dim = 0;//done
            return *this;
        }
    }

    /**
       \brief Returns true if the \a i-th coordinate has minimal value
     */
    inline bool isFloor(int i) const { return m_cur[i] == m_low[i];}

    /**
       \brief Returns true if the \a i-th coordinate has maximal value
     */
    inline bool isCeil (int i) const
    { return mode ? m_cur[i] == m_upp[i] : m_cur[i] + 1 == m_upp[i];}

    /**
       \brief Returns true if the current point lies on a boundary
     */
    bool isBoundary() const
    {
        return (m_cur.array() == m_low.array()).any() ||
            (m_cur.array() == m_upp.array()).any() ;
    }

    /**
       \brief Returns the total number of points that are iterated
     */
    index_t numPoints() const
    {
        switch (mode)
        {
        case 0: return ((m_upp-m_low).array() + 1).prod();
        case 1: return ((m_upp-m_low).array() + 1).prod() - ((m_upp-m_low).array()-1).prod();
        case 2: return ( 1 << m_low.rows() );
        default: GISMO_ERROR("gsGridIterator::numPoints");
        }
    }

    /**
       \brief Returns the total number of points per coordinate which
       are iterated
    */
    point numPointsCwise() const { return (m_upp-m_low).array() + 1;}

    /**
       \brief Returns the first point in the iteration
    */
    const point & lower() const {return m_low;}

    /**
       \brief Returns the last point in the iteration
    */
    const point & upper() const {return m_upp;}

    /**
       \brief Utility function which returns the vector of strides

       \return An integer vector (stride vector) with the property
       that when we add it to *this we obtain the next point to be
       iterated. Moreover, the dot product of *this with the stride
       vector results in the "flat index", i.e. the lexicographic
       index of the point

    */
    point strides() const
    {
        point res(m_low.rows());
        res[0] = 1;
        for (index_t i = 1; i != res.size(); ++i )
            res[i] = res[i-1] * ( m_upp[i-1] - m_low[i-1] + 1 );
        return res;
    }

private:
    
    point m_low, m_upp; ///< Iteration lower and upper limits
    point  m_cur;       ///< Current point pointed at by the iterator
    int m_dim;          ///< Dimension (number of rows) of the iterated points
};


/** 
    \brief Iterator over a Cartesian product of uniformly distributed
    numeric points inside a (hyper-)cube.

    The iteration is done in the natural lexicographic order.

    - mode = 0 : iteration over uniform samples in [a, b]
    gsGridIterator<T,CUBE> grid(a,b);

    - mode = 1 : iteration over uniform samples on the boundary points [a, b]
    gsGridIterator<T,BDR> grid(a,b);

    - mode = 2 : iteration over the vertices [a, b]
    gsGridIterator<T,VERTEX> grid(a,b);

    A third argument, eg.  gsGridIterator<T,CUBE,d> grid(a,b);
    specifies fixed size dimension \a d.

    \tparam T type of the numeric coordinates of the index vector

    \tparam d statically known dimension, or dynamic dimension if d =
    -1 (default value)

    \tparam mode 0: all sample points in [a,b], 1: all points in
    [a,b), 2: vertices of cube [a,b]

    \ingroup Tensor
*/
template<class T, int mode, int d>
class gsGridIterator<T,mode,d,false>
{
public:

    typedef gsVector<T,d> point;

    typedef gsGridIterator<index_t, mode, d> integer_iterator;

    typedef typename integer_iterator::point point_index;
public:

    /**
       \brief Constructor using lower and upper limits

       Uniformly sampled points will be generated

       \param a lower limit (vertex of a cube)
       \param b upper limit (vertex of a cube)
       \param np number of sample points per coordinate
    */
    gsGridIterator(point const & a, 
                   point const & b, 
                   point_index const & np)
    : m_iter(np, 1)
    {
        reset(a, b);
        GISMO_STATIC_ASSERT(mode > -1 && mode<3, INCONSISTENT_INSTANTIZATION);
    }

    /**
       \brief Constructor using lower and upper limits
     
       Uniformly sampled points will be generated

       \param ab Matrix with two columns, corresponding to lower and upper limits  
       \param np number of sample points per coordinate
    */
    gsGridIterator(gsMatrix<T,d,2> const & ab, 
                   point_index const & np)
    : m_iter(np, 1)
    {
        reset(ab.col(0), ab.col(1));
    }

        /**
       \brief Constructor using lower and upper limits
     
       Uniformly sampled points will be generated. The number of the
       points is approximately \a numPoints

       \param ab Matrix with two columns, corresponding to lower and upper limits  
       \param numPoints the number sample points to be used (in total, approximately)
    */
    gsGridIterator(gsMatrix<T,d,2> const & ab, unsigned numPoints)
    : m_low(ab.col(0)), m_upp(ab.col(1))
    {
        // deduce the number of points per direction
        const point span = ab.col(1) - ab.col(0);
        const point wght = span / span.sum();
        const T h = math::pow( span.prod() / numPoints, 1.0 / (d!=-1?d:ab.rows()) );
        point_index npts(ab.rows());
        for (index_t i = 0; i != (d!=-1?d:ab.rows()); ++i)
            npts[i] = cast<T,index_t>(math::ceil( span[i] / (h*wght[i]) ) );
        m_iter = integer_iterator(npts, 1);

        reset(ab.col(0), ab.col(1));
    }

    /**
       \brief Resets the iterator, so that a new iteration over the
       points may start
     */
    void reset() { m_cur = m_low; m_iter.reset(); }

    /**
       \brief Resets the iterator using new lower and upper limits
       
       \param a lower limit (vertex of a cube)
       \param b upper limit (vertex of a cube)
     */    
    void reset(point const & a, 
               point const & b)
    {
        m_cur = m_low = a;
        m_upp = b;
        m_step = (b-a).array() / (m_iter.numPointsCwise().array() - 1)
            .matrix().cwiseMax(1).template cast<T>().array() ;
        m_iter.reset();
    }

    // See http://eigen.tuxfamily.org/dox-devel/group__TopicStructHavingEigenMembers.html
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF( (sizeof(point)%16)==0 );

public:

    operator bool() const {return m_iter;}
    
    inline gsGridIterator & operator++()
    {
        if ( ++m_iter )
        {
            for (int i = 0; i != (d==-1?m_low.size():d); ++i)
                if ( m_iter.isFloor(i) )
                    m_cur.at(i) = m_low[i]; // avoid numerical error at first val
                else if ( m_iter.isCeil(i) )
                    m_cur.at(i) = m_upp[i]; // avoid numerical error at last val
                else
                    m_cur.at(i) = m_low[i] + m_iter->at(i) * m_step[i];
        }
        return *this;        
    }

    
    const gsMatrix<T> & operator*() const {return m_cur;}

    const gsMatrix<T> * operator->() const {return &m_cur;}

    /**
       \brief Returns true if the \a i-th coordinate has minimal value
    */
    inline bool isFloor(int i) const { return m_iter.isFloor(i);}

    /**
       \brief Returns true if the \a i-th coordinate has maximal value
    */
    inline bool isCeil (int i) const { return m_iter.isCeil(i);}

    /**
       \brief Returns true if the current point lies on a boundary
     */
    inline bool isBoundary() const { return m_iter.isBoundary();}

    /**
       \brief Returns the total number of points that are iterated
    */
    index_t numPoints() const {return m_iter.numPoints();}
    
    /**
       \brief Returns the total number of points per coordinate which
       are iterated
    */
    point_index numPointsCwise() const {return m_iter.numPointsCwise();}

    /**
       \brief Returns the first point in the iteration
    */
    const point & lower() const {return m_low;}

    /**
       \brief Returns the last point in the iteration
    */
    const point & upper() const {return m_upp;}

    /**
       \brief Returns the tensor index of the current point
    */
    const point_index & tensorIndex() const { return *m_iter;}

    /**
       \brief Returns the coordinate-wise stepping between the samples
    */
    const point & step() const {return m_step;}

    /**
       \brief Returns a reference to the underlying integer lattice
       iterator
    */    
    const integer_iterator & index_iterator() const { return m_iter;}
    
private:

    point  m_low, m_upp, m_step; ///< Iteration lower and upper limits and stepsize
    integer_iterator m_iter;     ///< Underlying integer lattice iterator
    gsMatrix<T>      m_cur;      ///< Current point pointed at by the iterator
};


/** 
    \brief Iterator over a Cartesian product of points, which is
    given by coordinate-wise point sets

    The iteration is done in lexicographic order. Usage:

    gsGridIterator<T,CWISE> grid(a,b);

    or 

    gsGridIterator<T,CWISE,d> grid(a,b);

    \tparam T type of the numeric coordinates of the index vector

    \tparam d statically known dimension, or dynamic dimension if d =
    -1 (default value)

    \ingroup Tensor
*/
template<class T, int d>
class gsGridIterator<T,3,d,false>
{
public:

    typedef gsGridIterator<index_t, 0, d> integer_iterator;

    typedef typename integer_iterator::point point_index;

    typedef std::vector<gsMatrix<T> > CwiseContainer;
    
public:

    /**
       \brief Constructor using references to the coordinate vectors
       
       \param cwise A list of matrices, each containing the sample
       points in the respective coordinate
    */
    explicit gsGridIterator(const CwiseContainer & cwise)
    : m_cwise(cwise)
    {
        gsVector<index_t,d> npts(cwise.size());
        for (index_t i = 0; i != npts.rows(); ++i)
        {
            npts[i] = cwise[i].rows() - 1;
            GISMO_ASSERT(cwise[i].cols()==1 || cwise[i].rows()==1, "Invalid input");
        }
        m_iter = integer_iterator(npts, 0);
        if (1==cwise.front().rows())
            m_cur.derived().resize(cwise.size(),1);
        else
            m_cur.derived().resize(1,cwise.size());
        update();
    }

    /**
       \brief Resets the iterator, so that a new iteration over the
       points may start
    */
    void reset() { m_iter.reset(); update();}

public:

    operator bool() const {return m_iter;}
    
    inline gsGridIterator & operator++()
    {
        if (++m_iter) update();
        return *this;
    }
    
    const gsMatrix<T> & operator*() const {return m_cur;}

    const gsMatrix<T> * operator->() const {return &m_cur;}

    /**
       \brief Returns true if the \a i-th coordinate has minimal value
    */
    inline bool isFloor(int i) const { return m_iter.isFloor(i);}

    /**
       \brief Returns true if the \a i-th coordinate has maximal value
    */
    inline bool isCeil (int i) const { return m_iter.isCeil(i);}

    /**
       \brief Returns true if the current point lies on a boundary
     */
    inline bool isBoundary() const { return m_iter.isBoundary();}

    /**
       \brief Returns the total number of points that are iterated
    */
    index_t numPoints() const {return m_iter.numPoints();}
    
    /**
       \brief Returns the total number of points per coordinate which
       are iterated
    */
    point_index numPointsCwise() const {return m_iter.numPointsCwise();}

    /**
       \brief Returns the tensor index of the current point
    */
    const point_index & tensorIndex() const { return *m_iter;}

    /**
       \brief Returns a reference to the underlying integer lattice
       iterator
    */    
    const integer_iterator & index_iterator() const { return m_iter;}

private:

    // Update the point to the current iterator position
    inline void update()
    {
        for (index_t i = 0; i != m_cur.rows(); ++i)
            m_cur.at(i) = m_cwise[i].at(m_iter->at(i));
    }

private:

    const CwiseContainer & m_cwise; ///< List of coordinate-wise values
    integer_iterator m_iter;        ///< Underlying integer lattice iterator
    gsMatrix<T>      m_cur;         ///< Current point pointed at by the iterator
};


} // namespace gismo
