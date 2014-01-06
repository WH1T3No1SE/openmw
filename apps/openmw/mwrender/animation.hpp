#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <OgreController.h>
#include <OgreVector3.h>

#include <components/nifogre/ogrenifloader.hpp>

#include "../mwworld/ptr.hpp"


namespace MWRender
{
class Camera;

class Animation
{
public:
    enum Group {
        Group_LowerBody = 1<<0,

        Group_Torso = 1<<1,
        Group_LeftArm = 1<<2,
        Group_RightArm = 1<<3,

        Group_UpperBody = Group_Torso | Group_LeftArm | Group_RightArm,

        Group_All = Group_LowerBody | Group_UpperBody
    };

protected:
    /* This is the number of *discrete* groups. */
    static const size_t sNumGroups = 4;

    class AnimationValue : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Animation *mAnimation;
        std::string mAnimationName;

    public:
        AnimationValue(Animation *anim)
          : mAnimation(anim)
        { }

        void setAnimName(const std::string &name)
        { mAnimationName = name; }
        const std::string &getAnimName() const
        { return mAnimationName; }

        virtual Ogre::Real getValue() const;
        virtual void setValue(Ogre::Real value);
    };

    class EffectAnimationValue : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        float mTime;
    public:
        EffectAnimationValue() : mTime(0) {  }
        void addTime(float time) { mTime += time; }
        void resetTime(float value) { mTime = value; }

        virtual Ogre::Real getValue() const;
        virtual void setValue(Ogre::Real value);
    };



    class NullAnimationValue : public Ogre::ControllerValue<Ogre::Real>
    {
    public:
        virtual Ogre::Real getValue() const
        { return 0.0f; }
        virtual void setValue(Ogre::Real value)
        { }
    };


    struct AnimSource : public Ogre::AnimationAlloc {
        NifOgre::TextKeyMap mTextKeys;
        std::vector<Ogre::Controller<Ogre::Real> > mControllers[sNumGroups];
    };
    typedef std::vector< Ogre::SharedPtr<AnimSource> > AnimSourceList;

    struct AnimState {
        Ogre::SharedPtr<AnimSource> mSource;
        float mStartTime;
        float mLoopStartTime;
        float mLoopStopTime;
        float mStopTime;

        float mTime;
        float mSpeedMult;

        bool mPlaying;
        size_t mLoopCount;

        int mPriority;
        int mGroups;
        bool mAutoDisable;

        AnimState() : mStartTime(0.0f), mLoopStartTime(0.0f), mLoopStopTime(0.0f), mStopTime(0.0f),
                      mTime(0.0f), mSpeedMult(1.0f), mPlaying(false), mLoopCount(0),
                      mPriority(0), mGroups(0), mAutoDisable(true)
        { }
    };
    typedef std::map<std::string,AnimState> AnimStateMap;

    typedef std::map<Ogre::MovableObject*,std::string> ObjectAttachMap;

    struct EffectParams
    {
        std::string mModelName; // Just here so we don't add the same effect twice
        NifOgre::ObjectScenePtr mObjects;
        int mEffectId;
        bool mLoop;
        std::string mBoneName;
    };

    std::vector<EffectParams> mEffects;

    MWWorld::Ptr mPtr;
    Camera *mCamera;

    Ogre::SceneNode *mInsert;
    Ogre::Entity *mSkelBase;
    NifOgre::ObjectScenePtr mObjectRoot;
    AnimSourceList mAnimSources;
    Ogre::Node *mAccumRoot;
    bool mAccumRootPosUpd;
    Ogre::Node *mNonAccumRoot;
    NifOgre::NodeTargetValue<Ogre::Real> *mNonAccumCtrl;
    Ogre::Vector3 mAccumulate;

    AnimStateMap mStates;

    Ogre::SharedPtr<AnimationValue> mAnimationValuePtr[sNumGroups];
    Ogre::SharedPtr<NullAnimationValue> mNullAnimationValuePtr;

    ObjectAttachMap mAttachedObjects;


    /* Sets the appropriate animations on the bone groups based on priority.
     */
    void resetActiveGroups();

    static size_t detectAnimGroup(const Ogre::Node *node);

    static float calcAnimVelocity(const NifOgre::TextKeyMap &keys,
                                  NifOgre::NodeTargetValue<Ogre::Real> *nonaccumctrl,
                                  const Ogre::Vector3 &accum,
                                  const std::string &groupname);

    /* Updates a skeleton instance so that all bones matching the source skeleton (based on
     * bone names) are positioned identically. */
    void updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel);

    /* Updates the position of the accum root node for the given time, and
     * returns the wanted movement vector from the previous time. */
    void updatePosition(float oldtime, float newtime, Ogre::Vector3 &position);

    static NifOgre::TextKeyMap::const_iterator findGroupStart(const NifOgre::TextKeyMap &keys, const std::string &groupname);

    /* Resets the animation to the time of the specified start marker, without
     * moving anything, and set the end time to the specified stop marker. If
     * the marker is not found, or if the markers are the same, it returns
     * false.
     */
    bool reset(AnimState &state, const NifOgre::TextKeyMap &keys,
               const std::string &groupname, const std::string &start, const std::string &stop,
               float startpoint);

    void handleTextKey(AnimState &state, const std::string &groupname, const NifOgre::TextKeyMap::const_iterator &key);

    /* Sets the root model of the object. If 'baseonly' is true, then any meshes or particle
     * systems in the model are ignored (useful for NPCs, where only the skeleton is needed for
     * the root).
     *
     * Note that you must make sure all animation sources are cleared before reseting the object
     * root. All nodes previously retrieved with getNode will also become invalidated.
     */
    void setObjectRoot(const std::string &model, bool baseonly);

    /* Adds the keyframe controllers in the specified model as a new animation source. Note that
     * the filename portion of the provided model name will be prepended with 'x', and the .nif
     * extension will be replaced with .kf. */
    void addAnimSource(const std::string &model);

    /** Adds an additional light to the given object list using the specified ESM record. */
    void addExtraLight(Ogre::SceneManager *sceneMgr, NifOgre::ObjectScenePtr objlist, const ESM::Light *light);

    static void setRenderProperties(NifOgre::ObjectScenePtr objlist, Ogre::uint32 visflags, Ogre::uint8 solidqueue,
                                    Ogre::uint8 transqueue, Ogre::Real dist=0.0f,
                                    bool enchantedGlow=false, Ogre::Vector3* glowColor=NULL);

    void clearAnimSources();

    // TODO: Should not be here
    Ogre::Vector3 getEnchantmentColor(MWWorld::Ptr item);

