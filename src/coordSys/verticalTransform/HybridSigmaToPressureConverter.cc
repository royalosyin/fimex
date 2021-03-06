
#include "fimex/coordSys/verticalTransform/HybridSigmaToPressureConverter.h"

#include "fimex/CDM.h"
#include "fimex/CDMReader.h"
#include "fimex/CDMReaderUtils.h"
#include "fimex/coordSys/CoordinateSystem.h"
#include "fimex/coordSys/verticalTransform/VerticalTransformationUtils.h"
#include "fimex/Data.h"
#include "fimex/vertical_coordinate_transformations.h"

#include <boost/make_shared.hpp>

namespace MetNoFimex {

using std::vector;

// static method
VerticalConverterPtr HybridSigmaToPressureConverter::createConverter(CDMReaderPtr reader, CoordSysPtr cs,
    const std::string& a, const std::string& b, const std::string& ps, const std::string& p0)
{
    return boost::make_shared<HybridSigmaToPressureConverter>(reader, cs, a, b, ps, p0);
}

std::vector<std::string> HybridSigmaToPressureConverter::getShape() const
{
    // combine shapes of a, b (length = height) and ps (other dimensions)
    std::vector<std::string> shape = reader_->getCDM().getVariable(ps_).getShape();
    shape[2] = cs_->getGeoZAxis()->getName(); // FIXME this is a crazy hack!
    return shape;
}

DataPtr HybridSigmaToPressureConverter::getDataSlice(const SliceBuilder& sb) const
{
    int unLimDimPos = 0; // TODO is this correct?
    const vector<double> aVec = getDataSliceInUnit(reader_, a_, "", unLimDimPos);
    const vector<double> bVec = getDataSliceInUnit(reader_, b_, "", unLimDimPos);
    const double p0 = getDataSliceInUnit(reader_, p0_, "hPa", unLimDimPos).front();

    DataPtr psData = getSliceData(reader_, sb, ps_, "hPa");

    const size_t nx = getSliceSize(sb, cs_->getGeoXAxis()->getName());
    const size_t ny = getSliceSize(sb, cs_->getGeoYAxis()->getName());
    const size_t nl = getSliceSize(sb, cs_->getGeoZAxis()->getName());
    const size_t sizeXY = nx*ny, sizeXYZ = sizeXY*nl;
    const size_t size = psData->size() * nl, sizeOther = size / sizeXYZ;

    boost::shared_array<double> psVal = psData->asDouble();
    boost::shared_array<double> pVal(new double[size]);

    // loop over all dims but z, calculate pressure
    // FIXME the following code assumes that x,y,z are the lowest dimensions for all variables
    size_t offset = 0, offsetSurface = 0;
    for (size_t i=0; i<sizeOther; ++i, offset += sizeXYZ, offsetSurface += sizeXY) {
        for (size_t xy=0; xy < sizeXY; ++xy) {
            const size_t idxSurface = offsetSurface + xy;
            for (size_t l = 0; l < nl; l += 1) {
                const size_t idx = offset + xy + l*sizeXY;
                mifi_atmosphere_hybrid_sigma_pressure(1, p0, psVal[idxSurface], &aVec[l], &bVec[l], &pVal[idx]);
            }
        }
    }
    return createData(size, pVal);
}

} // namespace MetNoFimex
