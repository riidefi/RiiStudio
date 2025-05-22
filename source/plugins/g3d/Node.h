#include <librii/g3d/io/AnimChrIO.hpp>
#include <librii/g3d/io/AnimClrIO.hpp>
#include <librii/g3d/io/AnimTexPatIO.hpp>
#include <librii/g3d/io/AnimVisIO.hpp>
#include <librii/g3d/data/Archive.hpp>

namespace riistudio::g3d {

class Model : public libcube::Model, public kpi::TDocData<riistudio::g3d::G3DModelData>, public virtual kpi::IObject {
public:
    Model() = default;
    virtual ~Model() = default;

    kpi::MutCollectionRange<Material> getMaterials() { return { &mMaterials }; }
    kpi::MutCollectionRange<Bone> getBones() { return { &mBones }; }
    kpi::MutCollectionRange<Polygon> getMeshes() { return { &mMeshes }; }
    kpi::MutCollectionRange<PositionBuffer> getBuf_Pos() { return { &mBuf_Pos }; }
    kpi::MutCollectionRange<NormalBuffer> getBuf_Nrm() { return { &mBuf_Nrm }; }
    kpi::MutCollectionRange<ColorBuffer> getBuf_Clr() { return { &mBuf_Clr }; }
    kpi::MutCollectionRange<TextureCoordinateBuffer> getBuf_Uv() { return { &mBuf_Uv }; }
    kpi::ConstCollectionRange<Material> getMaterials() const { return { &mMaterials }; }
    kpi::ConstCollectionRange<Bone> getBones() const { return { &mBones }; }
    kpi::ConstCollectionRange<Polygon> getMeshes() const { return { &mMeshes }; }
    kpi::ConstCollectionRange<PositionBuffer> getBuf_Pos() const { return { &mBuf_Pos }; }
    kpi::ConstCollectionRange<NormalBuffer> getBuf_Nrm() const { return { &mBuf_Nrm }; }
    kpi::ConstCollectionRange<ColorBuffer> getBuf_Clr() const { return { &mBuf_Clr }; }
    kpi::ConstCollectionRange<TextureCoordinateBuffer> getBuf_Uv() const { return { &mBuf_Uv }; }

protected:
    kpi::ICollection* v_getMaterials() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mMaterials)); }
    kpi::ICollection* v_getBones() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mBones)); }
    kpi::ICollection* v_getMeshes() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mMeshes)); }
    kpi::ICollection* v_getBuf_Pos() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mBuf_Pos)); }
    kpi::ICollection* v_getBuf_Nrm() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mBuf_Nrm)); }
    kpi::ICollection* v_getBuf_Clr() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mBuf_Clr)); }
    kpi::ICollection* v_getBuf_Uv() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mBuf_Uv)); }
    void onRelocate() {
        mMaterials.onParentMoved(this);
        mBones.onParentMoved(this);
        mMeshes.onParentMoved(this);
        mBuf_Pos.onParentMoved(this);
        mBuf_Nrm.onParentMoved(this);
        mBuf_Clr.onParentMoved(this);
        mBuf_Uv.onParentMoved(this);
    }

public:
    Model(Model&& rhs) noexcept {
        new (&mMaterials) decltype(mMaterials) (std::move(rhs.mMaterials));
        new (&mBones) decltype(mBones) (std::move(rhs.mBones));
        new (&mMeshes) decltype(mMeshes) (std::move(rhs.mMeshes));
        new (&mBuf_Pos) decltype(mBuf_Pos) (std::move(rhs.mBuf_Pos));
        new (&mBuf_Nrm) decltype(mBuf_Nrm) (std::move(rhs.mBuf_Nrm));
        new (&mBuf_Clr) decltype(mBuf_Clr) (std::move(rhs.mBuf_Clr));
        new (&mBuf_Uv) decltype(mBuf_Uv) (std::move(rhs.mBuf_Uv));

        onRelocate();
    }
    Model(const Model& rhs) {
        new (&mMaterials) decltype(mMaterials) (rhs.mMaterials);
        new (&mBones) decltype(mBones) (rhs.mBones);
        new (&mMeshes) decltype(mMeshes) (rhs.mMeshes);
        new (&mBuf_Pos) decltype(mBuf_Pos) (rhs.mBuf_Pos);
        new (&mBuf_Nrm) decltype(mBuf_Nrm) (rhs.mBuf_Nrm);
        new (&mBuf_Clr) decltype(mBuf_Clr) (rhs.mBuf_Clr);
        new (&mBuf_Uv) decltype(mBuf_Uv) (rhs.mBuf_Uv);

        onRelocate();
    }
    Model& operator=(Model&& rhs) {
        new (this) Model (std::move(rhs));
        onRelocate();
        return *this;
    }
    Model& operator=(const Model& rhs) {
        new (this) Model (rhs);
        onRelocate();
        return *this;
    }

