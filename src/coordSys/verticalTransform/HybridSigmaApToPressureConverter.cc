
#include "fimex/coordSys/verticalTransform/HybridSigmaApToPressureConverter.h"

#include "fimex/CDM.h"
#include "fimex/CDMReader.h"
#include "fimex/CDMReaderUtils.h"
#include "fimex/coordSys/CoordinateSystem.h"
#include "fimex/coordSys/verticalTransform/VerticalTransformationUtils.h"
#include "fimex/Data.h"
#include "fimex/vertical_coordinate_transformations.h"

#include <boost/make_shared.hpp>

#include <vector>

namespace MetNoFimex {

using std::vector;

// static method
VerticalConverterPtr HybridSigmaApToPressureConverter::createConverter(CDMReaderPtr reader, CoordSysPtr cs,
    const std::string& ap, const std::string& b, const std::string& ps, const std::string& p0)
{
    return boost::make_shared<HybridSigmaApToPressureConverter>(reader, cs, ap, b, ps, p0);
}

std::vector<std::string> HybridSigmaApToPressureConverter::getShape() const
{
    const CDM& rcdm = reader_->getCDM();
    // combine shapes of a, b (length = height) and ps (other dimensions)
    std::vector<std::string> shape = rcdm.getVariable(ps_).getShape();

    bool have_z = false;
    if (cs_->getGeoZAxis()) {
        const std::string& cs_z_name = cs_->getGeoZAxis()->getName();

        // FIXME listCoordinateSystems probably makes this very slow ...
        std::vector<boost::shared_ptr<const CoordinateSystem> > css = listCoordinateSystems(reader_);
        boost::shared_ptr<const CoordinateSystem> cs_ps = findCompleteCoordinateSystemFor(css, ps_);
        if (cs_ps && cs_ps->getGeoZAxis()) {
            const std::string& ps_z_name = cs_ps->getGeoZAxis()->getName();
            std::vector<std::string>::iterator it_z = std::find(shape.begin(), shape.end(), ps_z_name);
            if (it_z != shape.end()) {
                *it_z = cs_z_name;
                have_z = true;
            }
        }
        if (!have_z) {
            std::vector<std::string>::iterator it_z = shape.end();
            if (rcdm.hasUnlimitedDim(rcdm.getVariable(ps_)))
                --it_z;
            shape.insert(it_z, cs_z_name);
            have_z = true;
        }
    }
    if (!have_z)
        throw CDMException("no vertical axis for HybridSigmaApToPressureConverter shape");

    return shape;
}

DataPtr HybridSigmaApToPressureConverter::getDataSlice(const SliceBuilder& sb) const
{
    int unLimDimPos = 0; // TODO is this correct?
    const vector<double> apVec = getDataSliceInUnit(reader_, ap_, "hPa", unLimDimPos);
    const vector<double> bVec = getDataSliceInUnit(reader_, b_, "", unLimDimPos);
    // FIXME never used const vector<double> p0Vec = getDataSliceInUnit(reader_, p0_, "hPa", unLimDimPos);

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
                mifi_atmosphere_hybrid_sigma_ap_pressure(1, psVal[idxSurface], &apVec[l], &bVec[l], &pVal[idx]);
            }
        }
    }
    return createData(size, pVal);
}

} // namespace MetNoFimex
