/*
 * Fimex, NcmlCDMReader.cc
 *
 * (C) Copyright 2009, met.no
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
 *
 *  Created on: Apr 29, 2009
 *      Author: Heiko Klein
 */

#include "fimex/NcmlCDMReader.h"
#include "fimex/XMLDoc.h"
#include "fimex/NetCDF_CF10_CDMReader.h"
#include "fimex/Logger.h"

namespace MetNoFimex
{

using namespace std;

static LoggerPtr logger = getLogger("fimex/NcmlCDMReader");

NcmlCDMReader::NcmlCDMReader(std::string configFile) throw(CDMException)
    : configFile(configFile)
{
    doc = std::auto_ptr<XMLDoc>(new XMLDoc(configFile));
    XPathObjPtr xpathObj = doc->getXPathObject("/netcdf[@location]");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    if (nodes->nodeNr != 1) {
        throw CDMException("config-file "+configFile+" does not contain location-attribute");
    }
    string ncFile = getXmlProp(nodes->nodeTab[0], "location");
    dataReader = boost::shared_ptr<CDMReader>(new NetCDF_CF10_CDMReader(ncFile));
    init();
}

NcmlCDMReader::NcmlCDMReader(const boost::shared_ptr<CDMReader> dataReader, std::string configFile) throw(CDMException)
    : configFile(configFile), dataReader(dataReader)
{
    doc = std::auto_ptr<XMLDoc>(new XMLDoc(configFile));
    init();
}

void NcmlCDMReader::init() throw(CDMException)
{
    cdm = dataReader->getCDM();

    initRemove();
    initVariableNameChange();
    initDimensionNameChange();
    initAttributeNameChange();
    initWarnUnsupported();
}

void NcmlCDMReader::warnUnsupported(std::string xpath, std::string msg) {
    XPathObjPtr xpathObj = doc->getXPathObject(xpath);
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if (size > 0) {
        LOG4FIMEX(logger, Logger::INFO, msg);
    }
}

void NcmlCDMReader::initRemove() {
    XPathObjPtr xpathObj = doc->getXPathObject("/netcdf/remove");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        std::string name = getXmlProp(nodes->nodeTab[i], "name");
        if (name == "") throw CDMException("name attribute required for /netcdf/remove element in "+configFile);
        std::string type = getXmlProp(nodes->nodeTab[i], "type");
        if (type == "") throw CDMException("type attribute required for /netcdf/remove element in "+configFile);
        if (type == "variable") {
            cdm.removeVariable(name);
        } else if (type == "dimension") {
            cdm.removeDimension(name);
        } else if (type == "attribute") {
            cdm.removeAttribute(CDM::globalAttributeNS(), name);
        } else if (type == "group") {
            // groups not supported
        } else {
            LOG4FIMEX(logger, Logger::FATAL, "unknown type to remove in "+configFile);
            throw CDMException("unknown type to remove in "+configFile);
        }
    }


    /* remove the variable attributes */
    xpathObj = doc->getXPathObject("/netcdf/remove");
    nodes = xpathObj->nodesetval;
    size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        std::string name = getXmlProp(nodes->nodeTab[i], "name");
        if (name == "") throw CDMException("name attribute required for /netcdf/remove element in "+configFile);
        std::string type = getXmlProp(nodes->nodeTab[i], "type");
        if (type == "attribute") throw CDMException("type attribute required = 'attribute' for /netcdf/variable/remove element in "+configFile);
        std::string varName = getXmlProp(nodes->nodeTab[i]->parent, "name");
        if (varName == "") throw CDMException("name attribute required for all variable element in "+configFile);
        cdm.removeAttribute(varName, name);
    }
}

/* warn about features not supported in this class */
void NcmlCDMReader::initWarnUnsupported() {
    warnUnsupported("/netcdf/aggregation", "aggregation not supported");
    warnUnsupported("/netcdf/dimension[isUnlimited]", "setting of unlimited dimension not supported");
    warnUnsupported("/netcdf/group]","groups not supported");
}

void NcmlCDMReader::initVariableTypeChange()
{
    XPathObjPtr xpathObj = doc->getXPathObject("/netcdf/variable[@type]");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        std::string type = getXmlProp(nodes->nodeTab[i], "type");
        std::string name = getXmlProp(nodes->nodeTab[i], "name");
        if (name == "") throw CDMException("ncml-file "+ configFile + " has no name for variable");
        if (cdm.hasVariable(name)) {
            variableTypeChanges[name] = string2datatype(type);
            cdm.getVariable(name).setDataType(string2datatype(type));
        }
    }
}