public:
    Animation(const MWWorld::Ptr &ptr, Ogre::SceneNode *node);
    virtual ~Animation();

    /**
     * @brief Add an effect mesh attached to a bone or the insert scene node
     * @param model
     * @param effectId An ID for this effect. Note that adding the same ID again won't add another effect.
     * @param loop Loop the effect. If false, it is removed automatically after it finishes playing. If true,
     *              you need to remove it manually using removeEffect when the effect should end.
     * @param bonename Bone to attach to, or empty string to use the scene node instead
     * @param texture override the texture specified in the model's materials
     * @note Will not add an effect twice.
     */
    void addEffect (const std::string& model, int effectId, bool loop = false, const std::string& bonename = "", std::string texture = "");
    void removeEffect (int effectId);
    void getLoopingEffects (std::vector<int>& out);

    /// Prepare this animation for being rendered with \a camera (rotates billboard nodes)
    virtual void preRender (Ogre::Camera* camera);

    virtual void setAlpha(float alpha) {}
private:
    void updateEffects(float duration);


public:
    void updatePtr(const MWWorld::Ptr &ptr);

    bool hasAnimation(const std::string &anim);

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const Ogre::Vector3 &accum);

    /** Plays an animation.
     * \param groupname Name of the animation group to play.
     * \param priority Priority of the animation. The animation will play on
     *                 bone groups that don't have another animation set of a
     *                 higher priority.
     * \param groups Bone groups to play the animation on.
     * \param autodisable Automatically disable the animation when it stops
     *                    playing.
     * \param speedmult Speed multiplier for the animation.
     * \param start Key marker from which to start.
     * \param stop Key marker to stop at.
     * \param startpoint How far in between the two markers to start. 0 starts
     *                   at the start marker, 1 starts at the stop marker.
     * \param loops How many times to loop the animation. This will use the
     *              "loop start" and "loop stop" markers if they exist,
     *              otherwise it will use "start" and "stop".
     */
    void play(const std::string &groupname, int priority, int groups, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops);

    /** Returns true if the named animation group is playing. */
    bool isPlaying(const std::string &groupname) const;

    bool isPlaying(Group group) const;

    /** Gets info about the given animation group.
     * \param groupname Animation group to check.
     * \param complete Stores completion amount (0 = at start key, 0.5 = half way between start and stop keys), etc.
     * \param speedmult Stores the animation speed multiplier
     * \return True if the animation is active, false otherwise.
     */
    bool getInfo(const std::string &groupname, float *complete=NULL, float *speedmult=NULL) const;

    /** Disables the specified animation group;
     * \param groupname Animation group to disable.
     */
    void disable(const std::string &groupname);
    void changeGroups(const std::string &groupname, int group);

    /** Retrieves the velocity (in units per second) that the animation will move. */
    float getVelocity(const std::string &groupname) const;

    virtual Ogre::Vector3 runAnimation(float duration);

    virtual void showWeapons(bool showWeapon);
    virtual void showCarriedLeft(bool show) {}

    void enableLights(bool enable);

    Ogre::AxisAlignedBox getWorldBounds();

    void setCamera(Camera *cam)
    { mCamera = cam; }

    Ogre::Node *getNode(const std::string &name);

    // Attaches the given object to a bone on this object's base skeleton. If the bone doesn't
    // exist, the object isn't attached and NULL is returned. The returned TagPoint is only
    // valid until the next setObjectRoot call.
    Ogre::TagPoint *attachObjectToBone(const Ogre::String &bonename, Ogre::MovableObject *obj);
    void detachObjectFromBone(Ogre::MovableObject *obj);
};

class ObjectAnimation : public Animation {
public:
    ObjectAnimation(const MWWorld::Ptr& ptr, const std::string &model);

    void addLight(const ESM::Light *light);

    bool canBatch() const;
    void fillBatch(Ogre::StaticGeometry *sg);
};

}
#endif
