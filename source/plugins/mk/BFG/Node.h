// This is a generated file

namespace riistudio::mk {

class BinaryFog : public mk::NullClass, public kpi::TDocData<mk::BinaryFogData>, public kpi::INode {
public:
    BinaryFog() = default;
    virtual ~BinaryFog() = default;

    kpi::MutCollectionRange<FogEntry> getFogEntries() { return { &mFogEntries }; }
    kpi::ConstCollectionRange<FogEntry> getFogEntries() const { return { &mFogEntries }; }

protected:
    kpi::ICollection* v_getFogEntries() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mFogEntries)); }
    void onRelocate() {
        mFogEntries.onParentMoved(this);
    }
    BinaryFog(BinaryFog&& rhs) {
        new (&mFogEntries) decltype(mFogEntries) (std::move(rhs.mFogEntries));

        onRelocate();
    }
    BinaryFog(const BinaryFog& rhs) {
        new (&mFogEntries) decltype(mFogEntries) (rhs.mFogEntries);

        onRelocate();
    }
    BinaryFog& operator=(BinaryFog&& rhs) {
        new (this) BinaryFog (std::move(rhs));
        onRelocate();
        return *this;
    }
    BinaryFog& operator=(const BinaryFog& rhs) {
        new (this) BinaryFog (rhs);
        onRelocate();
        return *this;
    }

private:
    kpi::CollectionImpl<FogEntry> mFogEntries{this};

    // INode implementations
    std::size_t numFolders() const override { return 1; }
    const kpi::ICollection* folderAt(std::size_t index) const override {
        switch (index) {
        case 0: return &mFogEntries;
        default: return nullptr;
        }
    }
    kpi::ICollection* folderAt(std::size_t index) override {
        switch (index) {
        case 0: return &mFogEntries;
        default: return nullptr;
        }
    }
    const char* idAt(std::size_t index) const override {
        switch (index) {
        case 0: return typeid(FogEntry).name();
        default: return nullptr;
        }
    }
    std::size_t fromId(const char* id) const override {
        if (!strcmp(id, typeid(FogEntry).name())) return 0;
        return ~0;
    }
    virtual kpi::IDocData* getImmediateData() { return static_cast<kpi::TDocData<mk::BinaryFogData>*>(this); }
    virtual const kpi::IDocData* getImmediateData() const { return static_cast<const kpi::TDocData<mk::BinaryFogData>*>(this); }

public:
    struct _Memento : public kpi::IMemento {
        kpi::ConstPersistentVec<FogEntry> mFogEntries;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            kpi::nextFolder(this->mFogEntries, _new.getFogEntries(), old ? &old->mFogEntries : nullptr);
        }
    };
    std::unique_ptr<kpi::IMemento> next(const kpi::IMemento* last) const override {
        return std::make_unique<_Memento>(*this, last);
    }
    void from(const kpi::IMemento& _memento) override {
        auto* in = dynamic_cast<const _Memento*>(&_memento);
        assert(in);
        kpi::fromFolder(getFogEntries(), in->mFogEntries);
    }
    template<typename T> void* operator=(const T& rhs) { from(rhs); return this; }
};

} // namespace riistudio::mk
