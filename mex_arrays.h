#ifndef __MEX_ARRAYS__
#define __MEX_ARRAYS__

//=================================================
// @file         mex_arrays.h
// @author       Jonathan Hadida, Oxford DTC
// @contact      Jonathan.hadida [at] dtc.ox.ac.uk
//=================================================

#include <iostream>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

#include <exception>
#include <stdexcept>

#include <memory>
#include <algorithm>
#include <type_traits>

#include "mex.h"

#define MEX_ARRAY_SAFE_ACCESS

// Protect 1D access
#ifdef MEX_ARRAY_SAFE_ACCESS
#define MEX_ARRAY_PROTECT(k,n) (k % n)
#else
#define MEX_ARRAY_PROTECT(k,n) k
#endif



        /********************     **********     ********************/
        /********************     **********     ********************/



/**
 * Convert numeric types to mex types.
 */
template <typename T>
struct mx_type
{
	static const char *name;
	static const mxClassID id;
};

// ------------------------------------------------------------------------

template <> const char* mx_type<char>::name = "int8";
template <> const char* mx_type<unsigned char>::name = "uint8";
template <> const char* mx_type<short>::name = "int16";
template <> const char* mx_type<unsigned short>::name = "uint16";
template <> const char* mx_type<int>::name = "int32";
template <> const char* mx_type<unsigned int>::name = "uint32";
template <> const char* mx_type<float>::name = "single";
template <> const char* mx_type<double>::name = "double";

template <> const mxClassID mx_type<char>::id = mxINT8_CLASS;
template <> const mxClassID mx_type<unsigned char>::id = mxUINT8_CLASS;
template <> const mxClassID mx_type<short>::id = mxINT16_CLASS;
template <> const mxClassID mx_type<unsigned short>::id = mxUINT16_CLASS;
template <> const mxClassID mx_type<int>::id = mxINT32_CLASS;
template <> const mxClassID mx_type<unsigned int>::id = mxUINT32_CLASS;
template <> const mxClassID mx_type<float>::id = mxSINGLE_CLASS;
template <> const mxClassID mx_type<double>::id = mxDOUBLE_CLASS;

// ------------------------------------------------------------------------

template <> const char* mx_type<const char>::name = "int8";
template <> const char* mx_type<const unsigned char>::name = "uint8";
template <> const char* mx_type<const short>::name = "int16";
template <> const char* mx_type<const unsigned short>::name = "uint16";
template <> const char* mx_type<const int>::name = "int32";
template <> const char* mx_type<const unsigned int>::name = "uint32";
template <> const char* mx_type<const float>::name = "single";
template <> const char* mx_type<const double>::name = "double";

template <> const mxClassID mx_type<const char>::id = mxINT8_CLASS;
template <> const mxClassID mx_type<const unsigned char>::id = mxUINT8_CLASS;
template <> const mxClassID mx_type<const short>::id = mxINT16_CLASS;
template <> const mxClassID mx_type<const unsigned short>::id = mxUINT16_CLASS;
template <> const mxClassID mx_type<const int>::id = mxINT32_CLASS;
template <> const mxClassID mx_type<const unsigned int>::id = mxUINT32_CLASS;
template <> const mxClassID mx_type<const float>::id = mxSINGLE_CLASS;
template <> const mxClassID mx_type<const double>::id = mxDOUBLE_CLASS;



        /********************     **********     ********************/
        /********************     **********     ********************/



/**
 * Convert nd coordinates to 1d index.
 */
template <unsigned N>
unsigned sub2ind( const unsigned *subs, const unsigned *size, const unsigned *strides )
{
	register unsigned ind = 0;

	for (unsigned i = 0; i < N; ++i) 
	{
#ifdef MEX_ARRAY_SAFE_ACCESS
	ind += ( subs[i] % size[i] ) * strides[i];
#else
	ind += subs[i] * strides[i];
#endif
	}

	return ind;
}