void NcmlCDMReader::initAddReassignAttribute()
{
    XPathObjPtr xpathObj = doc->getXPathObject("/netcdf/attribute[@value]");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        std::string name = getXmlProp(nodes->nodeTab[i], "name");
        std::string value = getXmlProp(nodes->nodeTab[i], "value");
        std::string separator = getXmlProp(nodes->nodeTab[i], "separator");
        if (separator == "") separator = " ";
        vector<string> values = tokenize(value, separator);
        std::string type = getXmlProp(nodes->nodeTab[i], "type");
        if (type == "") type = "string";
        cdm.removeAttribute(CDM::globalAttributeNS(), name);
        cdm.addAttribute(CDM::globalAttributeNS(), CDMAttribute(name, string2datatype(type), values));
    }

    /* the variable attributes */
    xpathObj = doc->getXPathObject("/netcdf/variable/attribute[@value]");
    nodes = xpathObj->nodesetval;
    size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        std::string varName = getXmlProp(nodes->nodeTab[i]->parent, "name");
        std::string name = getXmlProp(nodes->nodeTab[i], "name");
        std::string value = getXmlProp(nodes->nodeTab[i], "value");
        std::string separator = getXmlProp(nodes->nodeTab[i], "separator");
        if (separator == "") separator = " ";
        vector<string> values = tokenize(value, separator);
        std::string type = getXmlProp(nodes->nodeTab[i], "type");
        if (type == "") type = "string";
        cdm.removeAttribute(varName, name);
        cdm.addAttribute(varName, CDMAttribute(name, string2datatype(type), values));
    }
}

/* change the variable names */
void NcmlCDMReader::initVariableNameChange()
{
    XPathObjPtr xpathObj = doc->getXPathObject("/netcdf/variable[@orgName]");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        std::string orgName = getXmlProp(nodes->nodeTab[i], "orgName");
        if (cdm.hasVariable(orgName)) {
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            if (name == "") throw CDMException("ncml-file "+ configFile + " has no name for variable with orgName: "+ orgName);
            cdm.renameVariable(orgName, name);
            variableNameChanges[name] = orgName;
        }
    }
}

/* change the dimension names */
void NcmlCDMReader::initDimensionNameChange()
{
    XPathObjPtr xpathObj = doc->getXPathObject("/netcdf/dimension[@orgName]");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        std::string orgName = getXmlProp(nodes->nodeTab[i], "orgName");
        if (cdm.hasVariable(orgName)) {
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            if (name == "") throw CDMException("ncml-file "+ configFile + " has no name for dimension with orgName: "+ orgName);
            cdm.renameDimension(orgName, name);
        }
    }
}

/* change the attribute names */
void NcmlCDMReader::initAttributeNameChange()
{
    { /* the global attributes */
        XPathObjPtr xpathObj = doc->getXPathObject("/netcdf/attribute[@orgName]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        for (int i = 0; i < size; i++) {
            std::string orgName = getXmlProp(nodes->nodeTab[i], "orgName");
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            if (name == "") throw CDMException("ncml-file "+ configFile + " has no name for attribute with orgName: "+ orgName);
            try {
                CDMAttribute& attr = cdm.getAttribute(CDM::globalAttributeNS(), orgName);
                cdm.removeAttribute(CDM::globalAttributeNS(), name); // remove other attributes with same name
                attr.setName(name);
            } catch (CDMException ex) {
                // no such attribute, don't care
            }
        }
    }
    { /* the variable attributes */
        XPathObjPtr xpathObj = doc->getXPathObject("/netcdf/variable[@name]/attribute[@orgName]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        for (int i = 0; i < size; i++) {
            std::string varName = getXmlProp(nodes->nodeTab[i]->parent, "name");
            std::string orgName = getXmlProp(nodes->nodeTab[i], "orgName");
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            if (name == "") throw CDMException("ncml-file "+ configFile + " has no name for attribute with orgName: "+ orgName);
            try {
                CDMAttribute& attr = cdm.getAttribute(varName, orgName);
                cdm.removeAttribute(varName, name); // remove other attributes with same name
                attr.setName(name);
            } catch (CDMException ex) {
                // no such attribute, don't care
            }
        }
    }
}


const boost::shared_ptr<Data> NcmlCDMReader::getDataSlice(const std::string& varName, size_t unLimDimPos) throw(CDMException)
{
    // TODO Auto-generated constructor stub

}


}