private:
    kpi::CollectionImpl<Material> mMaterials{this};
    kpi::CollectionImpl<Bone> mBones{this};
    kpi::CollectionImpl<Polygon> mMeshes{this};
    kpi::CollectionImpl<PositionBuffer> mBuf_Pos{this};
    kpi::CollectionImpl<NormalBuffer> mBuf_Nrm{this};
    kpi::CollectionImpl<ColorBuffer> mBuf_Clr{this};
    kpi::CollectionImpl<TextureCoordinateBuffer> mBuf_Uv{this};

public:
    struct _Memento : public kpi::IMemento {
        riistudio::g3d::G3DModelData d;
        kpi::ConstPersistentVec<Material> mMaterials;
        kpi::ConstPersistentVec<Bone> mBones;
        kpi::ConstPersistentVec<Polygon> mMeshes;
        kpi::ConstPersistentVec<PositionBuffer> mBuf_Pos;
        kpi::ConstPersistentVec<NormalBuffer> mBuf_Nrm;
        kpi::ConstPersistentVec<ColorBuffer> mBuf_Clr;
        kpi::ConstPersistentVec<TextureCoordinateBuffer> mBuf_Uv;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            d = static_cast<const riistudio::g3d::G3DModelData&>(_new);
            kpi::nextFolder(this->mMaterials, _new.getMaterials(), old ? &old->mMaterials : nullptr);
            kpi::nextFolder(this->mBones, _new.getBones(), old ? &old->mBones : nullptr);
            kpi::nextFolder(this->mMeshes, _new.getMeshes(), old ? &old->mMeshes : nullptr);
            kpi::nextFolder(this->mBuf_Pos, _new.getBuf_Pos(), old ? &old->mBuf_Pos : nullptr);
            kpi::nextFolder(this->mBuf_Nrm, _new.getBuf_Nrm(), old ? &old->mBuf_Nrm : nullptr);
            kpi::nextFolder(this->mBuf_Clr, _new.getBuf_Clr(), old ? &old->mBuf_Clr : nullptr);
            kpi::nextFolder(this->mBuf_Uv, _new.getBuf_Uv(), old ? &old->mBuf_Uv : nullptr);
        }
    };
    std::unique_ptr<kpi::IMemento> next(const kpi::IMemento* last) const {
        return std::make_unique<_Memento>(*this, last);
    }
    void from(const kpi::IMemento& _memento) {
        auto* in = dynamic_cast<const _Memento*>(&_memento);
        assert(in);
        static_cast<riistudio::g3d::G3DModelData&>(*this) = in->d;
        kpi::fromFolder(getMaterials(), in->mMaterials);
        kpi::fromFolder(getBones(), in->mBones);
        kpi::fromFolder(getMeshes(), in->mMeshes);
        kpi::fromFolder(getBuf_Pos(), in->mBuf_Pos);
        kpi::fromFolder(getBuf_Nrm(), in->mBuf_Nrm);
        kpi::fromFolder(getBuf_Clr(), in->mBuf_Clr);
        kpi::fromFolder(getBuf_Uv(), in->mBuf_Uv);
    }
    template<typename T> void* operator=(const T& rhs) { from(rhs); return this; }
};

struct SceneData {
    std::vector<librii::g3d::ChrAnim> chrs;
    std::vector<librii::g3d::PatAnim> pats;
    std::vector<librii::g3d::BinaryVis> viss;
    std::string path;

	bool operator==(const SceneData&) const = default;
};

