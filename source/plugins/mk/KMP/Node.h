// This is a generated file

namespace riistudio::mk {

class CourseMap : public mk::NullClass, public kpi::TDocData<mk::CourseMapData>, public kpi::INode {
public:
    CourseMap() = default;
    virtual ~CourseMap() = default;

    kpi::MutCollectionRange<StartPoint> getStartPoints() { return { &mStartPoints }; }
    kpi::MutCollectionRange<EnemyPath> getEnemyPaths() { return { &mEnemyPaths }; }
    kpi::MutCollectionRange<ItemPath> getItemPaths() { return { &mItemPaths }; }
    kpi::MutCollectionRange<CheckPath> getCheckPaths() { return { &mCheckPaths }; }
    kpi::MutCollectionRange<Path> getPaths() { return { &mPaths }; }
    kpi::MutCollectionRange<GeoObj> getGeoObjs() { return { &mGeoObjs }; }
    kpi::MutCollectionRange<Area> getAreas() { return { &mAreas }; }
    kpi::MutCollectionRange<Camera> getCameras() { return { &mCameras }; }
    kpi::MutCollectionRange<RespawnPoint> getRespawnPoints() { return { &mRespawnPoints }; }
    kpi::MutCollectionRange<Cannon> getCannonPoints() { return { &mCannonPoints }; }
    kpi::MutCollectionRange<Stage> getStages() { return { &mStages }; }
    kpi::MutCollectionRange<MissionPoint> getMissionPoints() { return { &mMissionPoints }; }
    kpi::ConstCollectionRange<StartPoint> getStartPoints() const { return { &mStartPoints }; }
    kpi::ConstCollectionRange<EnemyPath> getEnemyPaths() const { return { &mEnemyPaths }; }
    kpi::ConstCollectionRange<ItemPath> getItemPaths() const { return { &mItemPaths }; }
    kpi::ConstCollectionRange<CheckPath> getCheckPaths() const { return { &mCheckPaths }; }
    kpi::ConstCollectionRange<Path> getPaths() const { return { &mPaths }; }
    kpi::ConstCollectionRange<GeoObj> getGeoObjs() const { return { &mGeoObjs }; }
    kpi::ConstCollectionRange<Area> getAreas() const { return { &mAreas }; }
    kpi::ConstCollectionRange<Camera> getCameras() const { return { &mCameras }; }
    kpi::ConstCollectionRange<RespawnPoint> getRespawnPoints() const { return { &mRespawnPoints }; }
    kpi::ConstCollectionRange<Cannon> getCannonPoints() const { return { &mCannonPoints }; }
    kpi::ConstCollectionRange<Stage> getStages() const { return { &mStages }; }
    kpi::ConstCollectionRange<MissionPoint> getMissionPoints() const { return { &mMissionPoints }; }

protected:
    kpi::ICollection* v_getStartPoints() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mStartPoints)); }
    kpi::ICollection* v_getEnemyPaths() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mEnemyPaths)); }
    kpi::ICollection* v_getItemPaths() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mItemPaths)); }
    kpi::ICollection* v_getCheckPaths() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mCheckPaths)); }
    kpi::ICollection* v_getPaths() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mPaths)); }
    kpi::ICollection* v_getGeoObjs() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mGeoObjs)); }
    kpi::ICollection* v_getAreas() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mAreas)); }
    kpi::ICollection* v_getCameras() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mCameras)); }
    kpi::ICollection* v_getRespawnPoints() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mRespawnPoints)); }
    kpi::ICollection* v_getCannonPoints() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mCannonPoints)); }
    kpi::ICollection* v_getStages() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mStages)); }
    kpi::ICollection* v_getMissionPoints() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&mMissionPoints)); }
    void onRelocate() {
        mStartPoints.onParentMoved(this);
        mEnemyPaths.onParentMoved(this);
        mItemPaths.onParentMoved(this);
        mCheckPaths.onParentMoved(this);
        mPaths.onParentMoved(this);
        mGeoObjs.onParentMoved(this);
        mAreas.onParentMoved(this);
        mCameras.onParentMoved(this);
        mRespawnPoints.onParentMoved(this);
        mCannonPoints.onParentMoved(this);
        mStages.onParentMoved(this);
        mMissionPoints.onParentMoved(this);
    }
    CourseMap(CourseMap&& rhs) {
        new (&mStartPoints) decltype(mStartPoints) (std::move(rhs.mStartPoints));
        new (&mEnemyPaths) decltype(mEnemyPaths) (std::move(rhs.mEnemyPaths));
        new (&mItemPaths) decltype(mItemPaths) (std::move(rhs.mItemPaths));
        new (&mCheckPaths) decltype(mCheckPaths) (std::move(rhs.mCheckPaths));
        new (&mPaths) decltype(mPaths) (std::move(rhs.mPaths));
        new (&mGeoObjs) decltype(mGeoObjs) (std::move(rhs.mGeoObjs));
        new (&mAreas) decltype(mAreas) (std::move(rhs.mAreas));
        new (&mCameras) decltype(mCameras) (std::move(rhs.mCameras));
        new (&mRespawnPoints) decltype(mRespawnPoints) (std::move(rhs.mRespawnPoints));
        new (&mCannonPoints) decltype(mCannonPoints) (std::move(rhs.mCannonPoints));
        new (&mStages) decltype(mStages) (std::move(rhs.mStages));
        new (&mMissionPoints) decltype(mMissionPoints) (std::move(rhs.mMissionPoints));

        onRelocate();
    }
    CourseMap(const CourseMap& rhs) {
        new (&mStartPoints) decltype(mStartPoints) (rhs.mStartPoints);
        new (&mEnemyPaths) decltype(mEnemyPaths) (rhs.mEnemyPaths);
        new (&mItemPaths) decltype(mItemPaths) (rhs.mItemPaths);
        new (&mCheckPaths) decltype(mCheckPaths) (rhs.mCheckPaths);
        new (&mPaths) decltype(mPaths) (rhs.mPaths);
        new (&mGeoObjs) decltype(mGeoObjs) (rhs.mGeoObjs);
        new (&mAreas) decltype(mAreas) (rhs.mAreas);
        new (&mCameras) decltype(mCameras) (rhs.mCameras);
        new (&mRespawnPoints) decltype(mRespawnPoints) (rhs.mRespawnPoints);
        new (&mCannonPoints) decltype(mCannonPoints) (rhs.mCannonPoints);
        new (&mStages) decltype(mStages) (rhs.mStages);
        new (&mMissionPoints) decltype(mMissionPoints) (rhs.mMissionPoints);

        onRelocate();
    }

