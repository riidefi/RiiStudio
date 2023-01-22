#include <core/kpi/Plugins.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/kpi/RichNameManager.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/egg/Blight.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <vendor/fa5/IconsFontAwesome5.h>

struct LGHTHeader {
  librii::gx::Color backColor;

  bool operator==(const LGHTHeader&) const = default;
};

struct LGHTEntry : public virtual kpi::IObject {
  librii::egg::Light light;
  u32 i = 0;

  bool operator==(const LGHTEntry& rhs) const {
    return light == rhs.light && i == rhs.i;
  }
  std::string getName() const {
    return std::format("{} Light ID={}", magic_enum::enum_name(light.lightType),
                       i);
  }
};

struct Ambient : public virtual kpi::IObject {
  librii::gx::Color color;
  u32 i = 0;

  bool operator==(const Ambient& rhs) const {
    return color == rhs.color && i == rhs.i;
  }
  std::string getName() const { return std::format("Ambient Color ID={}", i); }
};

class LGHT : public kpi::TDocData<LGHTHeader>, public kpi::INode {
public:
  LGHT() = default;
  virtual ~LGHT() = default;

  kpi::MutCollectionRange<LGHTEntry> getEntries() { return {&mEntries}; }
  kpi::ConstCollectionRange<LGHTEntry> getEntries() const {
    return {&mEntries};
  }
  kpi::MutCollectionRange<Ambient> getAmbs() { return {&mAmbs}; }
  kpi::ConstCollectionRange<Ambient> getAmbs() const { return {&mAmbs}; }

protected:
  kpi::ICollection* v_getEntries() const {
    return const_cast<kpi::ICollection*>(
        static_cast<const kpi::ICollection*>(&mEntries));
  }
  void onRelocate() {
    mEntries.onParentMoved(this);
    mAmbs.onParentMoved(this);
  }
  LGHT(LGHT&& rhs) {
    new (&mEntries) decltype(mEntries)(std::move(rhs.mEntries));
    new (&mAmbs) decltype(mAmbs)(std::move(rhs.mAmbs));

    onRelocate();
  }
  LGHT(const LGHT& rhs) {
    new (&mEntries) decltype(mEntries)(rhs.mEntries);
    new (&mAmbs) decltype(mAmbs)(rhs.mAmbs);

    onRelocate();
  }
  LGHT& operator=(LGHT&& rhs) {
    new (this) LGHT(std::move(rhs));
    onRelocate();
    return *this;
  }
  LGHT& operator=(const LGHT& rhs) {
    new (this) LGHT(rhs);
    onRelocate();
    return *this;
  }

private:
  kpi::CollectionImpl<LGHTEntry> mEntries{this};
  kpi::CollectionImpl<Ambient> mAmbs{this};

  // INode implementations
  std::size_t numFolders() const override { return 2; }
  const kpi::ICollection* folderAt(std::size_t index) const override {
    switch (index) {
    case 0:
      return &mEntries;
    case 1:
      return &mAmbs;
    default:
      return nullptr;
    }
  }
  kpi::ICollection* folderAt(std::size_t index) override {
    switch (index) {
    case 0:
      return &mEntries;
    case 1:
      return &mAmbs;
    default:
      return nullptr;
    }
  }
  const char* idAt(std::size_t index) const override {
    switch (index) {
    case 0:
      return typeid(LGHTEntry).name();
    case 1:
      return typeid(Ambient).name();
    default:
      return nullptr;
    }
  }
  std::size_t fromId(const char* id) const override {
    if (!strcmp(id, typeid(LGHTEntry).name()))
      return 0;
    if (!strcmp(id, typeid(Ambient).name()))
      return 1;
    return ~0;
  }
  virtual kpi::IDocData* getImmediateData() {
    return static_cast<kpi::TDocData<LGHTHeader>*>(this);
  }
  virtual const kpi::IDocData* getImmediateData() const {
    return static_cast<const kpi::TDocData<LGHTHeader>*>(this);
  }

public:
  struct _Memento : public kpi::IMemento {
    kpi::ConstPersistentVec<LGHTEntry> mEntries;
    kpi::ConstPersistentVec<Ambient> mAmbs;
    template <typename M>
    _Memento(const M& _new, const kpi::IMemento* last = nullptr) {
      const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
      kpi::nextFolder(this->mEntries, _new.getEntries(),
                      old ? &old->mEntries : nullptr);
      kpi::nextFolder(this->mAmbs, _new.getAmbs(), old ? &old->mAmbs : nullptr);
    }
  };
  std::unique_ptr<kpi::IMemento>
  next(const kpi::IMemento* last) const override {
    return std::make_unique<_Memento>(*this, last);
  }
  void from(const kpi::IMemento& _memento) override {
    auto* in = dynamic_cast<const _Memento*>(&_memento);
    assert(in);
    kpi::fromFolder(getEntries(), in->mEntries);
    kpi::fromFolder(getAmbs(), in->mAmbs);
  }
  template <typename T> void* operator=(const T& rhs) {
    from(rhs);
    return this;
  }
};

