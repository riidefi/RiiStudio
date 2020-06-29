// This is a generated file

namespace libcube {

class Model : public riistudio::lib3d::Model, public kpi::TDocData<libcube::ModelData> {
public:
    Model() = default;
    virtual ~Model() = default;

    kpi::MutCollectionRange<libcube::IGCMaterial> getMaterials() { return { v_getMaterials() }; }
    kpi::MutCollectionRange<libcube::IBoneDelegate> getBones() { return { v_getBones() }; }
    kpi::MutCollectionRange<libcube::IndexedPolygon> getMeshes() { return { v_getMeshes() }; }
    kpi::ConstCollectionRange<libcube::IGCMaterial> getMaterials() const { return { v_getMaterials() }; }
    kpi::ConstCollectionRange<libcube::IBoneDelegate> getBones() const { return { v_getBones() }; }
    kpi::ConstCollectionRange<libcube::IndexedPolygon> getMeshes() const { return { v_getMeshes() }; }

protected:
    virtual kpi::ICollection* v_getMaterials() const = 0;
    virtual kpi::ICollection* v_getBones() const = 0;
    virtual kpi::ICollection* v_getMeshes() const = 0;
};

} // namespace libcube

namespace libcube {

class Scene : public riistudio::lib3d::Scene {
public:
    Scene() = default;
    virtual ~Scene() = default;

    kpi::MutCollectionRange<libcube::Model> getModels() { return { v_getModels() }; }
    kpi::MutCollectionRange<libcube::Texture> getTextures() { return { v_getTextures() }; }
    kpi::ConstCollectionRange<libcube::Model> getModels() const { return { v_getModels() }; }
    kpi::ConstCollectionRange<libcube::Texture> getTextures() const { return { v_getTextures() }; }

protected:
    virtual kpi::ICollection* v_getModels() const = 0;
    virtual kpi::ICollection* v_getTextures() const = 0;
};

} // namespace libcube
