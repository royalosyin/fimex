/*
 * Fimex, CDM_XMLConfigHelper.cc
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
 *  Created on: Sep 16, 2009
 *      Author: Heiko Klein
 */

#include "CDM_XMLConfigHelper.h"
#include "fimex/CDMVariable.h"
#include "fimex/CDMAttribute.h"
#include "fimex/ReplaceStringObject.h"
#include "fimex/XMLDoc.h"
#include <libxml/xpath.h>
#include <boost/regex.hpp>
#include "fimex/Utils.h"

namespace MetNoFimex {

static string replaceTemplateAttribute(string value, const map<string, boost::shared_ptr<ReplaceStringObject> > templateReplacements) {
    for (map<string, boost::shared_ptr<ReplaceStringObject> >::const_iterator it = templateReplacements.begin(); it != templateReplacements.end(); ++it) {
        boost::smatch matches;
        boost::regex rgx(boost::regex(".*%" + it->first + "(\\(([^,]*),?(.*)?\\))?" + "%.*"));
        if (boost::regex_match(value, matches, rgx)) {
            boost::shared_ptr<ReplaceStringObject> rso = it->second;
            if (matches.size() > 2) {
                std::vector<std::string> options;
                if (matches.size() > 3) {
                    options = tokenize(matches[3], ",");
                }
                // values within the inner brackets
                rso->setFormatStringAndOptions(matches[2], options);
            }
            stringstream ss;
            rso->put(ss);
            value = boost::regex_replace(value, rgx, ss.str());
        }
    }
    return value;
}

/**
 * read all <attribute .../> subnodes of this node and add them to attributes, replace values by templateReplacements as needed
 */
void fillAttributeListFromXMLNode(vector<CDMAttribute>& attributes, const xmlNodePtr node, const std::map<std::string, boost::shared_ptr<ReplaceStringObject> >& templateReplacements) {
    if (node == 0) return;
    if ((node->type == XML_ELEMENT_NODE) &&
        (string("attribute") == reinterpret_cast<const char *>(node->name))) {
            string name = getXmlProp(node, "name");
            string value = getXmlProp(node, "value");
            string type = getXmlProp(node, "type");

            value = replaceTemplateAttribute(value, templateReplacements);
            attributes.push_back(CDMAttribute(name,type,value));
    }
    fillAttributeListFromXMLNode(attributes, node->next, templateReplacements);
}

/**
 * read a xml-node retrieved by the xpathString and extract the nodes attributes and all <attributes> sub-elements with name, value and type
 *
 * @param doc the document to read from
 * @param xpathString the string leading to the node
 * @param xmlAttributes returns all attributes of the first node matched
 * @param varAttributes returns all <attribute .../> sub elements of this node
 * @param templateReplacements the CDMAttribute values may containt templates (%VAR%) wich are replaced by these values
 * @return number of nodes matched (only the first has been read)
 */
int readXPathNodeWithCDMAttributes(const XMLDoc& doc, const string& xpathString, std::map<string, string>& xmlAttributes, std::vector<CDMAttribute>& varAttributes, const map<string, boost::shared_ptr<ReplaceStringObject> >& templateReplacements) throw(CDMException)
{
    XPathObjPtr xpathObj = doc.getXPathObject(xpathString);
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if (size == 0) return 0;
    // only parsing node[0]
    xmlNodePtr node = nodes->nodeTab[0];
    if (node->type != XML_ELEMENT_NODE) {
        throw CDMException("xpath does not point to XML_ELEMENT_NODE: " + xpathString);
    }
    xmlAttrPtr attr = node->properties;
    while (attr != 0) {
        string name(reinterpret_cast<const char *>(attr->name));
        string value(reinterpret_cast<const char *>(attr->children->content));
        xmlAttributes[name] = value;
        attr = attr->next;
    }
    fillAttributeListFromXMLNode(varAttributes, node->children, templateReplacements);
    return size;
}

} // end of MetNoFimex