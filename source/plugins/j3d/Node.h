// This is a generated file

namespace riistudio::j3d {

class Model : public libcube::Model, public kpi::TDocData<riistudio::j3d::ModelData>, public kpi::INode {
public:
    Model() = default;
    virtual ~Model() = default;

    kpi::MutCollectionRange<riistudio::j3d::Material> getMaterials() { return { &mMaterials }; }
    kpi::MutCollectionRange<riistudio::j3d::Joint> getBones() { return { &mBones }; }
    kpi::MutCollectionRange<riistudio::j3d::Shape> getMeshes() { return { &mMeshes }; }
    kpi::ConstCollectionRange<riistudio::j3d::Material> getMaterials() const { return { &mMaterials }; }
    kpi::ConstCollectionRange<riistudio::j3d::Joint> getBones() const { return { &mBones }; }
    kpi::ConstCollectionRange<riistudio::j3d::Shape> getMeshes() const { return { &mMeshes }; }

protected:
    kpi::ICollection* v_getMaterials() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mMaterials)); }
    kpi::ICollection* v_getBones() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mBones)); }
    kpi::ICollection* v_getMeshes() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mMeshes)); }
    void onRelocate() {
        mMaterials.onParentMoved(this);
        mBones.onParentMoved(this);
        mMeshes.onParentMoved(this);
    }
    Model(Model&& rhs) {
        new (&mMaterials) decltype(mMaterials) (std::move(rhs.mMaterials));
        new (&mBones) decltype(mBones) (std::move(rhs.mBones));
        new (&mMeshes) decltype(mMeshes) (std::move(rhs.mMeshes));

        onRelocate();
    }
    Model(const Model& rhs) {
        new (&mMaterials) decltype(mMaterials) (rhs.mMaterials);
        new (&mBones) decltype(mBones) (rhs.mBones);
        new (&mMeshes) decltype(mMeshes) (rhs.mMeshes);

        onRelocate();
    }

private:
    kpi::CollectionImpl<riistudio::j3d::Material> mMaterials{this};
    kpi::CollectionImpl<riistudio::j3d::Joint> mBones{this};
    kpi::CollectionImpl<riistudio::j3d::Shape> mMeshes{this};

    // INode implementations
    std::size_t numFolders() const override { return 3; }
    const kpi::ICollection* folderAt(std::size_t index) const override {
        switch (index) {
        case 0: return &mMaterials;
        case 1: return &mBones;
        case 2: return &mMeshes;
        default: return nullptr;
        }
    }
    kpi::ICollection* folderAt(std::size_t index) override {
        switch (index) {
        case 0: return &mMaterials;
        case 1: return &mBones;
        case 2: return &mMeshes;
        default: return nullptr;
        }
    }
    const char* idAt(std::size_t index) const override {
        switch (index) {
        case 0: return typeid(riistudio::j3d::Material).name();
        case 1: return typeid(riistudio::j3d::Joint).name();
        case 2: return typeid(riistudio::j3d::Shape).name();
        default: return nullptr;
        }
    }
    std::size_t fromId(const char* id) const override {
        if (!strcmp(id, typeid(riistudio::j3d::Material).name())) return 0;
        if (!strcmp(id, typeid(riistudio::j3d::Joint).name())) return 1;
        if (!strcmp(id, typeid(riistudio::j3d::Shape).name())) return 2;
        return ~0;
    }
    virtual kpi::IDocData* getImmediateData() { return static_cast<kpi::TDocData<riistudio::j3d::ModelData>*>(this); }
    virtual const kpi::IDocData* getImmediateData() const { return static_cast<const kpi::TDocData<riistudio::j3d::ModelData>*>(this); }

public:
    struct _Memento : public kpi::IMemento {
        kpi::ConstPersistentVec<riistudio::j3d::Material> mMaterials;
        kpi::ConstPersistentVec<riistudio::j3d::Joint> mBones;
        kpi::ConstPersistentVec<riistudio::j3d::Shape> mMeshes;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            kpi::nextFolder(this->mMaterials, _new.getMaterials(), old ? &old->mMaterials : nullptr);
            kpi::nextFolder(this->mBones, _new.getBones(), old ? &old->mBones : nullptr);
            kpi::nextFolder(this->mMeshes, _new.getMeshes(), old ? &old->mMeshes : nullptr);
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
    }
    template<typename T> void* operator=(const T& rhs) { from(rhs); return this; }
};

} // namespace riistudio::j3d

namespace riistudio::j3d {

class Collection : public libcube::Scene, public kpi::INode {
public:
    Collection() = default;
    virtual ~Collection() = default;

    kpi::MutCollectionRange<riistudio::j3d::Model> getModels() { return { &mModels }; }
    kpi::MutCollectionRange<riistudio::j3d::Texture> getTextures() { return { &mTextures }; }
    kpi::ConstCollectionRange<riistudio::j3d::Model> getModels() const { return { &mModels }; }
    kpi::ConstCollectionRange<riistudio::j3d::Texture> getTextures() const { return { &mTextures }; }

protected:
    kpi::ICollection* v_getModels() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mModels)); }
    kpi::ICollection* v_getTextures() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mTextures)); }
    void onRelocate() {
        mModels.onParentMoved(this);
        mTextures.onParentMoved(this);
    }
    Collection(Collection&& rhs) {
        new (&mModels) decltype(mModels) (std::move(rhs.mModels));
        new (&mTextures) decltype(mTextures) (std::move(rhs.mTextures));

        onRelocate();
    }
    Collection(const Collection& rhs) {
        new (&mModels) decltype(mModels) (rhs.mModels);
        new (&mTextures) decltype(mTextures) (rhs.mTextures);

        onRelocate();
    }

private:
    kpi::CollectionImpl<riistudio::j3d::Model> mModels{this};
    kpi::CollectionImpl<riistudio::j3d::Texture> mTextures{this};

    // INode implementations
    std::size_t numFolders() const override { return 2; }
    const kpi::ICollection* folderAt(std::size_t index) const override {
        switch (index) {
        case 0: return &mModels;
        case 1: return &mTextures;
        default: return nullptr;
        }
    }
    kpi::ICollection* folderAt(std::size_t index) override {
        switch (index) {
        case 0: return &mModels;
        case 1: return &mTextures;
        default: return nullptr;
        }
    }
    const char* idAt(std::size_t index) const override {
        switch (index) {
        case 0: return typeid(riistudio::j3d::Model).name();
        case 1: return typeid(riistudio::j3d::Texture).name();
        default: return nullptr;
        }
    }
    std::size_t fromId(const char* id) const override {
        if (!strcmp(id, typeid(riistudio::j3d::Model).name())) return 0;
        if (!strcmp(id, typeid(riistudio::j3d::Texture).name())) return 1;
        return ~0;
    }
    virtual kpi::IDocData* getImmediateData() { return nullptr; }
    virtual const kpi::IDocData* getImmediateData() const { return nullptr; }

public:
    struct _Memento : public kpi::IMemento {
        kpi::ConstPersistentVec<riistudio::j3d::Model> mModels;
        kpi::ConstPersistentVec<riistudio::j3d::Texture> mTextures;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            kpi::nextFolder(this->mModels, _new.getModels(), old ? &old->mModels : nullptr);
            kpi::nextFolder(this->mTextures, _new.getTextures(), old ? &old->mTextures : nullptr);
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
    }
    template<typename T> void* operator=(const T& rhs) { from(rhs); return this; }
};

} // namespace riistudio::j3d
