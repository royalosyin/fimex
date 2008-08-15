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

#include "fimex/GribApiCDMWriter_ImplAbstract.h"
#include "fimex/TimeUnit.h"
#include "fimex/TimeLevelDataSliceFetcher.h"

namespace MetNoFimex
{

/** helper classes Scale and UnScale to transform double vectors */
class Scale : public std::unary_function<std::string, bool>
{
	const double scale_factor;
	const double add_offset;
public:
	Scale(double scale_factor, double add_offset) : scale_factor(scale_factor), add_offset(add_offset) {}
	virtual ~Scale() {}
	virtual double operator() (double x) const {return x*scale_factor + add_offset;}
};

class UnScale : public Scale
{
public:
	UnScale(double scale_factor, double add_offset) : Scale(1/scale_factor, -1 * add_offset/scale_factor) {}
	virtual ~UnScale() {}
};

/** TimeUnit 2 FimexTime converter*/
class TimeUnit2FimexTime : public std::unary_function<double, FimexTime>
{
	const TimeUnit& tu;
public:
	TimeUnit2FimexTime(const TimeUnit& tu) : tu(tu) {}
	FimexTime operator() (double unitTime) {return tu.unitTime2fimexTime(unitTime);}
};

GribApiCDMWriter_ImplAbstract::GribApiCDMWriter_ImplAbstract(int gribVersion, const boost::shared_ptr<CDMReader>& cdmReader, const std::string& outputFile, const std::string& configFile)
: gribVersion(gribVersion), cdmReader(cdmReader), outputFile(outputFile), configFile(configFile), xmlConfig(new XMLDoc(configFile)), gribFile(outputFile.c_str(), std::ios::binary|std::ios::out)
{
	logger = getLogger("fimex.GribApi_CDMWriter");
	std::string gribTemplate("GRIB" + type2string(gribVersion));
	gribHandle = boost::shared_ptr<grib_handle>(grib_handle_new_from_template(0, gribTemplate.c_str()), grib_handle_delete);
	if (gribHandle.get() == 0) throw CDMException("unable to open grib_handle_from_template for grib-template: " + gribTemplate);
	// check the file
	if (!gribFile.is_open()) throw CDMException("Cannot write grib-file: "+outputFile);
}

GribApiCDMWriter_ImplAbstract::~GribApiCDMWriter_ImplAbstract()
{
}

void GribApiCDMWriter_ImplAbstract::run() throw(CDMException)
{
	LOG4FIMEX(logger, Logger::DEBUG, "GribApiCDMWriter_ImplAbstract::run()  " );
	// TODO: set global attributes

	const CDM& cdm = cdmReader->getCDM();
	const CDM::VarVec& vars = cdm.getVariables();
	// iterator over all variables
	for (CDM::VarVec::const_iterator vi = vars.begin(); vi != vars.end(); ++vi) {
		const std::string& varName = vi->getName();
		try {
		std::vector<FimexTime> times = getTimes(varName);
		std::vector<double> levels = getLevels(varName);
		TimeLevelDataSliceFetcher tld(cdmReader, varName);
		try {
			setProjection(varName);
		} catch (CDMException& e) {
			LOG4FIMEX(logger, Logger::WARN, "cannot write variable " << varName << " due to projection problems: " << e.what());
			continue;
		}
		for (size_t t = 0; t < times.size(); t++) {
			for (size_t l = 0; l < levels.size(); l++) {
				boost::shared_ptr<Data> data = tld.getTimeLevelSlice(t, l);
				if (data->size() == 0) {
					// no data, silently skip to next level/time
					continue;
				}
				double levelVal = levels[l];
				const FimexTime& fTime = times[t];
				setMissingValue(varName, fTime, levelVal);
				setData(data);
				setLevel(varName, levelVal);
				setTime(varName, fTime);
				setParameter(varName, fTime, levelVal);
				writeGribHandleToFile();
			}
		}
		} catch (CDMException& e) {
			LOG4FIMEX(logger, Logger::WARN, "unable to write parameter "<< varName << ": " << e.what());
		}
	}
}

void GribApiCDMWriter_ImplAbstract::setData(const boost::shared_ptr<Data>& data) {
	GRIB_CHECK(grib_set_double_array(gribHandle.get(), "values", data->asConstDouble().get(), data->size()), "setting values");
}

void GribApiCDMWriter_ImplAbstract::setTime(const std::string& varName, const FimexTime& fTime)
{
	LOG4FIMEX(logger, Logger::DEBUG, "setTime(" << varName << ", " << fTime << ")" );
	long date = fTime.year * 10000 + fTime.month * 100 + fTime.mday;
	long time = fTime.hour * 100 + fTime.minute;
	GRIB_CHECK(grib_set_long(gribHandle.get(), "dataDate", date), "setting dataDate");
	GRIB_CHECK(grib_set_long(gribHandle.get(), "dataTime", time), "setting dataTime");
}

std::vector<double> GribApiCDMWriter_ImplAbstract::getLevels(const std::string& varName) throw(CDMException)
{
	LOG4FIMEX(logger, Logger::DEBUG, "getLevels(" << varName << ")" );
	Units units;
	std::vector<double> levelData;
	// TODO: proper definition of level (code table 3) (indicatorOfLevel)
	// recalculate level values to have units as defined in code table 3
	// recalculate units of level
	const CDM& cdm = cdmReader->getCDM();
	std::string verticalAxis = cdm.getVerticalAxis(varName);
	std::string verticalAxisXPath("/cdm_gribwriter_config/axes/vertical_axis");
	std::string unit;
	if (verticalAxis != ""){
		boost::shared_ptr<Data> myLevelData = cdmReader->getData(verticalAxis);
		const boost::shared_array<double> levelDataArray = myLevelData->asConstDouble();
		levelData= std::vector<double>(&levelDataArray[0], &levelDataArray[myLevelData->size()]);
		CDMAttribute attr;
		if (cdm.getAttribute(verticalAxis, "standard_name", attr)) {
			verticalAxisXPath += "[@standard_name=\""+ attr.getData()->asString() + "\"]";
		} else if (cdmReader->getCDM().getAttribute(verticalAxis, "units", attr)) {
			// units compatible to Pa or m
			std::string unit = attr.getData()->asString();
			if (units.areConvertible(unit, "m")) {
				verticalAxisXPath += "[@unitCompatibleTo=\"m\"]";
			} else if (units.areConvertible(unit, "Pa")) {
				verticalAxisXPath += "[@unitCompatibleTo=\"Pa\"]";
			} else {
				throw CDMException("units of vertical axis " + verticalAxis + " should be compatible with m or Pa but are: " + unit);
			}
		} else {
			throw CDMException("couldn't find standard_name or units for vertical Axis " + verticalAxis + ". Is this CF compatible?");
		}
	} else {
		// cdmGribWriterConfig should contain something like standard_name=""
		verticalAxisXPath += "[@standard_name=\"\"]";
		// TODO get default from config
		levelData.push_back(0);
	}
	// scale the original levels according to the cdm
	double scale_factor = 1.;
	double add_offset = 0.;
	CDMAttribute attr;
	if (cdm.getAttribute(verticalAxis, "scale_factor", attr)) {
		scale_factor = attr.getData()->asDouble()[0];
	}
	if (cdm.getAttribute(verticalAxis, "add_offset", attr)) {
		add_offset = attr.getData()->asDouble()[0];
	}
	std::transform(levelData.begin(), levelData.end(), levelData.begin(), Scale(scale_factor, add_offset));


	// scale the levels according to grib
	verticalAxisXPath += "/grib" + type2string(gribVersion);
	XPathObjPtr verticalXPObj = xmlConfig->getXPathObject(verticalAxisXPath);
	xmlNodeSetPtr nodes = verticalXPObj->nodesetval;
	int size = (nodes) ? nodes->nodeNr : 0;
	if (size == 1) {
		xmlNodePtr node = nodes->nodeTab[0];
		// scale the levels from cf-units to grib-untis
		std::string gribUnits = getXmlProp(node, "units");
		if (gribUnits != "") {
			if (cdm.getAttribute(verticalAxis, "units", attr)) {
				double slope;
				double offset;
				units.convert(attr.getData()->asString(), gribUnits, slope, offset);
				std::transform(levelData.begin(), levelData.end(), levelData.begin(), Scale(slope, offset));
			}
		}

		// unscale to be able to put the data into values suitable to grib
		std::string gribScaleFactorStr = getXmlProp(node, "scale_factor");
		std::string gribAddOffsetStr = getXmlProp(node, "add_offset");
		scale_factor = 1.;
		add_offset = 0.;
		if (gribScaleFactorStr != "") {
			scale_factor = string2type<double>(gribScaleFactorStr);
		}
		if (gribAddOffsetStr != "") {
			add_offset = string2type<double>(gribAddOffsetStr);
		}
		std::transform(levelData.begin(), levelData.end(), levelData.begin(), UnScale(scale_factor, add_offset));
	} else if (size > 1) {
		throw CDMException("several entries in grib-config at " + configFile + ": " + verticalAxisXPath);
	} else {
		std::cerr << "could not find vertical Axis " << verticalAxisXPath << " in " << configFile << ", skipping parameter " << varName << std::endl;
	}
	return levelData;
}

std::vector<FimexTime> GribApiCDMWriter_ImplAbstract::getTimes(const std::string& varName) throw(CDMException)
{
	LOG4FIMEX(logger, Logger::DEBUG, "getTimes(" << varName << ")" );
	const CDM& cdm = cdmReader->getCDM();
	const std::string& time = cdm.getTimeAxis(varName);
	std::vector<FimexTime> timeData;
	if (time != "") {
		const boost::shared_array<double> timeDataArray = cdmReader->getData(time)->asDouble();
		std::vector<double> timeDataVector(&timeDataArray[0], &timeDataArray[cdm.getDimension(time).getLength()]);
		TimeUnit tu(cdm.getAttribute(time, "units").getStringValue());
		std::transform(timeDataVector.begin(), timeDataVector.end(), std::back_inserter(timeData), TimeUnit2FimexTime(tu));
	} else {
		// TODO find a more useful default
		timeData.push_back(FimexTime());
	}
	return timeData;
}

void GribApiCDMWriter_ImplAbstract::writeGribHandleToFile()
{
	LOG4FIMEX(logger, Logger::DEBUG, "writeGribHandleToFile");
	// write data to file
    size_t size;
    const void* buffer;
    /* get the coded message in a buffer */
    GRIB_CHECK(grib_get_message(gribHandle.get(),&buffer,&size),0);
    gribFile.write(reinterpret_cast<const char*>(buffer), size);
}


}