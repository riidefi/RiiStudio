#pragma once

#include <LibCore/common.h>
#include <Lib3D/interface/i3dmodel.hpp>

#include <LibCore/api/Node.hpp>

namespace libcube {

    
struct IBoneDelegate : public lib3d::Bone
{
	PX_TYPE_INFO("GC Bone", "gc_bone", "GC::IBoneDelegate");

    enum class Billboard
    {
        None,
        /* G3D Standard */
        Z_Parallel,
        Z_Face, // persp
        Z_ParallelRotate,
        Z_FaceRotate,
        /* G3D Y*/
        Y_Parallel,
        Y_Face, // persp

        // TODO -- merge
        J3D_XY,
        J3D_Y
    };
    virtual Billboard getBillboard() const = 0;
    virtual void setBillboard(Billboard b) = 0;
    // For extendeds
    virtual s64 getBillboardAncestor() { return -1; }
    virtual void setBillboardAncestor(s64 ancestor_id) {}

    void copy(lib3d::Bone& to) override
    {
        lib3d::Bone::copy(to);
        IBoneDelegate* bone = reinterpret_cast<IBoneDelegate*>(&to);
        if (bone)
        {
            if (bone->supportsBoneFeature(lib3d::BoneFeatures::StandardBillboards) == lib3d::Coverage::ReadWrite)
                bone->setBillboard(getBillboard());
            if (bone->supportsBoneFeature(lib3d::BoneFeatures::ExtendedBillboards) == lib3d::Coverage::ReadWrite)
                bone->setBillboardAncestor(getBillboardAncestor());
        }
    }
    inline bool canWrite(lib3d::BoneFeatures f)
    {
        return supportsBoneFeature(f) == lib3d::Coverage::ReadWrite;
    }
};


}