template <unsigned N>
unsigned sub2ind( va_list& vl, const unsigned *size, const unsigned *strides )
{
	register unsigned ind = 0;

	for (unsigned i = 1; i < N; ++i) 
	{
#ifdef MEX_ARRAY_SAFE_ACCESS
	ind += ( va_arg(vl,unsigned) % size[i] ) * strides[i];
#else
	ind += va_arg(vl,unsigned) * strides[i];
#endif
	}

	va_end(vl); return ind;
}

template <> inline unsigned 
sub2ind<0>( const unsigned*, const unsigned*, const unsigned* )
	{ return 0; }
template <> inline unsigned 
sub2ind<1>( const unsigned *subs, const unsigned*, const unsigned* )
	{ return *subs; }
template <> inline unsigned 
sub2ind<2>( const unsigned *subs, const unsigned *size, const unsigned* )
	{ return (subs[0] + subs[1]*size[0]); }

// ------------------------------------------------------------------------

/**
 * Define a singleton to handle access errors safely.
 */
template <typename T> struct singleton { static T instance; };
template <typename T> T singleton<T>::instance = T();

// ------------------------------------------------------------------------

/**
 * Create fake array to protect against access of empty container.
 */
template <typename T>
struct fake_array
{
	inline T& operator[] (unsigned n) const { return singleton<T>::instance; }
};

// ------------------------------------------------------------------------

/**
 * 
 */
template <typename T>
struct no_delete
{ inline void operator() ( T* ptr ) const { ptr = nullptr; } };



        /********************     **********     ********************/
        /********************     **********     ********************/



/**
 * nd array class.
 */
template <typename T, unsigned N>
class ndArray
{
public:

	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;
	typedef std::shared_ptr<value_type> shared;
	typedef ndArray<T,N> self;



	static constexpr bool is_mutable = !std::is_const<T>::value;

	// Constructors
	ndArray() { clear(); }
	ndArray( const mxArray *A ) { assign(A); }
	ndArray( pointer ptr, const unsigned *size, bool manage ) { assign(ptr,size,manage); }


	// Copy constructor
	ndArray( const self& other );

	// Check validity
	inline bool empty() const { return !((bool) m_data); }
	inline operator bool() const { return m_data; }

	// Print info
	void info() const;



	// ------------------------------------------------------------------------
	


	// Clear contents
	void clear();
	void reset();


	// Assign
	void assign( const mxArray *A );
	void assign( pointer ptr, const unsigned *size, bool manage );


	// Copy from other instance
	template <typename U>
	void copy( const ndArray<U,N>& other );



	// ------------------------------------------------------------------------



	// 1D access
	reference operator[] ( unsigned n ) const 
		{ return data()[MEX_ARRAY_PROTECT(n,N)]; }

	// ND access
	reference operator[] ( const unsigned *subs ) const
		{ return data()[ sub2ind<N>(subs, m_size, m_strides) ]; }

	// Coordinates access
	reference operator() ( unsigned i, ... ) const
		{ 
			va_list vl; va_start(vl,i); 
			return data()[ i+sub2ind<N>(vl, m_size, m_strides) ]; 
		}


	// Access data directly
	inline pointer data() const { return m_data.get(); }


	// Iterators
	inline pointer begin() { return data(); }
	inline pointer end() { return data() + m_numel; }



	// ------------------------------------------------------------------------



	// Access dimensions
	inline const unsigned* size() const { return m_size; }
	inline unsigned size( unsigned n ) const { return m_size[ n % N ]; }
	inline const unsigned* strides() const { return m_strides; }
	inline unsigned stride( unsigned n ) const { return m_strides[ n % N ]; }
	inline unsigned numel() const { return m_numel; }



private:

	void assign_shared( pointer ptr, bool manage );

	unsigned m_numel;
	unsigned m_size[N];
	unsigned m_strides[N];

	shared m_data;
};



        /********************     **********     ********************/
        /********************     **********     ********************/



// Include implementation
#include "mex_arrays.hpp"

#endif