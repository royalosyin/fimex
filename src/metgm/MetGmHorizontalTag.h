/*
 * Fimex
 *
 * (C) Copyright 2011, met.no
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

/**
  * Used as private/implementation class
  */

#ifndef METGM_HORIZONTALTAG_H
#define METGM_HORIZONTALTAG_H

// fimex
//
#include "fimex/CDM.h"
#include "fimex/Data.h"
#include "fimex/TimeUnit.h"
#include "fimex/CDMReader.h"
#include "fimex/CDMVariable.h"
#include "fimex/CDMDimension.h"
#include "fimex/CDMAttribute.h"
#include "fimex/CDMException.h"
#include "fimex/CDMReaderUtils.h"
#include "fimex/coordSys/CoordinateSystem.h"

// boost
//
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// standard
//
#include <ctime>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

namespace MetNoFimex {

    class MetGmXTag;
    class MetGmYTag;
    class MetGmGroup3Ptr;

    class MetGmHorizontalTag {
    public:

        static boost::shared_ptr<MetGmXTag> createMetGmXTagForWriting(const boost::shared_ptr<CDMReader> pCdmReader, const CDMVariable* pVariable);
        static boost::shared_ptr<MetGmYTag> createMetGmYTagForWriting(const boost::shared_ptr<CDMReader> pCdmReader, const CDMVariable* pVariable);

        static boost::shared_ptr<MetGmXTag> createMetGmXTagForReading(const boost::shared_ptr<MetGmGroup3Ptr> pg3);
        static boost::shared_ptr<MetGmYTag> createMetGmYTagForReading(const boost::shared_ptr<MetGmGroup3Ptr> pg3);

        inline unsigned int         numberOfPoints()     { return numberOfPoints_; }
        inline double               distance()           { return distance_; }
        inline double               center()             { return center_; }
        inline std::vector<double>& horizontalPoints()   { return horizontalPoints_; }

    protected:

        inline MetGmHorizontalTag()
            : numberOfPoints_(0), center_(0), distance_(0) { }

        inline void extractHorizontalPoints(const DataPtr& data)
        {
            if(data->size() <= 1)
                throw CDMException("horizontal axis has one point can't determine distance needed for MetGm");

            const boost::shared_array<double> hArray = data->asDouble();

            horizontalPoints_ = std::vector<double>(&hArray[0], &hArray[data->size()]);
        }

        unsigned int        numberOfPoints_;
        double              center_;
        double              distance_;
        std::vector<double> horizontalPoints_;
    };

    class MetGmXTag : private MetGmHorizontalTag {
    public:
        friend class MetGmHorizontalTag;

        inline unsigned int         nx()        { return numberOfPoints(); }
        inline double               dx()        { return distance(); }
        inline double               cx()        { return center(); }
        inline std::vector<double>& xPoints()   { return horizontalPoints(); }
    private:
        inline MetGmXTag() : MetGmHorizontalTag() { }

    };

    class MetGmYTag : private MetGmHorizontalTag {
    public:
        friend class MetGmHorizontalTag;

        inline unsigned int         ny()        { return numberOfPoints(); }
        inline double               dy()        { return distance(); }
        inline double               cy()        { return center(); }
        inline std::vector<double>& yPoints()   { return horizontalPoints(); }
    private:
        inline MetGmYTag() : MetGmHorizontalTag() { }

    };
}

#endif // METGM_HORIZONTALTAG_H
