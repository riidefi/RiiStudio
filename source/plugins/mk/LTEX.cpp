#include <core/kpi/Plugins.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/kpi/RichNameManager.hpp>
#include <core/util/gui.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/egg/LTEX.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <vendor/fa5/IconsFontAwesome5.h>

struct LTEXHeader {
  std::string textureName{};
  librii::egg::BaseLayer baseLayer{};

  std::vector<std::array<u8, 3>> stackJunkEntries;

  bool operator==(const LTEXHeader&) const = default;
};

struct LTEXEntry : public virtual kpi::IObject {
  librii::egg::DrawSetting draw{};
  bool enabled{};
  u32 i;

  bool operator==(const LTEXEntry& rhs) const {
    return draw == rhs.draw && enabled == rhs.enabled && i == rhs.i;
  }
  std::string getName() const { return std::format("DrawSetting ID={}", i); }
};

class LTEX : public kpi::TDocData<LTEXHeader>, public kpi::INode {
public:
  LTEX() = default;
  virtual ~LTEX() = default;

  kpi::MutCollectionRange<LTEXEntry> getEntries() { return {&mEntries}; }
  kpi::ConstCollectionRange<LTEXEntry> getEntries() const {
    return {&mEntries};
  }

protected:
  kpi::ICollection* v_getEntries() const {
    return const_cast<kpi::ICollection*>(
        static_cast<const kpi::ICollection*>(&mEntries));
  }
  void onRelocate() { mEntries.onParentMoved(this); }
  LTEX(LTEX&& rhs) {
    new (&mEntries) decltype(mEntries)(std::move(rhs.mEntries));

    onRelocate();
  }
  LTEX(const LTEX& rhs) {
    new (&mEntries) decltype(mEntries)(rhs.mEntries);

    onRelocate();
  }
  LTEX& operator=(LTEX&& rhs) {
    new (this) LTEX(std::move(rhs));
    onRelocate();
    return *this;
  }
  LTEX& operator=(const LTEX& rhs) {
    new (this) LTEX(rhs);
    onRelocate();
    return *this;
  }

private:
  kpi::CollectionImpl<LTEXEntry> mEntries{this};

  // INode implementations
  std::size_t numFolders() const override { return 1; }
  const kpi::ICollection* folderAt(std::size_t index) const override {
    switch (index) {
    case 0:
      return &mEntries;
    default:
      return nullptr;
    }
  }
  kpi::ICollection* folderAt(std::size_t index) override {
    switch (index) {
    case 0:
      return &mEntries;
    default:
      return nullptr;
    }
  }
  const char* idAt(std::size_t index) const override {
    switch (index) {
    case 0:
      return typeid(LTEXEntry).name();
    default:
      return nullptr;
    }
  }
  std::size_t fromId(const char* id) const override {
    if (!strcmp(id, typeid(LTEXEntry).name()))
      return 0;
    return ~0;
  }
  virtual kpi::IDocData* getImmediateData() {
    return static_cast<kpi::TDocData<LTEXHeader>*>(this);
  }
  virtual const kpi::IDocData* getImmediateData() const {
    return static_cast<const kpi::TDocData<LTEXHeader>*>(this);
  }

public:
  struct _Memento : public kpi::IMemento {
    kpi::ConstPersistentVec<LTEXEntry> mEntries;
    template <typename M>
    _Memento(const M& _new, const kpi::IMemento* last = nullptr) {
      const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
      kpi::nextFolder(this->mEntries, _new.getEntries(),
                      old ? &old->mEntries : nullptr);
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
  }
  template <typename T> void* operator=(const T& rhs) {
    from(rhs);
    return this;
  }
};

struct LMAPHeader {
  bool operator==(const LMAPHeader&) const = default;
};

using LMAPEntry = LTEX;

class LMAP : public kpi::TDocData<LMAPHeader>, public kpi::INode {
public:
  LMAP() = default;
  virtual ~LMAP() = default;

