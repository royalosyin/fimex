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

#ifndef CDMREADER_H_
#define CDMREADER_H_

#include <boost/shared_ptr.hpp>
#include "fimex/CDM.h"
#include "fimex/Data.h"
#include "fimex/CDMVariable.h"

namespace MetNoFimex
{

/**
 * @brief Basic interface for CDM reading and manipulation classes
 * 
 * The CDMReader is the basic interface for reading and manipulation of 
 * the cdm datastructure. The {@link CDMWriter} will work with an implementation
 * of the CDMReader and read the included data in the cdm or the data provided
 * through the implementation of the {@link CDMReader#getDataSlice}
 * 
 * @see FeltCDMReader
 */
class CDMReader
{
public:
	CDMReader() {};
	virtual ~CDMReader() {}

	virtual const CDM& getCDM() const {return cdm;}
	/**
	 * @brief data-reading function to be called from the CDMWriter
	 * 
	 * This function needs to be implemented by the CDMReader. It should provide the data
	 * for each variable, either by reading from disk, converting from another CDMReader or
	 * reading from an in-memory data-section.
	 * 
	 * @param varName name of the variable to read
	 * @param unLimDimPos (optional) if the variable contains a unlimited dimension (max one allowed) an slice of this position is returned
	 */
	virtual const boost::shared_ptr<Data> getDataSlice(const std::string& varName, size_t unLimDimPos = 0) throw(CDMException) = 0;
	/**
	 * Read the data from the variable.hasData() and select the correct unLimDimPos.
	 * This function should be used internally from getDataSlice.
	 * @param variable the variable to read data from
	 * @param unLimDimPos (optional) the unlimited position
	 */
	virtual const boost::shared_ptr<Data> getDataFromMemory(const CDMVariable& variable, size_t unLimDimPos = 0) throw(CDMException);
	
protected:
	CDM cdm;
};

}

#endif /*CDMREADER_H_*/
