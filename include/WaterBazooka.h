#pragma once

#include "Game/LiveActor/LiveActor.h"
#include "Game/Util/JointController.h"

#include "Game/Util/ActorAnimUtil.h"
#include "WaterBazookaCapsule.h"

class MogucchiShooterBazooka;
class WaterBazookaCapsule;
class ElectricPressureBullet;
class ModelObj;
class CameraTargetDemoActor;
class ActorCameraInfo;

class WaterBazooka : public LiveActor {
public:
    WaterBazooka(const char *pName);
    virtual void init(const JMapInfoIter &rIter);
    virtual bool receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver);
    virtual bool receiveMsgEnemyAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver);
    virtual bool receiveOtherMsg(u32 msg, HitSensor *pSender, HitSensor *pReceiver);
    virtual void makeActorDead();
    virtual void kill();
    virtual void control();
    virtual void calcAndSetBaseMtx();

    void initShooter();
    void initBazookaCapsule();
    void initBreakModel(const char* pObjectName);
    void initCameraTarget();
    void initBullet(const JMapInfoIter &rIter);
    bool isNrvDemo() const;
    bool isElectricLeak() const;
    bool isElectricLeakSign() const;
    bool isFirstShoot() const;
    bool isTired() const;
    bool isPanic() const;
    bool isBazookaPinch() const;
    bool isBazookaLifeOut() const;
    bool aimAtMario();
    bool tryWaitForBattle();
    bool tryShotBullet();
    bool tryJumpBackPlayerFromBazooka() const;
    bool tryPanic();
    s32 getSmokeLevel() const;
    
    void exeWaitForBattle();
    void exeWait();
    void exeAim();
    void exeAimEnd();
    void exeShot();
    void exeShotNoMotion();
    void exeTire();
    void exeDemoCrackCapsule();
    void exeDemoAnger();
    void exeDemoBreakWait();
    void exeDemoBreakSign();
    void exeDemoBreakExplosion();
    void exeDemoBreakEnd();
    void exeWaitForLaugh();
    void exePanic();
    void exeStorm();
    void exeOnEndStorm();

    bool calcJointCannon(TPos3f *, const JointControllerInfo &);
    void damageBazooka(HitSensor *pSender, HitSensor *pReceiver);
    void setCameraTargetMtx();
    void switchShowOrHide();
    void updateElectricLeak();
    void startDemoCrackCapsule();
    void startDemoAnger();
    void startDemoBreakCapsule();
    ElectricPressureBullet *selectBulletElectric();
    void calcGunPointFromCannon(TPos3f *);
    void calcNearDropPoint(TVec3f *) const;

    inline void startBrk(const char* pName) {
        MR::startBrk(this, pName);
        MR::startBrk(mCapsule, pName);
    }

    MogucchiShooterBazooka* mShooter;
    WaterBazookaCapsule* mCapsule;
    ModelObj* mBreakModel;
    CameraTargetDemoActor* mDemoActor;
    ActorCameraInfo* mCameraInfo;
    int mAlreadyDoneFlag;
    s32 mLife;
    s32 mShotNum;
    CollisionParts* mCannonCollisionParts;
    JointController* mJointCtrl;
    TPos3f mBaseMtx;
    s32 mHasPowerStar;
    bool mUsePinchBGM;
    bool mIsPinchBGMStarted;
    bool mIsElectric;
    bool mIsFire;
    bool mUseRiddleSfx;
    ElectricPressureBullet** mBullets;
    s32 mElectricTime;
};