  kpi::MutCollectionRange<LMAPEntry> getEntries() { return {&mEntries}; }
  kpi::ConstCollectionRange<LMAPEntry> getEntries() const {
    return {&mEntries};
  }

protected:
  kpi::ICollection* v_getEntries() const {
    return const_cast<kpi::ICollection*>(
        static_cast<const kpi::ICollection*>(&mEntries));
  }
  void onRelocate() { mEntries.onParentMoved(this); }
  LMAP(LMAP&& rhs) {
    new (&mEntries) decltype(mEntries)(std::move(rhs.mEntries));

    onRelocate();
  }
  LMAP(const LMAP& rhs) {
    new (&mEntries) decltype(mEntries)(rhs.mEntries);

    onRelocate();
  }
  LMAP& operator=(LMAP&& rhs) {
    new (this) LMAP(std::move(rhs));
    onRelocate();
    return *this;
  }
  LMAP& operator=(const LMAP& rhs) {
    new (this) LMAP(rhs);
    onRelocate();
    return *this;
  }

private:
  kpi::CollectionImpl<LMAPEntry> mEntries{this};

  // INode implementations
  std::size_t numFolders() const override { return 1; }
  const kpi::ICollection* folderAt(std::size_t index) const override {
    switch (index) {
    case 0:
      return &mEntries;
    default:
      return nullptr;
    }
  }
  kpi::ICollection* folderAt(std::size_t index) override {
    switch (index) {
    case 0:
      return &mEntries;
    default:
      return nullptr;
    }
  }
  const char* idAt(std::size_t index) const override {
    switch (index) {
    case 0:
      return typeid(LMAPEntry).name();
    default:
      return nullptr;
    }
  }
  std::size_t fromId(const char* id) const override {
    if (!strcmp(id, typeid(LMAPEntry).name()))
      return 0;
    return ~0;
  }
  virtual kpi::IDocData* getImmediateData() {
    return static_cast<kpi::TDocData<LMAPHeader>*>(this);
  }
  virtual const kpi::IDocData* getImmediateData() const {
    return static_cast<const kpi::TDocData<LMAPHeader>*>(this);
  }

public:
  struct _Memento : public kpi::IMemento {
    kpi::ConstPersistentVec<LMAPEntry> mEntries;
    template <typename M>
    _Memento(const M& _new, const kpi::IMemento* last = nullptr) {
      const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
      kpi::nextFolder(this->mEntries, _new.getEntries(),
                      old ? &old->mEntries : nullptr);
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
  }
  template <typename T> void* operator=(const T& rhs) {
    from(rhs);
    return this;
  }
};

class LMAP_IO {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {
    return file.ends_with("blmap") ? typeid(LMAP).name() : "";
  }

