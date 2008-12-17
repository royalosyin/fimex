/*
 * Fimex, TimeSpec.cc
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
 *
 *  Created on: Dec 11, 2008
 *      Author: Heiko Klein
 */

#include "fimex/TimeSpec.h"
#include "fimex/Utils.h"
#include <vector>
#include <algorithm>
#include <iterator>
#include "fimex/Logger.h"
#include "fimex/TimeUnit.h"

namespace MetNoFimex
{

using namespace std;

TimeSpec::TimeSpec(const string& timeSpec, const FimexTime& startTime, const FimexTime& endTime) throw(CDMException)
	: outputUnit("seconds since 1970-01-01 00:00:00")
{
	LoggerPtr logger = getLogger("fimex.TimeSpec");
	LOG4FIMEX(logger, Logger::DEBUG, "getting timespec of " << timeSpec);
	vector<string> toks = tokenize(timeSpec, ";");
	std::string timeStepStr;
	std::string unit;
	std::string relativeStart;
	for(vector<string>::iterator it = toks.begin(); it != toks.end(); ++it) {
		vector<string> extraParam = tokenize(*it, "=");
		if (extraParam.size() == 1) {
			if (timeStepStr == "") {
				timeStepStr = extraParam[0];
			} else {
				throw CDMException("time-steps redefined from " + timeStepStr + " to " + extraParam[0]);
			}
		} else if (extraParam.size() == 2) {
			if (extraParam[0] == "unit") {
				unit = extraParam[1];
			} else if (extraParam[0] == "relativeStart") {
				relativeStart = extraParam[1];
			} else {
				throw CDMException("unknown timeSpec parameter: '" + extraParam[0] + "'");
			}
		} else {
			throw CDMException("unknown timeSpec section: '" + *it + "'");
		}
	}

	if (relativeStart == "") {
		// absolute time in spec
		vector<string> times = tokenize(timeStepStr, ",");
		transform(times.begin(), times.end(), back_inserter(timeSteps), string2FimexTime);
	} else {
		// relative times
		throw CDMException("relative times not implemented yet");
	}
	if (unit != "") {
		throw CDMException("unit not implemented yet in TimeSpec");
	}

	LOG4FIMEX(logger, Logger::DEBUG, "got parameters unit:" << unit << ";relativeStart:" << relativeStart << ";steps:"<<timeStepStr);
}


} /* MetNoFimex */