class LGHT_IO {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {
    return file.ends_with("blight") ? typeid(LGHT).name() : "";
  }

  void read(kpi::IOTransaction& transaction) const {
    kpi::IOContext ctx("blmap", transaction);
    LGHT& light = static_cast<LGHT&>(transaction.node);
    oishii::BinaryReader reader(std::move(transaction.data));
    reader.setEndian(std::endian::big);
    librii::egg::Blight bin;
    {
      auto ok = bin.read(reader);
      if (!ok) {
        ctx.error(ok.error());
        transaction.state = kpi::TransactionState::Failure;
        return;
      }
    }
    librii::egg::LightSet set;
    auto ok = set.from(bin);
    if (!ok) {
      ctx.error(ok.error());
      transaction.state = kpi::TransactionState::Failure;
      return;
    }
    light.backColor = set.backColor;
    u32 i = 0;
    for (auto& l : set.lights) {
      auto& o = light.getEntries().add();
      o.i = i++;
      o.light = l;
    }
    i = 0;
    for (auto& a : set.ambientColors) {
      auto& o = light.getAmbs().add();
      o.i = i++;
      o.color = a;
    }
  }
  bool canWrite(kpi::INode& node) const {
    return dynamic_cast<LGHT*>(&node) != nullptr;
  }
  Result<void> write(kpi::INode& node, oishii::Writer& writer) const {
    LGHT& light = static_cast<LGHT&>(node);
    writer.setEndian(std::endian::big);
    librii::egg::LightSet set;
    set.backColor = light.backColor;
    for (auto& x : light.getEntries()) {
      set.lights.push_back(x.light);
    }
    for (auto& x : light.getAmbs()) {
      set.ambientColors.push_back(x.color);
    }
    librii::egg::Blight blight;
    set.to(blight);
    blight.write(writer);
    return {};
  }
};

kpi::DecentralizedInstaller
    TypeInstaller([](kpi::ApplicationPlugins& installer) {
      installer.addType<LGHTEntry>();
      installer.addType<Ambient>();
      installer.addType<LGHT>();
      kpi::RichNameManager::getInstance().addRichName<LGHTEntry>(
          (const char*)ICON_FA_LIGHTBULB, "Light",
          (const char*)ICON_FA_LIGHTBULB, "Lights", ImVec4(.7f, .2f, .3f, 1.f));
      kpi::RichNameManager::getInstance().addRichName<Ambient>(
          (const char*)ICON_FA_FILL, "Ambient Color", (const char*)ICON_FA_FILL,
          "Ambient Colors", ImVec4(.5f, .2f, .4f, 1.f));
      kpi::RichNameManager::getInstance().addRichName<LGHT>(
          (const char*)ICON_FA_LIGHTBULB, "Light Set",
          (const char*)ICON_FA_LIGHTBULB, "Light Sets");

      installer.addDeserializer<LGHT_IO>();
      installer.addSerializer<LGHT_IO>();
    });

