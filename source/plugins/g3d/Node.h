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
};

} // namespace riistudio::g3d

namespace riistudio::g3d {

class Collection : public libcube::Scene, public kpi::INode {
public:
    Collection() = default;
    virtual ~Collection() = default;

    kpi::MutCollectionRange<Model> getModels() { return { &mModels }; }
    kpi::MutCollectionRange<Texture> getTextures() { return { &mTextures }; }
    kpi::ConstCollectionRange<Model> getModels() const { return { &mModels }; }
    kpi::ConstCollectionRange<Texture> getTextures() const { return { &mTextures }; }

protected:
    kpi::ICollection* v_getModels() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mModels)); }
    kpi::ICollection* v_getTextures() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mTextures)); }

private:
    kpi::CollectionImpl<Model> mModels{this};
    kpi::CollectionImpl<Texture> mTextures{this};

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
        case 0: return typeid(Model).name();
        case 1: return typeid(Texture).name();
        default: return nullptr;
        }
    }
    std::size_t fromId(const char* id) const override {
        if (!strcmp(id, typeid(Model).name())) return 0;
        if (!strcmp(id, typeid(Texture).name())) return 1;
        return ~0;
    }
    virtual kpi::IDocData* getImmediateData() { return nullptr; }
    virtual const kpi::IDocData* getImmediateData() const { return nullptr; }
};

} // namespace riistudio::g3d
