// This is a generated file

namespace riistudio::g3d {

class Model : public libcube::Model, public kpi::TDocData<riistudio::g3d::G3DModelData>, public kpi::INode {
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
    Model(Model&& rhs) {
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

    // INode implementations
    std::size_t numFolders() const override { return 7; }
    const kpi::ICollection* folderAt(std::size_t index) const override {
        switch (index) {
        case 0: return &mMaterials;
        case 1: return &mBones;
        case 2: return &mMeshes;
        case 3: return &mBuf_Pos;
        case 4: return &mBuf_Nrm;
        case 5: return &mBuf_Clr;
        case 6: return &mBuf_Uv;
        default: return nullptr;
        }
    }
    kpi::ICollection* folderAt(std::size_t index) override {
        switch (index) {
        case 0: return &mMaterials;
        case 1: return &mBones;
        case 2: return &mMeshes;
        case 3: return &mBuf_Pos;
        case 4: return &mBuf_Nrm;
        case 5: return &mBuf_Clr;
        case 6: return &mBuf_Uv;
        default: return nullptr;
        }
    }
    const char* idAt(std::size_t index) const override {
        switch (index) {
        case 0: return typeid(Material).name();
        case 1: return typeid(Bone).name();
        case 2: return typeid(Polygon).name();
        case 3: return typeid(PositionBuffer).name();
        case 4: return typeid(NormalBuffer).name();
        case 5: return typeid(ColorBuffer).name();
        case 6: return typeid(TextureCoordinateBuffer).name();
        default: return nullptr;
        }
    }
    std::size_t fromId(const char* id) const override {
        if (!strcmp(id, typeid(Material).name())) return 0;
        if (!strcmp(id, typeid(Bone).name())) return 1;
        if (!strcmp(id, typeid(Polygon).name())) return 2;
        if (!strcmp(id, typeid(PositionBuffer).name())) return 3;
        if (!strcmp(id, typeid(NormalBuffer).name())) return 4;
        if (!strcmp(id, typeid(ColorBuffer).name())) return 5;
        if (!strcmp(id, typeid(TextureCoordinateBuffer).name())) return 6;
        return ~0;
    }
    virtual kpi::IDocData* getImmediateData() { return static_cast<kpi::TDocData<riistudio::g3d::G3DModelData>*>(this); }
    virtual const kpi::IDocData* getImmediateData() const { return static_cast<const kpi::TDocData<riistudio::g3d::G3DModelData>*>(this); }

public:
    struct _Memento : public kpi::IMemento {
        kpi::ConstPersistentVec<Material> mMaterials;
        kpi::ConstPersistentVec<Bone> mBones;
        kpi::ConstPersistentVec<Polygon> mMeshes;
        kpi::ConstPersistentVec<PositionBuffer> mBuf_Pos;
        kpi::ConstPersistentVec<NormalBuffer> mBuf_Nrm;
        kpi::ConstPersistentVec<ColorBuffer> mBuf_Clr;
        kpi::ConstPersistentVec<TextureCoordinateBuffer> mBuf_Uv;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            kpi::nextFolder(this->mMaterials, _new.getMaterials(), old ? &old->mMaterials : nullptr);
            kpi::nextFolder(this->mBones, _new.getBones(), old ? &old->mBones : nullptr);
            kpi::nextFolder(this->mMeshes, _new.getMeshes(), old ? &old->mMeshes : nullptr);
            kpi::nextFolder(this->mBuf_Pos, _new.getBuf_Pos(), old ? &old->mBuf_Pos : nullptr);
            kpi::nextFolder(this->mBuf_Nrm, _new.getBuf_Nrm(), old ? &old->mBuf_Nrm : nullptr);
            kpi::nextFolder(this->mBuf_Clr, _new.getBuf_Clr(), old ? &old->mBuf_Clr : nullptr);
            kpi::nextFolder(this->mBuf_Uv, _new.getBuf_Uv(), old ? &old->mBuf_Uv : nullptr);
        }
    };
    std::unique_ptr<kpi::IMemento> next(const kpi::IMemento* last) const override {
        return std::make_unique<_Memento>(*this, last);
    }
    void from(const kpi::IMemento& _memento) override {
        auto* in = dynamic_cast<const _Memento*>(&_memento);
        assert(in);
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

} // namespace riistudio::g3d

namespace riistudio::g3d {

class Collection : public libcube::Scene, public kpi::INode {
public:
    Collection() = default;
    virtual ~Collection() = default;