  void read(kpi::IOTransaction& transaction) const {
    kpi::IOContext ctx("blmap", transaction);
    LMAP& lmap = static_cast<LMAP&>(transaction.node);
    oishii::BinaryReader reader(std::move(transaction.data));
    reader.setEndian(std::endian::big);
    librii::egg::LightMap bin;
    rsl::SafeReader safe(reader);
    auto ok = bin.read(safe);
    if (!ok) {
      ctx.error(ok.error());
      transaction.state = kpi::TransactionState::Failure;
      return;
    }
    for (auto& e : bin.textures) {
      auto& out = lmap.getEntries().add();
      out.textureName = std::string(e.textureName.begin(), e.textureName.end());
      out.baseLayer = e.baseLayer;

      if (e.drawSettings.size() != 32) {
        ctx.error("Only supports BLMAP with 32 DrawSettings");
        transaction.state = kpi::TransactionState::Failure;
        return;
      }

      librii::egg::DrawSetting defaultEntry{};

      for (size_t i = 0; i < e.drawSettings.size(); ++i) {
        bool enabled = e.activeDrawSettings & (1 << i);
        out.stackJunkEntries.push_back(e.drawSettings[i].stackjunk);
        defaultEntry.stackjunk = e.drawSettings[i].stackjunk;
        if (!enabled && e.drawSettings[i] == defaultEntry)
          continue;
        auto& layer = out.getEntries().add();
        layer.draw = e.drawSettings[i];
        layer.enabled = enabled;
        layer.i = i;
      }
    }
  }
  bool canWrite(kpi::INode& node) const {
    return dynamic_cast<LMAP*>(&node) != nullptr;
  }
  void write(kpi::INode& node, oishii::Writer& writer) const {
    LMAP& lmap = static_cast<LMAP&>(node);
    writer.setEndian(std::endian::big);
    librii::egg::LightMap bin;

    for (const auto& tex : lmap.getEntries()) {
      auto& out = bin.textures.emplace_back();
      std::copy_n(tex.textureName.begin(),
                  std::min(out.textureName.size(), tex.textureName.size()),
                  out.textureName.begin());
      out.baseLayer = tex.baseLayer;
      // Assume always 32 entries
      out.drawSettings.resize(32);
      assert(tex.stackJunkEntries.size() == 32);
      for (size_t i = 0; i < tex.stackJunkEntries.size(); ++i) {
        out.drawSettings[i].stackjunk = tex.stackJunkEntries[i];
      }
      for (auto& e : tex.getEntries()) {
        if (e.enabled) {
          out.activeDrawSettings |= (1 << e.i);
        }
        out.drawSettings[e.i] = e.draw;
      }
    }

    bin.write(writer);
  }
};

kpi::DecentralizedInstaller
    TypeInstaller([](kpi::ApplicationPlugins& installer) {
      installer.addType<LTEXEntry>();
      installer.addType<LTEX>();
      installer.addType<LMAP>();
      kpi::RichNameManager::getInstance().addRichName<LTEXEntry>(
          (const char*)ICON_FA_GRIN_SQUINT, "Draw Setting",
          (const char*)ICON_FA_GRIN_SQUINT_TEARS, "Draw Settings");
      kpi::RichNameManager::getInstance().addRichName<LTEX>(
          (const char*)ICON_FA_LIGHTBULB, "Light Texture",
          (const char*)ICON_FA_LIGHTBULB, "Light Textures");
      kpi::RichNameManager::getInstance().addRichName<LMAP>(
          (const char*)ICON_FA_LIGHTBULB, "Light Map",
          (const char*)ICON_FA_LIGHTBULB, "Light Maps");

      installer.addDeserializer<LMAP_IO>();
      installer.addSerializer<LMAP_IO>();
    });

void Draw(LTEXEntry& cfg) {
  ImGui::Checkbox("Enabled", &cfg.enabled);

  riistudio::util::ConditionalActive g(cfg.enabled);

  int i = cfg.i;
  ImGui::InputInt("Target ID", &i);
  cfg.i = static_cast<u32>(i);
  ImGui::SliderFloat("Effect Scale", &cfg.draw.normEffectScale, 0.0f, 1.0f);
  cfg.draw.pattern = imcxx::EnumCombo("Pattern", cfg.draw.pattern);

#if 0
  // Visualize stack junk as a color
  float clr[3];
  clr[0] = static_cast<float>(cfg.draw.stackjunk[0]) / 255.0f;
  clr[1] = static_cast<float>(cfg.draw.stackjunk[1]) / 255.0f;
  clr[2] = static_cast<float>(cfg.draw.stackjunk[2]) / 255.0f;
  ImGui::ColorPicker3("Color", clr);
#endif
}

void Draw(LTEXHeader& ltex) {
  ltex.baseLayer = imcxx::EnumCombo("Base Layer Type", ltex.baseLayer);

  char buf[33]{};
  snprintf(buf, sizeof(buf), "%s", ltex.textureName.c_str());
  ImGui::InputText("Texture Name", buf, sizeof(buf));
  ltex.textureName = std::string(buf);
}

auto ltex_e = kpi::StatelessPropertyView<LTEXEntry>()
                  .setTitle("Properties")
                  .setIcon((const char*)ICON_FA_COGS)
                  .onDraw([](kpi::PropertyDelegate<LTEXEntry>& delegate) {
                    auto& cfg = delegate.getActive();
                    LTEXEntry tmp = cfg;
                    Draw(tmp);
                    KPI_PROPERTY_EX(delegate, enabled, tmp.enabled);
                    KPI_PROPERTY_EX(delegate, i, tmp.i);
                    KPI_PROPERTY_EX(delegate, draw.normEffectScale,
                                    tmp.draw.normEffectScale);
                    KPI_PROPERTY_EX(delegate, draw.pattern, tmp.draw.pattern);
                  });
auto ltex = kpi::StatelessPropertyView<LTEX>()
                .setTitle("Properties")
                .setIcon((const char*)ICON_FA_COGS)
                .onDraw([](kpi::PropertyDelegate<LTEX>& delegate) {
                  auto& ltex = delegate.getActive();
                  LTEXHeader tmp = ltex;
                  Draw(tmp);
                  KPI_PROPERTY_EX(delegate, baseLayer, tmp.baseLayer);
                  KPI_PROPERTY_EX(delegate, textureName, tmp.textureName);
                });
