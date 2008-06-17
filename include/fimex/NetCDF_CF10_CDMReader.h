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

#ifndef NETCDF_CF10_CDMREADER_H_
#define NETCDF_CF10_CDMREADER_H_

#include "fimex/config.h"
#include "fimex/CDMReader.h"
#include NETCDF_CPP_INCLUDE

namespace MetNoFimex
{

class NetCDF_CF10_CDMReader : public MetNoFimex::CDMReader
{
	std::string filename;
	NcFile ncFile;
public:
	NetCDF_CF10_CDMReader(const std::string& fileName);
	virtual ~NetCDF_CF10_CDMReader();
	virtual const boost::shared_ptr<Data> getDataSlice(const std::string& varName, size_t unLimDimPos = 0) throw(CDMException);
private:
	void addAttribute(const std::string& varName, NcAtt* ncAtt);
};

}

#endif /*NETCDF_CF10_CDMREADER_H_*/