private:
    kpi::CollectionImpl<StartPoint> mStartPoints{this};
    kpi::CollectionImpl<EnemyPath> mEnemyPaths{this};
    kpi::CollectionImpl<ItemPath> mItemPaths{this};
    kpi::CollectionImpl<CheckPath> mCheckPaths{this};
    kpi::CollectionImpl<Path> mPaths{this};
    kpi::CollectionImpl<GeoObj> mGeoObjs{this};
    kpi::CollectionImpl<Area> mAreas{this};
    kpi::CollectionImpl<Camera> mCameras{this};
    kpi::CollectionImpl<RespawnPoint> mRespawnPoints{this};
    kpi::CollectionImpl<Cannon> mCannonPoints{this};
    kpi::CollectionImpl<Stage> mStages{this};
    kpi::CollectionImpl<MissionPoint> mMissionPoints{this};

    // INode implementations
    std::size_t numFolders() const override { return 12; }
    const kpi::ICollection* folderAt(std::size_t index) const override {
        switch (index) {
        case 0: return &mStartPoints;
        case 1: return &mEnemyPaths;
        case 2: return &mItemPaths;
        case 3: return &mCheckPaths;
        case 4: return &mPaths;
        case 5: return &mGeoObjs;
        case 6: return &mAreas;
        case 7: return &mCameras;
        case 8: return &mRespawnPoints;
        case 9: return &mCannonPoints;
        case 10: return &mStages;
        case 11: return &mMissionPoints;
        default: return nullptr;
        }
    }
    kpi::ICollection* folderAt(std::size_t index) override {
        switch (index) {
        case 0: return &mStartPoints;
        case 1: return &mEnemyPaths;
        case 2: return &mItemPaths;
        case 3: return &mCheckPaths;
        case 4: return &mPaths;
        case 5: return &mGeoObjs;
        case 6: return &mAreas;
        case 7: return &mCameras;
        case 8: return &mRespawnPoints;
        case 9: return &mCannonPoints;
        case 10: return &mStages;
        case 11: return &mMissionPoints;
        default: return nullptr;
        }
    }
    const char* idAt(std::size_t index) const override {
        switch (index) {
        case 0: return typeid(StartPoint).name();
        case 1: return typeid(EnemyPath).name();
        case 2: return typeid(ItemPath).name();
        case 3: return typeid(CheckPath).name();
        case 4: return typeid(Path).name();
        case 5: return typeid(GeoObj).name();
        case 6: return typeid(Area).name();
        case 7: return typeid(Camera).name();
        case 8: return typeid(RespawnPoint).name();
        case 9: return typeid(Cannon).name();
        case 10: return typeid(Stage).name();
        case 11: return typeid(MissionPoint).name();
        default: return nullptr;
        }
    }
    std::size_t fromId(const char* id) const override {
        if (!strcmp(id, typeid(StartPoint).name())) return 0;
        if (!strcmp(id, typeid(EnemyPath).name())) return 1;
        if (!strcmp(id, typeid(ItemPath).name())) return 2;
        if (!strcmp(id, typeid(CheckPath).name())) return 3;
        if (!strcmp(id, typeid(Path).name())) return 4;
        if (!strcmp(id, typeid(GeoObj).name())) return 5;
        if (!strcmp(id, typeid(Area).name())) return 6;
        if (!strcmp(id, typeid(Camera).name())) return 7;
        if (!strcmp(id, typeid(RespawnPoint).name())) return 8;
        if (!strcmp(id, typeid(Cannon).name())) return 9;
        if (!strcmp(id, typeid(Stage).name())) return 10;
        if (!strcmp(id, typeid(MissionPoint).name())) return 11;
        return ~0;
    }
    virtual kpi::IDocData* getImmediateData() { return static_cast<kpi::TDocData<mk::CourseMapData>*>(this); }
    virtual const kpi::IDocData* getImmediateData() const { return static_cast<const kpi::TDocData<mk::CourseMapData>*>(this); }