void Draw(LGHTHeader& cfg_) {
  librii::gx::ColorF32 fc = cfg_.backColor;
  ImGui::ColorEdit4("Back Color", fc);
  cfg_.backColor = fc;
}

void Draw(LGHTEntry& cfg_, auto&& ambs, bool& amb_updated) {
  auto& cfg = cfg_.light;
  ImGui::Checkbox("Enable Calc", &cfg.enable_calc);
  ImGui::SameLine();
  riistudio::util::ConditionalActive g(cfg.enable_calc);
  ImGui::Checkbox("Send to GPU", &cfg.enable_sendToGpu);
  ImGui::SameLine();

  ImGui::Separator();
  cfg.lightType = imcxx::EnumCombo("Type", cfg.lightType);
  if (cfg.hasPosition() || true) {
    ImGui::InputFloat3("Position", glm::value_ptr(cfg.position));
  }
  if (cfg.hasAim()) {
    ImGui::InputFloat3("Aim", glm::value_ptr(cfg.aim));
  }
  bool snap = cfg.snapTargetIndex.has_value();
  ImGui::Checkbox("Snap To Other Light", &snap);
  if (snap) {
    int v = cfg.snapTargetIndex.value_or(0);
    ImGui::InputInt("Other Light Index", &v);
    cfg.snapTargetIndex = v;
  } else {
    cfg.snapTargetIndex.reset();
  }
  ImGui::Separator();

  ImGui::Checkbox("BLMAP", &cfg.blmap);
  ImGui::SameLine();
  ImGui::Checkbox("Affects BRRES materials (Color)", &cfg.brresColor);
  ImGui::SameLine();
  ImGui::Checkbox("Affects BRRES materials (Alpha)", &cfg.brresAlpha);

  ImGui::Separator();
  {
    riistudio::util::ConditionalActive g(cfg.hasDistAttn());
    // if (ImGui::BeginChild("Distance Falloff"))
    {
      ImGui::InputFloat("Reference Distance", &cfg.refDist);
      if (ImGui::SliderFloat("Reference Brightness", &cfg.refBright, 0.0f,
                             1.0f)) {
        if (cfg.refBright >= 1.0f) {
          cfg.refBright = std::nextafterf(1.0f, 0.0f);

        } else if (cfg.refBright <= 0.0f) {
          cfg.refBright = std::nextafterf(0.0f, 1.0f);
        }
      }
      cfg.distAttnFunction = imcxx::EnumCombo("Funtion", cfg.distAttnFunction);
    }
  }
  ImGui::Separator();
  {
    riistudio::util::ConditionalActive g(cfg.hasAngularAttn());
    // if (ImGui::BeginChild("Angular Falloff",ImVec2(0, 0), true))
    {
      if (ImGui::SliderFloat("Spotlight Radius", &cfg.spotCutoffAngle, 0.0f,
                             90.0f)) {
        if (cfg.refBright > 90.0f) {
          cfg.refBright = 90.0f;

        } else if (cfg.refBright <= 0.0f) {
          cfg.refBright = std::nextafterf(0.0f, 90.0f);
        }
      }
      cfg.spotFunction = imcxx::EnumCombo("Spotlight Type", cfg.spotFunction);
    }
  }
  ImGui::Separator();

  {
    librii::gx::ColorF32 fc = cfg.color;
    ImGui::ColorEdit4("Color", fc);
    cfg.color = fc;
  }
  ImGui::InputFloat("Color Scale", &cfg.intensity);
  {
    librii::gx::ColorF32 fc = cfg.specularColor;
    ImGui::ColorEdit4("Specular Color", fc);
    cfg.specularColor = fc;
  }
  {
    int amb = cfg.ambientLightIndex;
    ImGui::InputInt("Ambient Light Index", &amb);
    if (amb < 0 || amb >= ambs.size()) {
      amb = cfg.ambientLightIndex;
    }
    librii::gx::ColorF32 fc = ambs[amb].color;
    ImGui::ColorEdit4("Ambient Color (at that index)", fc,
                      ImGuiColorEditFlags_NoOptions |
                          ImGuiColorEditFlags_DisplayRGB);
    amb_updated |= ambs[amb].color != static_cast<librii::gx::Color>(fc);
    cfg.ambientLightIndex = amb;
  }
  {
    int unk = cfg.miscFlags;
    ImGui::InputInt("Misc Flags", &unk);
    cfg.miscFlags = unk;
  }

  ImGui::Separator();
  ImGui::Checkbox(
      "Override DISTANCE+ANGULAR FALLOFF settings at runtime with SHININESS=16",
      &cfg.runtimeShininess);

  ImGui::Checkbox("Override DISTANCE FALLOFF settings at runtime",
                  &cfg.runtimeDistAttn);
  ImGui::Checkbox("Override ANGULAR FALLOFF settings at runtime",
                  &cfg.runtimeAngularAttn);
  ImGui::Separator();
  cfg.coordSpace = imcxx::EnumCombo("Coordinate Space", cfg.coordSpace);
}

