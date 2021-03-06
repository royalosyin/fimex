EXTRA_DIST = testInterpolatorFill.sh testCharNc.sh.in testFiIndexGribs.sh.in\
             testVerticalInterpolator.sh.in testVerticalInterpolator2.sh.in \
             testVerticalInterpolatorHeight.sh.in testVerticalInterpolatorOceanSG2ToDepth.sh.in \
             testQualityExtractorFimex.sh.in testMerger.sh.in testMergerTarget.sh.in \
             testFeltReadNetCDFWrite.sh testNetCDFReadWrite.sh testNcmlRead.sh \
             testInterpolation2DataFillValue.sh.in testdata_int2data_in.nc testdata_int2data_ex.nc \
             testNcString.sh.in ncStringInput.nc ncStringExpected.nc ncStringWriter.xml \
             nccmp.sh \
             inData.txt diana.setup felt2netcdf.cfg \
             cdmGribReaderConfig_newEarth.xml cdmGribWriterConfig_append.xml \
             felt2nc_variables_level1000.xml \
             test.ncml fillInterpolator.cfg testXMLDoc.xml coordTest.nc char.nc \
             satellite_cma.nc \
             testQualityConfig.xml testQEmask_data.nc testQEmask_mask.nc testQEmask.xml.in \
             coordRefTimeTest.nc test.ncml twoCoordsTest.nc \
             verticalPressure.nc verticalSigmaHybrid.nc verticalPressurePressure.nc \
             verticalSigmaHybridPressure.nc verticalPressureHeight.nc \
             verticalOceanSG2.nc verticalOceanSG2_depth.nc \
             sample_ed1.gm sample_ed2.gm \
             erai.sfc.40N.0.75d.200301011200.nc template_noaa17.nc  c11.nc c11.ncml \
             merge_target_base.nc merge_target_top.nc \
             test_merge_inner.nc test_merge_outer.nc fillIn2.nc fillIn3.nc fillWriterConfig.xml \
             testdata_arome_vc.nc verticalVelocity.nc \
             data/joinExistingAgg.ncml data/joinExistingAgg1.nc \
             data/joinExistingAgg3.nc data/joinExistingAgg4.nc \
             data/joinExistingAggSuffix.ncml data/aggNothing.ncml data/aggWrong.ncml \
             data/unionAgg.ncml data/unionAgg2.nc3 data/north.nc

CLEANFILES = test.nc test2.nc test3.nc test4.nc out.nc out*.nc\
             testQualityExtractor.nc test_0time.nc \
             testInterpolator.nc testInterpolator2.nc testInterpolator3.nc \
             coordTestFilled.nc test.grb1 test.grb2 \
             bicubicOutData.txt bilinearOutData.txt \
             testMetgmReadEd1.nc testMetgmReadEd2.nc \
             hirlam12.mgm hirlam12.mgm.nc testGribRead.nc \
             test_netcdfrw.nc fillOut.nc \
             *.log *.trs

CC_TESTS = testInterpolation \
           testVerticalCoordinates \
           testBinaryConstants \
           testUtils \
           testTimeSpec \
           testSpatialAxisSpec \
           testData \
           testProjections \
           testUnits \
           testFileReaderFactory \
           testCDM \
           testSliceBuilder \
           testXMLDoc \
           testFeltReader

SH_TESTS = 
if ENABLE_FORTRAN
LDADD = $(top_builddir)/modules/F90/libfimexf.la

$(top_builddir)/modules/F90/libfimexf.la:
	$(MAKE) -C $(top_builddir)/modules/F90 libfimexf.la
else
LDADD =
endif
LDADD += ../src/libfimex.la ../src/felt/libfelt.la $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

AM_CPPFLAGS = -DTOP_SRCDIR=\"$(top_srcdir)\" -I$(top_srcdir)/include -I$(top_srcdir)/src \
              ${milib_CFLAGS} \
              @MIFI_UDUNITS_CPPFLAGS@ @MIFI_UDUNITS2_CPPFLAGS@ \
              @MIFI_PROJ_CPPFLAGS@ @XML_CPPFLAGS@ \
              @BOOST_CPPFLAGS@
AM_CXXFLAGS = 
AM_CFLAGS =
AM_FCFLAGS =  -DTOP_SRCDIR=\"$(top_srcdir)\" -I../modules/F90/

