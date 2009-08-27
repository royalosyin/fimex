/*
 wdb

 Copyright (C) 2007 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: wdb@met.no

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA  02110-1301, USA
 */

#include "felt/FeltGridDefinition.h"
#include "felt/FeltFile.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cmath>
#include "felt/FeltConstants.h"
#include "proj_api.h"

namespace felt {

/**
 * converts a point on earth to a projection plane
 * @param projStr projection definition for proj4
 * @param lon longitude in degree
 * @param lat latitutde in degree
 * @param x output of proj (usually m, but dependent on projStr)
 * @param y output of proj
 * @throws runtime_error on proj-failure
 */
static void projConvert(const std::string& projStr, double lon, double lat, double& x, double& y)
{
    projPJ outputPJ;
    if ( !(outputPJ = pj_init_plus(projStr.c_str())) ) {
        std::string errorMsg(pj_strerrno(pj_errno));
        throw std::runtime_error("Proj error: " + errorMsg);
    }

    projUV uv;
    uv.u = lon * DEG_TO_RAD;
    uv.v = lat * DEG_TO_RAD;

    uv = pj_fwd(uv, outputPJ);
    pj_free(outputPJ);

    if (uv.u == HUGE_VAL) {
        std::ostringstream errMsg;
        errMsg <<  "projection fails:" << projStr << " (lon,lat)=(" << lon << "," << lat << ")";
        throw std::runtime_error(errMsg.str());
    }
    x = uv.u;
    y = uv.v;
}


// first scaling method
void scaleGridInfoFirst_(boost::array<float, 6>& gridPar, int parsUsed, float scale, const std::vector<short int> & extraData)
{
    assert(parsUsed <= 6);
    for (int i = 0; i < parsUsed; ++i) {
        int i2 = 2*i;
        int iGrid1 = extraData.at(i2);
        int iGrid2 = extraData.at(i2 + 1);

        gridPar[i] = (iGrid1 * 10000. + iGrid2) / scale;
    }
}

// second scaling method, internal scaling factor
void scaleGridInfoSecond_(boost::array<float, 6>& gridPar, int parsUsed, const std::vector<short int> & extraData)
{

    // two shorts for consistency checks
    if(extraData[0] != parsUsed)
        throw std::runtime_error("First word in the grid encoding does not correspond to the gridspec size");
    if(extraData[1] != 3)
        throw std::runtime_error("Second word in the grid encoding does not correspond to 3 (rotated grid)");

    for (int i = 0; i < parsUsed; ++i) {
        int i3 = i*3 + 2; // 2 offset
        float scale = std::pow(10., extraData.at(i3));
        int iGrid1 = extraData.at(i3 + 1);
        int iGrid2 = extraData.at(i3 + 2);

        gridPar[i] = (iGrid1 * 10000. + iGrid2) / scale;
    }
}

/**
 *
 * @param gridPar the 6 gridParameters, these values will be changed!
 * @param parsUsed the number of relevant gridParameters for the projection, must be <= 6
 * @param scale scaling factor for the extraData used for the projection in scaleMethod 1
 * @param extraData the extra data at the end of the grid in a felt-grid on disk
 * @throw runtime_error if extraData.size doesn't match any scaling method
 */
void scaleExtraData_(boost::array<float, 6>& gridPar, int parsUsed, float scale, const std::vector<short int> & extraData)
{
    assert(parsUsed <= 6);
    assert(parsUsed > 0);
    if (extraData.size() == 0) {
        // do nothing
    } else if (extraData.size() == (2*static_cast<unsigned int>(parsUsed))) {
        scaleGridInfoFirst_(gridPar, parsUsed, scale, extraData);
    } else if (extraData.size() == (2+3*static_cast<unsigned int>(parsUsed))) {
        scaleGridInfoSecond_(gridPar, parsUsed, extraData);
    } else {
        std::ostringstream oss;
        oss << "inconsistent extra data in felt: extraData.size=" << extraData.size() << " required_parameters=" << parsUsed;
        throw std::runtime_error(oss.str());
    }
}

boost::array<float, 6> gridParametersPolarStereo_(int ixp, int iyp, int idist, int iphi, const std::vector<short int> & extraData)
{
    const int parsUsed = 5; // gridPar 6 not used
    boost::array<float, 6> gridPar;
    for (int i = 0; i < 6; ++i) {
        gridPar[i] = 0.;
    }
    // default at 60 degree
    gridPar[4] = 60.;
    if (idist > 0) {
        // standard
        gridPar[0] = ixp * .01;
        gridPar[1] = iyp * .01;
        gridPar[2] = idist * .1;
        gridPar[3] = iphi;
    } else {
        // old extension
        gridPar[0] = ixp;
        gridPar[1] = iyp;
        gridPar[2] = idist * -.1;
        gridPar[3] = iphi;
    }

    // corrections from extraData
    scaleExtraData_(gridPar, parsUsed, 100., extraData);

    // fix old met.no-scale (79.*150.cells between northpole and equator), check consistency
    if (gridPar[2] != 0) gridPar[2] = 79.*150./gridPar[2];
    else throw std::runtime_error("polar-stereographic: cells between equator and pole cannot be 0");

    if (gridPar[4] == 0 || gridPar[4] < -90. || gridPar[4] > 90.)
        throw std::runtime_error("polar-stereographic: undefined angle phi");

    return gridPar;
}

boost::array<float, 6> gridParametersGeographic_(int ilat, int ilon, int latDist, int lonDist, const std::vector<short int> & extraData)
{
    // longlat and rotated longlat
    const int parsUsed = 6;
    boost::array<float, 6> gridPar;
    for (int i = 0; i < 6; ++i) {
        gridPar[i] = 0.;
    }
    // standard
    gridPar[0] = ilon * .01;
    gridPar[1] = ilat * .01;
    gridPar[2] = lonDist * .01;
    gridPar[3] = latDist * .01;

    // corrections from extraData
    scaleExtraData_(gridPar, parsUsed, 10000., extraData);

    // check consistency
    if (gridPar[2] == 0 || gridPar[3] == 0)
        throw std::runtime_error("(rotated) geographic: gridDistance > 0 required");

    return gridPar;
}

boost::array<float, 6> gridParametersMercator_(int iWestBound, int iSouthBound, int iXincr, int iYincr, const std::vector<short int> & extraData)
{
    const int parsUsed = 5; // par 6 is empty
    boost::array<float, 6> gridPar;
    for (int i = 0; i < 6; ++i) {
        gridPar[i] = 0.;
    }
    // standard
    gridPar[0] = iWestBound * .01;
    gridPar[1] = iSouthBound * .01;
    gridPar[2] = iXincr * .1;
    gridPar[3] = iYincr * .1;

    // corrections from extraData
    scaleExtraData_(gridPar, parsUsed, 10000., extraData);

    // check consistency
    if (gridPar[2] == 0 || gridPar[3] == 0)
        throw std::runtime_error("mercator projection: gridDistance > 0 required");

    return gridPar;
}

boost::array<float, 6> gridParametersLambertConic_(int iWestBound, int iSouthBound, int iXincr, int iYincr, const std::vector<short int> & extraData)
{
    const int parsUsed = 6;
    boost::array<float, 6> gridPar;
    for (int i = 0; i < 6; ++i) {
        gridPar[i] = 0.;
    }
    // standard
    gridPar[0] = iWestBound * .01;
    gridPar[1] = iSouthBound * .01;
    gridPar[2] = iXincr * .1;
    gridPar[3] = iYincr * .1;

    // corrections from extraData
    scaleExtraData_(gridPar, parsUsed, 100., extraData);

    // check consistency
    if (gridPar[2] == 0 || gridPar[3] == 0)
        throw std::runtime_error("lambert conic projection: gridDistance > 0 required");
    if (gridPar[5] == 90 || gridPar[5] == -90)
        throw std::runtime_error("lambert conic projection: tangent = +-90degree, use polarstereographic!");
    if (gridPar[5] == 0)
        throw std::runtime_error("lambert conic projection: tangent = 0degree, use mercator!");

    return gridPar;
}

boost::array<float, 6> gridParameters(int gridType, int a, int b, int c, int d, const std::vector<short int> & extraData)
{
    switch (gridType) {
        case 1:
        case 4: return gridParametersPolarStereo_(a, b, c, d, extraData); break;
        case 2:
        case 3: return gridParametersGeographic_(a, b, c, d, extraData); break;
        case 5: return gridParametersMercator_(a, b, c, d, extraData); break;
        case 6: return gridParametersLambertConic_(a, b, c, d, extraData); break;
    }
    throw std::invalid_argument("Unknown grid specification");
}


std::string gridParametersToProjDefinition(int gridType, const boost::array<float, 6>& gs)
{
    std::ostringstream projStr;
    switch (gridType) {
    case 1:
    case 4:
    	projStr << "+proj=stere +lat_0=90 +lon_0=" << (gs[3]) << " +lat_ts=" << (gs[4]) << " +units=m";
    	break;
    case 2:
        projStr << "+proj=longlat";
        break;
    case 3:
        projStr << "+proj=ob_tran +o_proj=longlat +lon_0=" << (gs[4]) << " +o_lat_p=" << (90 - gs[5]);
        break;
    case 5:
        // lat_0!=0 not supported for merc, lon_0 and lat_0 part of startx, starty
        // TODO: untested, missing test case
        projStr << "+proj=merc +lat_ts=" << (gs[4]);
    case 6:
        // libmi supports only lat_0=0 = non-oblique (pole) lcc
        // TODO: untested, missing test case
        projStr << "+proj=lcc +lon_0="<< (gs[4]) << " +lat_1="<< (gs[5]) << " +lat_2=" << (gs[5]);
        break;
    default: throw std::invalid_argument("Unknown grid specification");

    }

    // libmi used constant earth radius for all projections (earthr.f); no_defs = no (US) defaults
    projStr << " +a=" << EARTH_RADIUS << " +e=0 +no_defs";

    return projStr.str();
}

FeltGridDefinition::Orientation FeltGridDefinition::getScanMode_()
{
    boost::array<float, 6> & gs = gridPars_;
	float & jIncrement = gs[3];
	float & startLatitude = gs[1];
	if(jIncrement < 0)
    {
		jIncrement = jIncrement * -1;
		startLatitude = startLatitude - (yNum_ * jIncrement) + jIncrement;
        return LeftUpperHorizontal;
    }
    return LeftLowerHorizontal;
}

FeltGridDefinition::FeltGridDefinition( int gridType,
										int xNum, int yNum, int a, int b, int c, int d,
										const std::vector<short int> & extraData) :
										    gridType_(gridType), xNum_(xNum), yNum_(yNum)
{
    switch (gridType){
    case 0:
        throw std::invalid_argument("Unspecified grid is not supported");
    case 1:
    case 4:
        FeltFile::log("FeltGridDefinition: stereograhpic grids");
   		polarStereographicProj_( gridType, a, b, c, d, extraData );
    	break;
    case 2:
    case 3:
      	geographicProj_( gridType, a, b, c, d, extraData );
        break;
    case 5:
        mercatorProj_( gridType, a, b, c, d, extraData );
    case 6:
        lambertConicProj_( gridType, a, b, c, d, extraData );
    default:
        throw std::invalid_argument("Unknown grid specification");
    }
}

FeltGridDefinition::~FeltGridDefinition()
{
}

void
FeltGridDefinition::polarStereographicProj_( int gridType,
                                             int a, int b,
                                             int c, int d,
                                             const std::vector<short int> & extraData)
{
    gridPars_ = gridParameters(gridType, a, b, c, d, extraData);
    const boost::array<float, 6>& gs = gridPars_;

    if (FeltFile::isLogging()) {
        std::ostringstream buffer;
        buffer << "FeltGridDefinition: " << std::endl;
        buffer << "Size of Grid: " << xNum_ << " x " << yNum_ << std::endl;
        buffer << "Grid Specification: " << gs[0] << " | " << gs[1] << " | " << gs[2] << " | " << gs[3] << " | " << gs[4] << " | " << gs[5] << std::endl;
        buffer << "Grid Type: " << gridType << std::endl;
        FeltFile::log(buffer.str());
    }
    if (FeltFile::isLogging())
        FeltFile::log("FeltGridDefinition: Proj Specification: " + gridParametersToProjDefinition(gridType, gs));

    orientation_ = LeftLowerHorizontal; // Default
    float incr = EARTH_RADIUS * (1+std::sin(PI/180.*gs[4])) / gs[2];

    startX_ = ( 1 - gs[0] ) * incr;
    startY_ = ( 1 - gs[1] ) * incr;
    incrementX_ = incr;
    incrementY_ = incr;
}

void
FeltGridDefinition::geographicProj_( int gridType,
                                     int a, int b,
                                     int c, int d,
                                     const std::vector<short int> & extraData)
{
    gridPars_ = gridParameters(gridType, a, b, c, d, extraData);
    const boost::array<float, 6>& gs = gridPars_;

    if (FeltFile::isLogging()) {
        std::ostringstream buffer;
        buffer << "FeltGridDefinition: " << std::endl;
        buffer << "Grid Specification: " << gs[0] << " | " << gs[1] << " | " << gs[2] << " | " << gs[3] << " | " << gs[4] << " | " << gs[5] << std::endl;
        buffer << "Grid Type: " << gridType << std::endl;
        FeltFile::log(buffer.str());
    }
    if (FeltFile::isLogging())
        FeltFile::log("FeltGridDefinition: Proj Specification: " + gridParametersToProjDefinition(gridType, gs));

    orientation_ = getScanMode_();
    incrementX_ = gs[2];
    incrementY_ = gs[3];
    startX_ = gs[0];
    startY_= gs[1];
}

void
FeltGridDefinition::mercatorProj_( int gridType,
                                   int a, int b,
                                   int c, int d,
                                   const std::vector<short int> & extraData)
{
    gridPars_ = gridParameters(gridType, a, b, c, d, extraData);
    const boost::array<float, 6>& gs = gridPars_;

    if (FeltFile::isLogging()) {
        std::ostringstream buffer;
        buffer << "FeltGridDefinition: " << std::endl;
        buffer << "Grid Specification: " << gs[0] << " | " << gs[1] << " | " << gs[2] << " | " << gs[3] << " | " << gs[4] << " | " << gs[5] << std::endl;
        buffer << "Grid Type: " << gridType << std::endl;
        FeltFile::log(buffer.str());
    }
    if (FeltFile::isLogging())
        FeltFile::log("FeltGridDefinition: Proj Specification: " + gridParametersToProjDefinition(gridType, gs));

    orientation_ = LeftLowerHorizontal; // Default
    incrementX_ = gs[2];
    incrementY_ = gs[3];

    // gs[0] and gs[1] are startX and startY given in degree, not in projection-plane, convert using proj
    double startX, startY;
    projConvert(gridParametersToProjDefinition(gridType, gs), gs[0], gs[1], startX, startY);
    startX_ = static_cast<float>(startX);
    startY_ = static_cast<float>(startY);
}

void
FeltGridDefinition::lambertConicProj_( int gridType,
                                       int a, int b,
                                       int c, int d,
                                       const std::vector<short int> & extraData)
{
    gridPars_ = gridParameters(gridType, a, b, c, d, extraData);
    const boost::array<float, 6>& gs = gridPars_;

    if (FeltFile::isLogging()) {
        std::ostringstream buffer;
        buffer << "FeltGridDefinition: " << std::endl;
        buffer << "Grid Specification: " << gs[0] << " | " << gs[1] << " | " << gs[2] << " | " << gs[3] << " | " << gs[4] << " | " << gs[5] << std::endl;
        buffer << "Grid Type: " << gridType << std::endl;
        FeltFile::log(buffer.str());
    }
    if (FeltFile::isLogging())
        FeltFile::log("FeltGridDefinition: Proj Specification: " + gridParametersToProjDefinition(gridType, gs));

    orientation_ = LeftLowerHorizontal; // Default
    incrementX_ = gs[2];
    incrementY_ = gs[3];

    // gs[0] and gs[1] are startX and startY given in degree, not in projection-plane, convert using proj
    double startX, startY;
    projConvert(gridParametersToProjDefinition(gridType, gs), gs[0], gs[1], startX, startY);
    startX_ = static_cast<float>(startX);
    startY_ = static_cast<float>(startY);
}


void
FeltGridDefinition::polarStereographicProj( int gridType,
									        float poleX, float poleY,
									        float gridD, float rot,
									        const std::vector<short int> & extraData)
{
    FeltFile::log("FeltGridDefinition: Polar stereographic projection identified in FELT file");
    boost::array<float, 6> & gs = gridPars_;
    if ( extraData.empty() )
    {
        double pi = 3.14159265358979323844;
        gs[0] = poleX;
        gs[1] = poleY;
        gs[3] = rot;
        gs[4] = 60.0;
        // grid-distance due to scale-factor and old 150km grid correction
        // make sure to set gs[4], in particular with gridType 5
        gs[2] = gridD * (6371.* (1+std::sin(pi/180.*gs[4]))) / (79.*150.);
        gs[5] = 0.0;
    }
	else
		throw std::runtime_error("The encoded polar stereographic grid specification in the FELT file is not supported");

    if (FeltFile::isLogging()) {
        std::ostringstream buffer;
        buffer << "FeltGridDefinition: " << std::endl;
        buffer << "Size of Grid: " << xNum_ << " x " << yNum_ << std::endl;
        buffer << "Grid Specification: " << gs[0] << " | " << gs[1] << " | " << gs[2] << " | " << gs[3] << " | " << gs[4] << " | " << gs[5] << std::endl;
        buffer << "Grid Type: " << gridType << std::endl;
        FeltFile::log(buffer.str());
    }
    if (FeltFile::isLogging())
        FeltFile::log("FeltGridDefinition: Proj Specification: " + gridParametersToProjDefinition(gridType, gs));

    orientation_ = LeftLowerHorizontal; // Default
    startX_ = ( 1 - gs[0] ) * gs[2];
    startY_ = ( 1 - gs[1] ) * gs[2];
    incrementX_ = gs[2];
    incrementY_ = gs[2];
}

void
FeltGridDefinition::geographicProj( int gridType,
									float startLongitude, float startLatitude,
									float iInc, float jInc,
									const std::vector<short int> & extraData)
{
    FeltFile::log("FeltGridDefinition: Geographic projection identified in FELT file");
    const int gsSize = 6;
    float scale = 10000.0;
    boost::array<float, 6> & gs = gridPars_;

    if ( extraData.empty() )
    {
        gs[0] = startLongitude;
        gs[1] = startLatitude;
        gs[2] = iInc;
        gs[3] = jInc;
        gs[4] = 0.0;
        gs[5] = 0.0;
    }
    else if ( extraData.size() == gsSize * 2 )
    {
		for(int i = 0;i < gsSize;i++)
			gs[i] = static_cast<float>((extraData[i * 2] * scale) + extraData[(i * 2) + 1]) / scale;
	}
	else if ( extraData.size() == 2 + (gsSize * 3) )
	{
		if(extraData[0] != gsSize)
			throw std::runtime_error("First word in the grid encoding does not correspond to the gridspec size");

		if(extraData[1] != 3)
			throw std::runtime_error("Second word in the grid encoding does not correspond to 3 (rotated grid)");

		for(int i = 0;i < gsSize;i++)
			gs[i] = static_cast<float>((extraData[(i * 3) + 3] * scale) + extraData[(i * 3) + 4]) / (extraData[(i * 3) + 2] * 10.0);
	}
	else
		throw std::runtime_error("The encoded grid specification in the FELT file is not supported");


    if (FeltFile::isLogging()) {
        std::ostringstream buffer;
        buffer << "FeltGridDefinition: " << std::endl;
        buffer << "Grid Specification: " << gs[0] << " | " << gs[1] << " | " << gs[2] << " | " << gs[3] << " | " << gs[4] << " | " << gs[5] << std::endl;
        buffer << "Grid Type: " << gridType << std::endl;
        FeltFile::log(buffer.str());
    }
    if (FeltFile::isLogging())
        FeltFile::log("FeltGridDefinition: Proj Specification: " + gridParametersToProjDefinition(gridType, gs));

    /* no longer supported
     * orientation_ = getScanMode_(gs, yNum_);
     */
    orientation_ = getScanMode();
    incrementX_ = gs[2];
    incrementY_ = gs[3];
    startX_ = gs[0];
    startY_= gs[1];
}

std::string FeltGridDefinition::projDefinition() const
{
	return gridParametersToProjDefinition(gridType_, getGridParameters());
};

int
FeltGridDefinition::getXNumber() const
{
    return xNum_;
};

int
FeltGridDefinition::getYNumber() const
{
    return yNum_;
};

float
FeltGridDefinition::getXIncrement() const
{
	return incrementX_;
};

float
FeltGridDefinition::getYIncrement() const
{
	return incrementY_;
};

float
FeltGridDefinition::startLatitude() const
{
	return startY_;
};

float
FeltGridDefinition::startLongitude() const
{
	return startX_;
};

float
FeltGridDefinition::startX() const
{
	return startX_;
};

float
FeltGridDefinition::startY() const
{
	return startY_;
};

FeltGridDefinition::Orientation FeltGridDefinition::getScanMode() const
{
	return orientation_;
};

const boost::array<float, 6>&
FeltGridDefinition::getGridParameters() const
{
    return gridPars_;
}


std::ostream & contentSummary(std::ostream & out, const FeltGridDefinition & grid)
{
	return out << "FeltGridDefinition( " << grid.getXIncrement() << ", "<<grid.getYIncrement() << ", "
		<< grid.startLongitude() << ", " << grid.startLatitude() << " )" << std::endl;
}

}