void Draw(Ambient& cfg_) {
  librii::gx::ColorF32 fc = cfg_.color;
  ImGui::ColorEdit4("Color", fc);
  cfg_.color = fc;
}

auto ltex_e = kpi::StatelessPropertyView<LGHTEntry>()
                  .setTitle("Properties")
                  .setIcon((const char*)ICON_FA_COGS)
                  .onDraw([](kpi::PropertyDelegate<LGHTEntry>& delegate) {
                    auto& cfg = delegate.getActive();
                    LGHTEntry tmp = cfg;
                    auto* parent = cfg.childOf;
                    assert(parent);
                    auto* lght = dynamic_cast<LGHT*>(parent);
                    assert(lght);
                    bool amb_changed = false;
                    Draw(tmp, lght->getAmbs(), amb_changed);
                    if (amb_changed) {
                      delegate.commit("Ambient Light Updated");
                    }
#define ENT(x) KPI_PROPERTY_EX(delegate, light.x, tmp.light.x);
                    ENT(enable_calc);
                    ENT(enable_sendToGpu);
                    ENT(lightType);
                    ENT(position);
                    ENT(aim);
                    ENT(snapTargetIndex);
                    ENT(coordSpace);
                    ENT(blmap);
                    ENT(brresColor);
                    ENT(brresAlpha);
                    ENT(runtimeDistAttn);
                    ENT(refDist);
                    ENT(refBright);
                    ENT(distAttnFunction);
                    ENT(runtimeAngularAttn);
                    ENT(spotCutoffAngle);
                    ENT(spotFunction);
                    ENT(runtimeShininess);
                    ENT(miscFlags);
                    ENT(intensity);
                    ENT(color);
                    ENT(specularColor);
                    ENT(ambientLightIndex);
#undef ENT
                  });
auto ltex = kpi::StatelessPropertyView<Ambient>()
                .setTitle("Properties")
                .setIcon((const char*)ICON_FA_COGS)
                .onDraw([](kpi::PropertyDelegate<Ambient>& delegate) {
                  auto& obj = delegate.getActive();
                  Ambient tmp = obj;
                  Draw(tmp);
                  KPI_PROPERTY_EX(delegate, color, tmp.color);
                });
auto lght = kpi::StatelessPropertyView<LGHT>()
                .setTitle("Properties")
                .setIcon((const char*)ICON_FA_COGS)
                .onDraw([](kpi::PropertyDelegate<LGHT>& delegate) {
                  auto& obj = delegate.getActive();
                  LGHTHeader tmp = obj;
                  Draw(tmp);
                  KPI_PROPERTY_EX(delegate, backColor, tmp.backColor);
                });