class Collection
    : public libcube::Scene,
      public kpi::TDocData<SceneData>, public virtual kpi::IObject{
  public:
    Collection() = default;
    virtual ~Collection() = default;

    kpi::MutCollectionRange<Model> getModels() { return { &mModels }; }
    kpi::MutCollectionRange<Texture> getTextures() { return { &mTextures }; }
    kpi::MutCollectionRange<SRT0> getAnim_Srts() { return { &mAnim_Srts }; }
    kpi::MutCollectionRange<CLR0> getAnim_Clrs() { return { &mAnim_Clrs }; }
    kpi::ConstCollectionRange<Model> getModels() const { return { &mModels }; }
    kpi::ConstCollectionRange<Texture> getTextures() const { return { &mTextures }; }
    kpi::ConstCollectionRange<SRT0> getAnim_Srts() const { return { &mAnim_Srts }; }
    kpi::ConstCollectionRange<CLR0> getAnim_Clrs() const { return { &mAnim_Clrs }; }

	librii::g3d::Archive toLibRii() const;

protected:
    kpi::ICollection* v_getModels() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mModels)); }
    kpi::ICollection* v_getTextures() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mTextures)); }
    kpi::ICollection* v_getAnim_Srts() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mAnim_Srts)); }
    kpi::ICollection* v_getAnim_Clrs() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mAnim_Clrs)); }
    void onRelocate() {
        mModels.onParentMoved(this);
        mTextures.onParentMoved(this);
        mAnim_Srts.onParentMoved(this);
        mAnim_Clrs.onParentMoved(this);
    }
    Collection(Collection&& rhs) {
        new (&mModels) decltype(mModels) (std::move(rhs.mModels));
        new (&mTextures) decltype(mTextures) (std::move(rhs.mTextures));
        new (&mAnim_Srts) decltype(mAnim_Srts) (std::move(rhs.mAnim_Srts));
        new (&mAnim_Clrs) decltype(mAnim_Clrs) (std::move(rhs.mAnim_Clrs));

        onRelocate();
    }
    Collection(const Collection& rhs) {
        new (&mModels) decltype(mModels) (rhs.mModels);
        new (&mTextures) decltype(mTextures) (rhs.mTextures);
        new (&mAnim_Srts) decltype(mAnim_Srts) (rhs.mAnim_Srts);
        new (&mAnim_Clrs) decltype(mAnim_Clrs) (rhs.mAnim_Clrs);

        onRelocate();
    }
    Collection& operator=(Collection&& rhs) {
        new (this) Collection (std::move(rhs));
        onRelocate();
        return *this;
    }
    Collection& operator=(const Collection& rhs) {
        new (this) Collection (rhs);
        onRelocate();
        return *this;
    }

private:
    kpi::CollectionImpl<Model> mModels{this};
    kpi::CollectionImpl<Texture> mTextures{this};
    kpi::CollectionImpl<SRT0> mAnim_Srts{this};
    kpi::CollectionImpl<CLR0> mAnim_Clrs{this};

public:
    struct _Memento : public kpi::IMemento {
        SceneData sd;
        kpi::ConstPersistentVec<Model> mModels;
        kpi::ConstPersistentVec<Texture> mTextures;
        kpi::ConstPersistentVec<SRT0> mAnim_Srts;
        kpi::ConstPersistentVec<CLR0> mAnim_Clrs;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            sd = static_cast<const SceneData&>(_new);
            kpi::nextFolder(this->mModels, _new.getModels(), old ? &old->mModels : nullptr);
            kpi::nextFolder(this->mTextures, _new.getTextures(), old ? &old->mTextures : nullptr);
            kpi::nextFolder(this->mAnim_Srts, _new.getAnim_Srts(), old ? &old->mAnim_Srts : nullptr);
            kpi::nextFolder(this->mAnim_Clrs, _new.getAnim_Clrs(), old ? &old->mAnim_Clrs : nullptr);
        }
    };
    std::unique_ptr<kpi::IMemento> next(const kpi::IMemento* last) const {
        return std::make_unique<_Memento>(*this, last);
    }
    void from(const kpi::IMemento& _memento) {
        auto* in = dynamic_cast<const _Memento*>(&_memento);
        assert(in);
        static_cast<SceneData&>(*this) = in->sd;
        kpi::fromFolder(getModels(), in->mModels);
        kpi::fromFolder(getTextures(), in->mTextures);
        kpi::fromFolder(getAnim_Srts(), in->mAnim_Srts);
        kpi::fromFolder(getAnim_Clrs(), in->mAnim_Clrs);
    }
    template<typename T> void* operator=(const T& rhs) { from(rhs); return this; }
};

} // namespace riistudio::g3d