	std::string path;

    kpi::MutCollectionRange<Model> getModels() { return { &mModels }; }
    kpi::MutCollectionRange<Texture> getTextures() { return { &mTextures }; }
    kpi::MutCollectionRange<SRT0> getAnim_Srts() { return { &mAnim_Srts }; }
    kpi::ConstCollectionRange<Model> getModels() const { return { &mModels }; }
    kpi::ConstCollectionRange<Texture> getTextures() const { return { &mTextures }; }
    kpi::ConstCollectionRange<SRT0> getAnim_Srts() const { return { &mAnim_Srts }; }

protected:
    kpi::ICollection* v_getModels() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mModels)); }
    kpi::ICollection* v_getTextures() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mTextures)); }
    kpi::ICollection* v_getAnim_Srts() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mAnim_Srts)); }
    void onRelocate() {
        mModels.onParentMoved(this);
        mTextures.onParentMoved(this);
        mAnim_Srts.onParentMoved(this);
    }
    Collection(Collection&& rhs) {
        new (&mModels) decltype(mModels) (std::move(rhs.mModels));
        new (&mTextures) decltype(mTextures) (std::move(rhs.mTextures));
        new (&mAnim_Srts) decltype(mAnim_Srts) (std::move(rhs.mAnim_Srts));

        onRelocate();
    }
    Collection(const Collection& rhs) {
        new (&mModels) decltype(mModels) (rhs.mModels);
        new (&mTextures) decltype(mTextures) (rhs.mTextures);
        new (&mAnim_Srts) decltype(mAnim_Srts) (rhs.mAnim_Srts);

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

    // INode implementations
    std::size_t numFolders() const override { return 3; }
    const kpi::ICollection* folderAt(std::size_t index) const override {
        switch (index) {
        case 0: return &mModels;
        case 1: return &mTextures;
        case 2: return &mAnim_Srts;
        default: return nullptr;
        }
    }
    kpi::ICollection* folderAt(std::size_t index) override {
        switch (index) {
        case 0: return &mModels;
        case 1: return &mTextures;
        case 2: return &mAnim_Srts;
        default: return nullptr;
        }
    }
    const char* idAt(std::size_t index) const override {
        switch (index) {
        case 0: return typeid(Model).name();
        case 1: return typeid(Texture).name();
        case 2: return typeid(SRT0).name();
        default: return nullptr;
        }
    }
    std::size_t fromId(const char* id) const override {
        if (!strcmp(id, typeid(Model).name())) return 0;
        if (!strcmp(id, typeid(Texture).name())) return 1;
        if (!strcmp(id, typeid(SRT0).name())) return 2;
        return ~0;
    }
    virtual kpi::IDocData* getImmediateData() { return nullptr; }
    virtual const kpi::IDocData* getImmediateData() const { return nullptr; }

public:
    struct _Memento : public kpi::IMemento {
        kpi::ConstPersistentVec<Model> mModels;
        kpi::ConstPersistentVec<Texture> mTextures;
        kpi::ConstPersistentVec<SRT0> mAnim_Srts;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            kpi::nextFolder(this->mModels, _new.getModels(), old ? &old->mModels : nullptr);
            kpi::nextFolder(this->mTextures, _new.getTextures(), old ? &old->mTextures : nullptr);
            kpi::nextFolder(this->mAnim_Srts, _new.getAnim_Srts(), old ? &old->mAnim_Srts : nullptr);
        }
    };
    std::unique_ptr<kpi::IMemento> next(const kpi::IMemento* last) const override {
        return std::make_unique<_Memento>(*this, last);
    }
    void from(const kpi::IMemento& _memento) override {
        auto* in = dynamic_cast<const _Memento*>(&_memento);
        assert(in);
        kpi::fromFolder(getModels(), in->mModels);
        kpi::fromFolder(getTextures(), in->mTextures);
        kpi::fromFolder(getAnim_Srts(), in->mAnim_Srts);
    }
    template<typename T> void* operator=(const T& rhs) { from(rhs); return this; }
};

} // namespace riistudio::g3d