public:
    struct _Memento : public kpi::IMemento {
        kpi::ConstPersistentVec<StartPoint> mStartPoints;
        kpi::ConstPersistentVec<EnemyPath> mEnemyPaths;
        kpi::ConstPersistentVec<ItemPath> mItemPaths;
        kpi::ConstPersistentVec<CheckPath> mCheckPaths;
        kpi::ConstPersistentVec<Path> mPaths;
        kpi::ConstPersistentVec<GeoObj> mGeoObjs;
        kpi::ConstPersistentVec<Area> mAreas;
        kpi::ConstPersistentVec<Camera> mCameras;
        kpi::ConstPersistentVec<RespawnPoint> mRespawnPoints;
        kpi::ConstPersistentVec<Cannon> mCannonPoints;
        kpi::ConstPersistentVec<Stage> mStages;
        kpi::ConstPersistentVec<MissionPoint> mMissionPoints;
        template<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {
            const auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;
            kpi::nextFolder(this->mStartPoints, _new.getStartPoints(), old ? &old->mStartPoints : nullptr);
            kpi::nextFolder(this->mEnemyPaths, _new.getEnemyPaths(), old ? &old->mEnemyPaths : nullptr);
            kpi::nextFolder(this->mItemPaths, _new.getItemPaths(), old ? &old->mItemPaths : nullptr);
            kpi::nextFolder(this->mCheckPaths, _new.getCheckPaths(), old ? &old->mCheckPaths : nullptr);
            kpi::nextFolder(this->mPaths, _new.getPaths(), old ? &old->mPaths : nullptr);
            kpi::nextFolder(this->mGeoObjs, _new.getGeoObjs(), old ? &old->mGeoObjs : nullptr);
            kpi::nextFolder(this->mAreas, _new.getAreas(), old ? &old->mAreas : nullptr);
            kpi::nextFolder(this->mCameras, _new.getCameras(), old ? &old->mCameras : nullptr);
            kpi::nextFolder(this->mRespawnPoints, _new.getRespawnPoints(), old ? &old->mRespawnPoints : nullptr);
            kpi::nextFolder(this->mCannonPoints, _new.getCannonPoints(), old ? &old->mCannonPoints : nullptr);
            kpi::nextFolder(this->mStages, _new.getStages(), old ? &old->mStages : nullptr);
            kpi::nextFolder(this->mMissionPoints, _new.getMissionPoints(), old ? &old->mMissionPoints : nullptr);
        }
    };
    std::unique_ptr<kpi::IMemento> next(const kpi::IMemento* last) const override {
        return std::make_unique<_Memento>(*this, last);
    }
    void from(const kpi::IMemento& _memento) override {
        auto* in = dynamic_cast<const _Memento*>(&_memento);
        assert(in);
        kpi::fromFolder(getStartPoints(), in->mStartPoints);
        kpi::fromFolder(getEnemyPaths(), in->mEnemyPaths);
        kpi::fromFolder(getItemPaths(), in->mItemPaths);
        kpi::fromFolder(getCheckPaths(), in->mCheckPaths);
        kpi::fromFolder(getPaths(), in->mPaths);
        kpi::fromFolder(getGeoObjs(), in->mGeoObjs);
        kpi::fromFolder(getAreas(), in->mAreas);
        kpi::fromFolder(getCameras(), in->mCameras);
        kpi::fromFolder(getRespawnPoints(), in->mRespawnPoints);
        kpi::fromFolder(getCannonPoints(), in->mCannonPoints);
        kpi::fromFolder(getStages(), in->mStages);
        kpi::fromFolder(getMissionPoints(), in->mMissionPoints);
    }
    template<typename T> void* operator=(const T& rhs) { from(rhs); return this; }
};

} // namespace riistudio::mk
