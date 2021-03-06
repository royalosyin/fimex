/*
 * Fimex, testNcmlAggregationReader.cc
 *
 * (C) Copyright 2013, met.no
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
 *  Created on: Apr 17, 2013
 *      Author: heikok
 */

#include "../config.h"
#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
using boost::unit_test_framework::test_suite;

#include <unistd.h>
#include <iostream>
#include <fstream>
#include "fimex/CDMFileReaderFactory.h"
#include "fimex/CDMconstants.h"
#include "fimex/CDMReader.h"
#include "fimex/CDM.h"
#include "fimex/Data.h"
#include "fimex/Logger.h"

using namespace std;
using namespace MetNoFimex;

struct TestConfig {
    char oldDir[2048];
    TestConfig() {
        string topSrcDir(TOP_SRCDIR);
        char* cwd = getcwd(oldDir, 2048);
        assert(cwd == oldDir);
        if (chdir((topSrcDir+"/test/data").c_str()) != 0) {
            std::cerr << "cannot go to: " << (topSrcDir+"/test/data") << endl;
            BOOST_CHECK(false);
        }
    }
    ~TestConfig() {
        if (chdir(oldDir) != 0) {
            std::cerr << "cannot go to: " << oldDir << endl;
            BOOST_CHECK(false);
        }
    }
};

BOOST_GLOBAL_FIXTURE(TestConfig)

BOOST_AUTO_TEST_CASE( test_joinExisting )
{
    //defaultLogLevel(Logger::DEBUG);
    string ncmlName("joinExistingAgg.ncml");
    if (!ifstream(ncmlName.c_str())) {
        // no testfile
        BOOST_CHECK(false);
        return;
    }
    boost::shared_ptr<CDMReader> reader(CDMFileReaderFactory::create(MIFI_FILETYPE_NCML, ncmlName));
    BOOST_CHECK(reader->getCDM().getUnlimitedDim()->getLength() == 5);
    BOOST_CHECK(reader->getDataSlice("unlim", 3)->asShort()[0] == 4);
    BOOST_CHECK(reader->getDataSlice("multi", 3)->asShort()[1] == -4);

    SliceBuilder sb(reader->getCDM(), "multi");
    sb.setStartAndSize("unlim", 3, 1);
    BOOST_CHECK(reader->getDataSlice("multi", sb)->asShort()[1] == -4);

    sb = SliceBuilder(reader->getCDM(), "unlim");
    sb.setStartAndSize("unlim", 3, 1);
    BOOST_CHECK(reader->getDataSlice("unlim", sb)->asShort()[0] == 4);

    BOOST_CHECK(true);

}

BOOST_AUTO_TEST_CASE( test_joinExistingSuffix )
{
    //defaultLogLevel(Logger::DEBUG);
    string ncmlName("joinExistingAggSuffix.ncml");
    if (!ifstream(ncmlName.c_str())) {
        // no testfile
        BOOST_CHECK(false);
        return;
    }
    boost::shared_ptr<CDMReader> reader(CDMFileReaderFactory::create(MIFI_FILETYPE_NCML, ncmlName));
    BOOST_CHECK(true);
    BOOST_CHECK(reader->getCDM().getUnlimitedDim()->getLength() == 5);
    BOOST_CHECK(reader->getDataSlice("unlim", 3)->asShort()[0] == 4);
    BOOST_CHECK(reader->getDataSlice("multi", 3)->asShort()[1] == -4);
}

BOOST_AUTO_TEST_CASE( test_aggNothing )
{
    //defaultLogLevel(Logger::DEBUG);
    string ncmlName("aggNothing.ncml");
    if (!ifstream(ncmlName.c_str())) {
        // no testfile
        BOOST_CHECK(false);
        return;
    }
    boost::shared_ptr<CDMReader> reader(CDMFileReaderFactory::create(MIFI_FILETYPE_NCML, ncmlName));
    BOOST_CHECK(true);
    BOOST_CHECK(reader->getCDM().getVariables().size() == 0);

}

BOOST_AUTO_TEST_CASE( test_aggWrong )
{
    //defaultLogLevel(Logger::DEBUG);
    string ncmlName("aggWrong.ncml");
    if (!ifstream(ncmlName.c_str())) {
        // no testfile
        BOOST_CHECK(false);
        return;
    }
    // suppress ERROR: file unreadable
    defaultLogLevel(Logger::FATAL);
    boost::shared_ptr<CDMReader> reader(CDMFileReaderFactory::create(MIFI_FILETYPE_NCML, ncmlName));
    BOOST_CHECK(true);
    defaultLogLevel(Logger::INFO);
    BOOST_CHECK(reader->getCDM().getVariables().size() == 0);
}


BOOST_AUTO_TEST_CASE( test_union )
{
    //defaultLogLevel(Logger::DEBUG);
    string ncmlName("unionAgg.ncml");
    if (!ifstream(ncmlName.c_str())) {
        // no testfile
        BOOST_CHECK(false);
        return;
    }
    boost::shared_ptr<CDMReader> reader(CDMFileReaderFactory::create(MIFI_FILETYPE_NCML, ncmlName));
    BOOST_CHECK(true);
    BOOST_CHECK(reader->getCDM().hasVariable("b"));
    BOOST_CHECK(reader->getCDM().hasVariable("extra"));
    BOOST_CHECK(reader->getCDM().hasVariable("multi"));
    BOOST_CHECK(reader->getCDM().hasVariable("unlim"));
    BOOST_CHECK(reader->getDataSlice("extra", 1)->asShort()[0] == -1);
    BOOST_CHECK(reader->getDataSlice("multi", 1)->asShort()[1] == -2);

    BOOST_CHECK(reader->getCDM().hasVariable("multi"));

    SliceBuilder sb(reader->getCDM(), "multi");
    sb.setStartAndSize("unlim", 1, 1);
    BOOST_CHECK(reader->getDataSlice("multi", sb)->asShort()[1] == -2);

}


#else
// no boost testframework
int main(int argc, char* args[]) {
}
#endif
