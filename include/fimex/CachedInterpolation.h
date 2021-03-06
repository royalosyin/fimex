/*
 * Fimex
 *
 * (C) Copyright 2008, met.no
 *
 * Project Info:  https://wiki.met.no/fimex/start
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef CACHEDINTERPOLATION_H_
#define CACHEDINTERPOLATION_H_

#include <boost/shared_array.hpp>
#include "fimex/interpolation.h"
#include "fimex/DataDecl.h"
#include "fimex/SliceBuilder.h"

namespace MetNoFimex
{

//forward decl.
class CDMReader;

/**
 * @headerfile fimex/CachedInterpolation.h
 */
/**
 * Struct to store information used to create a slicebuilder limiting the amount of input data.
 */
struct ReducedInterpolationDomain {
    // name of x-dimension
    std::string xDim;
    // name of y-dimension
    std::string yDim;
    // offset on x-axis
    size_t xMin;
    // original complete size of x-axis
    size_t xOrg;
    // offset on y-axis
    size_t yMin;
    // original complete size of y-axis
    size_t yOrg;
};

/**
 * Interface for new cached spatial interpolation as used in #MetNoFimex::CDMInterpolator
 */
class CachedInterpolationInterface {
private:
    std::string _xDimName, _yDimName;
public:
    CachedInterpolationInterface(std::string xDimName, std::string yDimName) : _xDimName(xDimName), _yDimName(yDimName) {}
    virtual ~CachedInterpolationInterface() {}
    virtual boost::shared_array<float> interpolateValues(boost::shared_array<float> inData, size_t size, size_t& newSize) const = 0;
    /** @return x-size of input array */
    virtual size_t getInX() const = 0;
    /** @return y-size of input array */
    virtual size_t getInY() const = 0;
    /** @return x-size of output array */
    virtual size_t getOutX() const = 0;
    /** @return y-size of output array */
    virtual size_t getOutY() const = 0;
    /**
     * Read the input data from the reader, which is later used for the interpolateValues() function. This function will eventually reduce the
     * domain of the input data if createReducedDomain was called earlier.
     * @param reader
     * @param varName
     * @param unLimDim
     * @return Data matching input-data for this CachedInterpolationInterface
     */
    virtual DataPtr getInputDataSlice(boost::shared_ptr<CDMReader> reader, const std::string& varName, size_t unLimDim) const;
    /**
     * Read the input data from the reader, which is later used for the interpolateValues() function. This function will eventually reduce the
     * domain of the input data if createReducedDomain was called earlier.
     * @param reader
     * @param varName
     * @param sb a slicebuilder to reduce other than the horizontal dimensions
     * @return Data matching input-data for this CachedInterpolationInterface
     */
    virtual DataPtr getInputDataSlice(boost::shared_ptr<CDMReader> reader, const std::string& varName, const SliceBuilder& sb) const;
private:
    /**
     * allow fetching of a reduced interpolation domain, i.e. to work with a much smaller amount of input data
     * @return a 0-pointer unless a internal function to reduce the domain has been run, e.g. CachedInterpolation::createReducedDomain()
     */
    virtual boost::shared_ptr<ReducedInterpolationDomain> reducedDomain() const {return boost::shared_ptr<ReducedInterpolationDomain>();}
};

/**
 * Container to cache projection details to speed up
 * interpolation of lots of fields.
 */
class CachedInterpolation : public CachedInterpolationInterface
{
private:
    std::vector<double> pointsOnXAxis;
    std::vector<double> pointsOnYAxis;
    size_t inX;
    size_t inY;
    size_t outX;
    size_t outY;
    boost::shared_ptr<ReducedInterpolationDomain> reducedDomain_;
    int (*func)(const float* infield, float* outvalues, const double x, const double y, const int ix, const int iy, const int iz);
public:
    /**
     * @param funcType {@link interpolation.h} interpolation method
     * @param pointsOnXAxis projected values of the new projections coordinates expressed in the current x-coordinate (size = outX*outY)
     * @param pointsOnYAxis projected values of the new projections coordinates expressed in the current y-coordinate (size = outX*outY)
     * @param inX size of current X axis
     * @param inY size of current Y axis
     * @param outX size of new X axis
     * @param outY size of new Y axis
     */
    CachedInterpolation(std::string xDimName, std::string yDimName, int funcType, std::vector<double> pointsOnXAxis, std::vector<double> pointsOnYAxis, size_t inX, size_t inY, size_t outX, size_t outY);
    virtual ~CachedInterpolation() {}
    /**
     * Actually interpolate the data. The data will be interpolated as floats internally.
     *
     * @param inData the input data
     * @param size the size of the input data array
     * @param newSize return the size of the output-array
     */
    virtual boost::shared_array<float> interpolateValues(boost::shared_array<float> inData, size_t size, size_t& newSize) const;
    /**
     * @return x-size of the input data
     */
    virtual size_t getInX() const {return inX;}
    /**
     * @return y-size of the input data
     */
    virtual size_t getInY() const {return inY;}
    /**
     * @return x-size of the output data
     */
    virtual size_t getOutX() const {return outX;}
    /**
     * @return y-size of the output data
     */
    virtual size_t getOutY() const {return outY;}
    virtual boost::shared_ptr<ReducedInterpolationDomain> reducedDomain() const {return reducedDomain_;}
    /**
     * Create a reduced domain for later generation of a slicebuild to read a smaller domain.
     * It should be run immediately after creating the CachedInterpolation.
     */
    void createReducedDomain(std::string xDimName, std::string yDimName);

};



}

#endif /*CACHEDINTERPOLATION_H_*/