if HAVE_FELT
  CC_TESTS += testQualityExtractor
  NETCDF_MI_SH_TESTS = testFeltReadNetCDFWrite.sh testNetCDFReadWrite.sh testNcmlRead.sh
  NETCDF_MI_TESTS = testNetcdfWriter testExtractor testInterpolator testTimeInterpolator test_c_consumer
  GRIBAPI_MI_TESTS = testGribWriter
endif
if HAVE_NETCDF
  CC_TESTS += ${NETCDF_MI_TESTS} testCoordSys \
              testCoordinateSystemSliceBuilder  testNcmlReader \
              testNcmlAggregationReader \
              testProcessor testMerger testNetCDFReaderWriter \
              testFillWriter \
              testVerticalVelocity testVLevelConverter

  GRIBAPI_NC_TESTS = testGribReader
  METGM_NC_TESTS = testMetGmWriter 
  SH_TESTS += ${NETCDF_MI_SH_TESTS} testCharNc.sh \
              testInterpolatorFill.sh testInterpolation2DataFillValue.sh \
              testVerticalInterpolator.sh testVerticalInterpolator2.sh \
              testVerticalInterpolatorHeight.sh testVerticalInterpolatorOceanSG2ToDepth.sh \
              testNcString.sh \
              testQualityExtractorFimex.sh testMerger.sh testMergerTarget.sh
endif
if HAVE_GRIBAPI
  CC_TESTS += ${GRIBAPI_MI_TESTS} ${GRIBAPI_NC_TESTS}
  SH_TESTS += testFiIndexGribs.sh
endif
if HAVE_METGM
  CC_TESTS += testMetGmReader ${METGM_NC_TESTS}
endif
include wdb/make.mk
if ENABLE_FORTRAN
if HAVE_FELT
if HAVE_NETCDF
  CC_TESTS += testFortran
endif
endif
endif

check_SCRIPTS = $(SH_TESTS)
TESTS = $(CC_TESTS) $(SH_TESTS)

check_PROGRAMS = $(CC_TESTS)
testBinaryConstants_SOURCES = testBinaryConstants.cc
testUtils_SOURCES = testUtils.cc
testUtils_LDADD = $(LDADD) @BOOST_REGEX_LIB@
testInterpolation_SOURCES = testInterpolation.cc
testVerticalCoordinates_SOURCES = testVerticalCoordinates.cc
testVLevelConverter_SOURCES = testVLevelConverter.cc
testSpatialAxisSpec_SOURCES = testSpatialAxisSpec.cc
testTimeSpec_SOURCES = testTimeSpec.cc
testData_SOURCES = testData.cc
testCDM_SOURCES = testCDM.cc
testSliceBuilder_SOURCES = testSliceBuilder.cc
testProjections_SOURCES = testProjections.cc
testCoordSys_SOURCES = testCoordSys.cc 
testCoordinateSystemSliceBuilder_SOURCES = testCoordinateSystemSliceBuilder.cc
testFeltReader_SOURCES = testFeltReader.cc
testNetcdfWriter_SOURCES = testNetcdfWriter.cc
testFillWriter_SOURCES = testFillWriter.cc
testNetCDFReaderWriter_SOURCES = testNetCDFReaderWriter.cc
testNcmlReader_SOURCES = testNcmlReader.cc
testNcmlAggregationReader_SOURCES = testNcmlAggregationReader.cc
testMetGmReader_SOURCES = testMetGmReader.cc
testMetGmWriter_SOURCES = testMetGmWriter.cc
testMerger_SOURCES = testMerger.cc
testGribReader_SOURCES = testGribReader.cc
testGribReader_LDADD = $(LDADD) @BOOST_SYSTEM_LIB@
testGribWriter_SOURCES = testGribWriter.cc
testGribWriter_LDADD = $(LDADD) @BOOST_FILESYSTEM_LIB@ @BOOST_SYSTEM_LIB@
testFileReaderFactory_SOURCES = testFileReaderFactory.cc
testExtractor_SOURCES = testExtractor.cc
testProcessor_SOURCES = testProcessor.cc
testQualityExtractor_SOURCES = testQualityExtractor.cc
testInterpolator_SOURCES = testInterpolator.cc
testTimeInterpolator_SOURCES = testTimeInterpolator.cc
testUnits_SOURCES = testUnits.cc
testVerticalVelocity_SOURCES = testVerticalVelocity.cc
testXMLDoc_SOURCES = testXMLDoc.cc
test_c_consumer_SOURCES = test_c_consumer.c
testFortran_SOURCES = testFortran.F90

build-checks: all-am
	$(MAKE) $(AM_MAKEFLAGS) $(check_PROGRAMS) $(check_SCRIPTS)
