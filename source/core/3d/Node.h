// This is a generated file

namespace riistudio::lib3d {

class Model {
public:
    Model() = default;
    virtual ~Model() = default;

    kpi::MutCollectionRange<Material> getMaterials() { return { v_getMaterials() }; }
    kpi::MutCollectionRange<Bone> getBones() { return { v_getBones() }; }
    kpi::MutCollectionRange<Polygon> getMeshes() { return { v_getMeshes() }; }
    kpi::ConstCollectionRange<Material> getMaterials() const { return { v_getMaterials() }; }
    kpi::ConstCollectionRange<Bone> getBones() const { return { v_getBones() }; }
    kpi::ConstCollectionRange<Polygon> getMeshes() const { return { v_getMeshes() }; }

protected:
    virtual kpi::ICollection* v_getMaterials() const = 0;
    virtual kpi::ICollection* v_getBones() const = 0;
    virtual kpi::ICollection* v_getMeshes() const = 0;
};

} // namespace riistudio::lib3d

namespace riistudio::lib3d {

class Scene {
public:
    Scene() = default;
    virtual ~Scene() = default;

    kpi::MutCollectionRange<Model> getModels() { return { v_getModels() }; }
    kpi::MutCollectionRange<Texture> getTextures() { return { v_getTextures() }; }
    kpi::ConstCollectionRange<Model> getModels() const { return { v_getModels() }; }
    kpi::ConstCollectionRange<Texture> getTextures() const { return { v_getTextures() }; }

protected:
    virtual kpi::ICollection* v_getModels() const = 0;
    virtual kpi::ICollection* v_getTextures() const = 0;
};

} // namespace riistudio::lib3d
