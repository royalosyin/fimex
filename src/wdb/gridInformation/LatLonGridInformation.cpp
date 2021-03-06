/*
 fimex

 Copyright (C) 2011 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: post@met.no

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

#include "LatLonGridInformation.h"
#include <fimex/CDM.h>
#include <fimex/CDMDimension.h>
#include <fimex/CDMVariable.h>
#include <fimex/CDMAttribute.h>
#include <fimex/Data.h>


namespace MetNoFimex
{
namespace wdb
{


LatLonGridInformation::LatLonGridInformation(PGresult * result, int row) :
	GridInformation(result, row)
{}

LatLonGridInformation::LatLonGridInformation(const boost::shared_ptr<Projection> & projection, unsigned numberX, unsigned numberY) :
	GridInformation(projection, numberX, numberY)
{}

LatLonGridInformation::~LatLonGridInformation()
{
}

void LatLonGridInformation::addToCdm(CDM & cdm) const
{
	cdm.addDimension(CDMDimension("longitude", numberX()));
	cdm.addDimension(CDMDimension("latitude", numberY()));

	cdm.addVariable(CDMVariable("longitude", CDM_FLOAT, std::vector<std::string>(1, "longitude")));
	cdm.addAttribute("longitude", CDMAttribute("long_name", "longitude"));
	cdm.addAttribute("longitude", CDMAttribute("standard_name", "longitude"));
	cdm.addAttribute("longitude", CDMAttribute("units", "degree_east"));
	cdm.addAttribute("longitude", CDMAttribute("axis", "X"));

	cdm.addVariable(CDMVariable("latitude", CDM_FLOAT, std::vector<std::string>(1, "latitude")));
	cdm.addAttribute("latitude", CDMAttribute("long_name", "latitude"));
	cdm.addAttribute("latitude", CDMAttribute("standard_name", "latitude"));
	cdm.addAttribute("latitude", CDMAttribute("units", "degree_north"));
	cdm.addAttribute("latitude", CDMAttribute("axis", "Y"));
}

DataPtr LatLonGridInformation::getField(const CDMVariable & variable) const
{
	DataPtr ret;
	if ( variable.getName() == "longitude" )
	{
		ret = createData(variable.getDataType(), numberX());
		for ( unsigned i = 0; i < numberX(); ++ i )
			ret->setValue(i, startX() + (incrementX() * i));
	}
	else if ( variable.getName() == "latitude" )
	{
		ret = createData(variable.getDataType(), numberY());
		for ( unsigned i = 0; i < numberY(); ++ i )
			ret->setValue(i, startY() + (incrementY() * i));
	}
	else
		ret = GridInformation::getField(variable);
	return ret;
}

bool LatLonGridInformation::canHandle(const std::string & name) const
{
	return name == "longitude" or name == "latitude" or GridInformation::canHandle(name);
}

void LatLonGridInformation::addSpatialDimensions(std::vector<std::string> & out) const
{
	out.push_back("longitude");
	out.push_back("latitude");
}

}
}
