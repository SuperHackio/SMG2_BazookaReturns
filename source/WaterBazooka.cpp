#include "WaterBazooka.h"
#include "WaterBazookaCapsule.h"
#include "MogucchiShooterBazooka.h"
#include "FireBazookaFireBall.h"

#include "Game/MapObj/ElectricPressureBullet.h"
#include "Game/MapObj/FirePressureBulletHolder.h"
#include "Game/MapObj/FirePressureBullet.h"
#include "Game/MapObj/WaterPressureBulletHolder.h"
#include "Game/MapObj/WaterPressureBullet.h"
#include "Game/Player/MarioActor.h"
#include "Game/Scene/SceneObjHolder.h"
#include "Game/Util.h"
#include "JSystem/JMATrigonometric.h"

#include "Game/Camera/CameraTargetDemoActor.h"
#include "PinchBgm.h"

namespace {
    static const f32 sElectricBulletSpeed = 12.0f;
    const char* const sDropPointStringTable[] = {
        "落下点1",
        "落下点2",
        "落下点3",
        "落下点4"
    };
};

namespace NrvWaterBazooka {
    FULL_NERVE(WaterBazookaNrvWaitForBattle, WaterBazooka, WaitForBattle);
    FULL_NERVE(WaterBazookaNrvWait, WaterBazooka, Wait);
    FULL_NERVE(WaterBazookaNrvAim, WaterBazooka, Aim);
    FULL_NERVE(WaterBazookaNrvAimEnd, WaterBazooka, AimEnd);
    FULL_NERVE(WaterBazookaNrvShot, WaterBazooka, Shot);
    FULL_NERVE(WaterBazookaNrvShotNoMotion, WaterBazooka, ShotNoMotion);
    FULL_NERVE(WaterBazookaNrvTire, WaterBazooka, Tire);
    FULL_NERVE(WaterBazookaNrvDemoCrackCapsule, WaterBazooka, DemoCrackCapsule);
    FULL_NERVE(WaterBazookaNrvDemoAnger, WaterBazooka, DemoAnger);
    FULL_NERVE(WaterBazookaNrvDemoBreakWait, WaterBazooka, DemoBreakWait);
    FULL_NERVE(WaterBazookaNrvDemoBreakSign, WaterBazooka, DemoBreakSign);
    FULL_NERVE(WaterBazookaNrvDemoBreakExplosion, WaterBazooka, DemoBreakExplosion);
    FULL_NERVE(WaterBazookaNrvDemoBreakEnd, WaterBazooka, DemoBreakEnd);
    FULL_NERVE(WaterBazookaNrvWaitForLaugh, WaterBazooka, WaitForLaugh);
    FULL_NERVE(WaterBazookaNrvPanic, WaterBazooka, Panic);
    FULL_NERVE_ONEND(WaterBazookaNrvStorm, WaterBazooka, Storm, Storm);
};  // namespace NrvWaterBazooka

#define PINCH_BGM mIsPinchBGMStarted ? "BGM_PINCH_02" : "BGM_PINCH_01"

namespace MR {
    // not matching
    HitSensor *addMessageSensorEnemy(LiveActor *pActor, const char *pSensorName) {
        return pActor->mSensorKeeper->add(pSensorName, ATYPE_ENEMY, 0, 0.0f, pActor, TVec3f(0.0f, 0.0f, 0.0f));
    }
}

extern u32 __ptmf_null;


WaterBazooka::WaterBazooka(const char* pName)
    : LiveActor(pName), mShooter(nullptr), mCapsule(nullptr), mBreakModel(nullptr), mDemoActor(nullptr), mCameraInfo(nullptr), mAlreadyDoneFlag(-1),
    mLife(3), mShotNum(0), mCannonCollisionParts(nullptr), mJointCtrl(nullptr), mIsPinchBGMStarted(false), mHasPowerStar(-1), mIsElectric(false), mIsFire(false),
    mBullets(nullptr), mElectricTime(-1), mUsePinchBGM(false), mUseRiddleSfx(false) {
    mBaseMtx.identity();
}

void WaterBazooka::init (const JMapInfoIter &rIter) {
    MR::processInitFunction(this, rIter, false);

    const char* pObjectName;
    MR::getObjectName(&pObjectName, rIter);
    mIsElectric = MR::isEqualSubString(pObjectName, "Electric");
    mIsFire = MR::isEqualSubString(pObjectName, "Fire");

    
    initHitSensor(2);
    MR::addBodyMessageSensorMapObj(this);
    MR::addMessageSensorEnemy(this, "cannon");
    MR::initCollisionParts(this, "Cannon1", getSensor("cannon"), MR::getJointMtx(this, "Cannon1"));
    mCannonCollisionParts = MR::createCollisionPartsFromLiveActor(this, "AllRoot", getSensor("Body"), MR::CollisionScaleType_Default);
    MR::validateCollisionParts(mCannonCollisionParts);

    MR::addEffectHitNormal(this, 0);
    MR::invalidateClipping(this);
    MR::initActorCamera(this, rIter, &mCameraInfo);
    JointControlDelegator<WaterBazooka> *v0 = MR::createJointDelegatorWithNullChildFunc(this, &WaterBazooka::calcJointCannon, "Cannon1");
    mJointCtrl = v0;
    MR::initJointTransform(this);
    JMath::gekko_ps_copy12(mBaseMtx.mMtx, MR::getJointMtx(this, "Cannon1"));
    initShooter();
    initBazookaCapsule();
    initBreakModel(pObjectName);
    initCameraTarget();
    initBullet(rIter);

    MR::getJMapInfoArg0NoInit(rIter, &mHasPowerStar);
    if (mHasPowerStar > 0)
        MR::declarePowerStar(this, mHasPowerStar);

    if (MR::tryRegisterDemoCast(this, rIter)) {
        MR::tryRegisterDemoCast(mShooter, rIter);
        MR::tryRegisterDemoCast(mCapsule, rIter);
        MR::tryRegisterDemoCast(mDemoActor, rIter);
        MR::tryRegisterDemoCast(mBreakModel, rIter);
    }
    MR::startBrk(this, "Damage");
    MR::setBrkFrameAndStop(this, 0.0f);
    MR::declareStarPiece(this, 24);
    MR::addToAttributeGroupSearchTurtle(this);

    initNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance, 0);

    bool useAlreadyDoneFlag = false;
    MR::getJMapInfoArg1NoInit(rIter, &useAlreadyDoneFlag);
    if (useAlreadyDoneFlag && MR::setupAlreadyDoneFlag(&mAlreadyDoneFlag, "SwingStop", rIter, false)) {
        if (MR::isValidSwitchDead(this))
            MR::onSwitchDead(this);
        makeActorDead();
    }
    else
        makeActorAppeared();

    MR::getJMapInfoArg2NoInit(rIter, &mUsePinchBGM);
    MR::getJMapInfoArg3NoInit(rIter, &mUseRiddleSfx);
}

bool WaterBazooka::receiveMsgPlayerAttack (u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (MR::isMsgStarPieceReflect(msg)) 
        return true;

    if (pReceiver == getSensor("Body")) 
        return false;

    if (MR::isDemoActive() || isNrvDemo())
        return false;

    if (MR::isMsgPlayerHipDropFloor(msg) && pReceiver->mSensorType == ATYPE_WATER_BAZOOKA_CAPSULE) {
        if (isElectricLeak()) {
            if (!MR::isPlayerDamaging() || MR::isPlayerParalyzing()) 
                return false;
        }
        damageBazooka(pSender, pReceiver);
        return true;
    } else {
        if (MR::isMsgJetTurtleAttack(msg)) {
            damageBazooka(pSender, pReceiver);
            return true;
        }
    }
    return false;
}

bool WaterBazooka::receiveMsgEnemyAttack (u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (pReceiver == getSensor("Body")) 
        return false;

    if (MR::isDemoActive() || isNrvDemo())
        return false;

    if (pSender->mSensorType == ATYPE_COCO_NUT) {
        damageBazooka(pSender, pReceiver);
        return true;
    }
    return false;
}

bool WaterBazooka::receiveOtherMsg (u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (!MR::isSensorPlayer(pSender)) {
        return false;
    }

    if (pReceiver == getSensor("Body")) {
        return false;
    }

    if (MR::isDemoActive() || isNrvDemo()) {
        return false;
    }

    if (MR::isMsgFloorTouch(msg) || MR::isMsgWallTouch(msg)) {
        if (isElectricLeak() && (!MR::isPlayerHipDropFalling() && !MR::isPlayerHipDropLand())) {
            MR::sendMsgEnemyAttackElectric(pSender, pReceiver);
            return true;
        }

        if (isNerve(&NrvWaterBazooka::WaterBazookaNrvStorm::sInstance)) {
            MR::sendMsgEnemyAttackFlip(pSender, pReceiver);
            return true;
        }

        return false;
    }

    return false;
}

bool WaterBazooka::isFirstShoot () const {
    if (MR::isFirstStep(this)) {
        return isNerve(&NrvWaterBazooka::WaterBazookaNrvShot::sInstance);
    }
    return false;
}

bool WaterBazooka::isTired () const {
    return isNerve(&NrvWaterBazooka::WaterBazookaNrvTire::sInstance);
}

bool WaterBazooka::isPanic () const {
    return isNerve(&NrvWaterBazooka::WaterBazookaNrvPanic::sInstance) || isNerve(&NrvWaterBazooka::WaterBazookaNrvStorm::sInstance);
}

bool WaterBazooka::isBazookaPinch () const {
    return mLife <= 1;
}

bool WaterBazooka::isBazookaLifeOut () const {
    return mLife <= 0;
}

void WaterBazooka::exeWaitForBattle () {
    if (MR::isFirstStep(this)) {
        MR::tryStartBck(this, "Wait", nullptr);
    }

    mBaseMtx.set(MR::getJointMtx(this, "Cannon1"));

    if (!MR::isValidSwitchA(this) || MR::isOnSwitchA(this) || MR::isStageStateScenarioOpeningCamera()) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance);
    }
}

void WaterBazooka::exeWait () {
    if (MR::isFirstStep(this) && isBazookaPinch() && !MR::isBrkPlaying(this, "PowerUp")) {
        MR::startBrk(this, "PowerUp");
    }

    if (mShooter->isLaughed()) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWaitForLaugh::sInstance);
        return;
    }

    if (MR::isGreaterStep(this, 0) && !MR::isPlayerHidden()) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvAim::sInstance);
        return;
    }

    if (tryWaitForBattle()) {
    }
}

void WaterBazooka::exeAim () {
    if (MR::isFirstStep(this)) {
        MR::tryStartBck(this, "ShotStart", nullptr);
    }

    MR::startActionSound(this, "LvTurn", -1, -1, -1);

    if (mIsElectric && MR::isGreaterEqualStep(this, 10)) {
        MR::startActionSound(this, "LvElecPreShoot", -1, -1, -1);
    }

    if (aimAtMario()) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvAimEnd::sInstance);
        return;
    }

    if (MR::isGreaterEqualStep(this, 60)) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvAimEnd::sInstance);
    }
}

void WaterBazooka::exeAimEnd () {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "SwingStop", nullptr);
        MR::startActionSound(this, "TurnEnd", -1, -1, -1);
    }

    if (mIsElectric) {
        MR::startActionSound(this, "LvElecPreShoot", -1, -1, -1);
    }

    if (MR::isBckStopped(this)) {
        if (isBazookaPinch() && !mIsElectric) {
            setNerve(&NrvWaterBazooka::WaterBazookaNrvShotNoMotion::sInstance);
        }
        else {
            setNerve(&NrvWaterBazooka::WaterBazookaNrvShot::sInstance);
        }
    }
}

void WaterBazooka::exeShot () {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Shot", nullptr);
        mShotNum++;
    }

    if (mIsElectric) {
        MR::startActionSound(this, "LvElecPreShoot", -1, -1, -1);
    }

    if (MR::isStep(this, 30)) {
        MR::startActionSound(this, (!mIsElectric && !mIsFire) ? "ShotWater" : "ShotElecFire", -1, -1, -1);
        tryShotBullet();
    }

    if (MR::isBckStopped(this)) {
        if (mShotNum >= 3) {
            mShotNum = 0;
            setNerve(&NrvWaterBazooka::WaterBazookaNrvTire::sInstance);
            return;
        }

        if (isBazookaPinch() && !mIsElectric) {
            setNerve(&NrvWaterBazooka::WaterBazookaNrvShotNoMotion::sInstance);
        }
        else {
            setNerve(&NrvWaterBazooka::WaterBazookaNrvWaitForLaugh::sInstance);
        }
    }
}

void WaterBazooka::exeShotNoMotion () {
    if (MR::isFirstStep(this)) {
        mShotNum++;
        MR::startBck(this, "ShortShot", nullptr);
        MR::startActionSound(this, (!mIsElectric && !mIsFire) ? "ShotWater" : "ShotElecFire", -1, -1, -1);
        tryShotBullet();
    }

    aimAtMario();

    if (mShooter->isLaughed()) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWaitForLaugh::sInstance);
        return;
    }

    if (MR::isStep(this, 30)) {
        if (mShotNum >= 5) {
            mShotNum = 0;
            setNerve(&NrvWaterBazooka::WaterBazookaNrvTire::sInstance);
            return;
        }

        setNerve(&NrvWaterBazooka::WaterBazookaNrvShotNoMotion::sInstance);
    }
}

void WaterBazooka::exeTire () {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Tire", nullptr);
    }

    if (mShooter->isLaughed()) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWaitForLaugh::sInstance);
        return;
    }

    if (MR::isStep(this, 120)) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance);
    }
}

void WaterBazooka::exeDemoCrackCapsule () {
    if (MR::isFirstStep(this)) {
        tryJumpBackPlayerFromBazooka();
        MR::startBck(this, "1stDamage", nullptr);
        mShooter->hitShock();
    }

    if (MR::isStep(this, 27)) {
        MR::startActionSound(this, "MoguAngry1", -1, -1, -1);
    }

    if (!MR::isDemoActiveRegistered(this)) {
        TVec3f starPieceSpawnPos;
        TVec3f up;
        MR::calcUpVec(&up, this);
        JMAVECScaleAdd(up, mTranslation, starPieceSpawnPos, 600.0f);
        MR::appearStarPiece(this, starPieceSpawnPos, 8, 25.0f, 40.0f, false);
        //MR::startSound(this, "SE_OJ_STAR_PIECE_BURST");
        MR::validateCollisionParts(mCapsule);
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance);
    }
}

void WaterBazooka::exeDemoAnger () {
    if (MR::isFirstStep(this)) {
        tryJumpBackPlayerFromBazooka();
        MR::startBck(this, "2ndDamage", nullptr);
        mShooter->anger();
        startBrk("PowerUp");
    }

    if (MR::isStep(this, 20)) {
        MR::startActionSound(this, "MoguAngry2", -1, -1, -1);
    }

    if (mIsElectric) {
        if (MR::isFirstStep(this)) {
            MR::emitEffect(this, "AngrySign");
        }
        else if (MR::isStep(this, 60)) {
            MR::deleteEffect(this, "AngrySign");
        }
        else if (MR::isStep(this, 90)) {
            MR::emitEffect(this, "Angry");
        }

        if (MR::isLessStep(this, 60)) {
            MR::startActionSound(this, "LvElecPreLeak", -1, -1, -1);
        }
        else if (MR::isGreaterEqualStep(this, 90)) {
            MR::startActionSound(this, "LvElecLeak", -1, -1, -1);
        }
    }

    if (!MR::isDemoActiveRegistered(this)) {
        if (mIsElectric) {
            MR::deleteEffect(this, "Angry");
            mElectricTime = 0;
        }

        TVec3f starPieceSpawnPos;
        TVec3f up;
        MR::calcUpVec(&up, this);
        JMAVECScaleAdd(up, mTranslation, starPieceSpawnPos, 600.0f);
        MR::appearStarPiece(this, starPieceSpawnPos, 16, 25.0f, 40.0f, false);
        MR::validateCollisionParts(mCapsule);
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance);
    }
}

void WaterBazooka::exeDemoBreakWait () {
    if (MR::isFirstStep(this)) {
        MR::offUpdateCollisionParts(this);
        if (MR::isPlayerInRush()) {
            MR::endBindAndPlayerJumpWithRollLanding(this, *MR::getPlayerVelocity(), 0);
        }
    }

    if (MR::isStep(this, 5)) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakSign::sInstance);
    }
}

void WaterBazooka::exeDemoBreakSign () {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Down", nullptr);
        tryJumpBackPlayerFromBazooka();
        if (mIsElectric) {
            MR::deleteEffect(this, "Angry");
            MR::deleteEffect(this, "AngrySign");
        }
    }

    MR::tryRumblePadWeak(this, 0);
    MR::startActionSound(this, "LvPreExplode", -1, -1, -1);
    if (MR::isGreaterEqualStep(this, 120)) {
        MR::startActionSound(this, "LvBreakFlash", -1, -1, -1);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakExplosion::sInstance);
    }
}

void WaterBazooka::exeDemoBreakExplosion () {
    if (MR::isFirstStep(this)) {
        MR::hideModel(this);
        MR::invalidateCollisionParts(this);
        MR::invalidateCollisionParts(mCannonCollisionParts);
        MR::invalidateHitSensors(this);
        MR::deleteEffect(this, "DamageSmokeS1");
        MR::deleteEffect(this, "DamageSmokeS2");
        MR::deleteEffect(this, "DamageSmokeL1");
        MR::deleteEffect(this, "DamageSmokeL2");
        mShooter->mCheckHiddenHost = true;
        mShooter->explosion();
        mCapsule->kill();
        mBreakModel->makeActorAppeared();
        MR::invalidateClipping(mBreakModel);
        MR::startBck(mBreakModel, "Break", nullptr);
        MR::startBrk(mBreakModel, "Break");
        MR::tryRumblePadStrong(this, 0);
        MR::shakeCameraNormal();
        MR::startActionSound(this, "Explode", -1, -1, -1);
        MR::startActionSound(this, "MoguDead", -1, -1, -1);
    }

    if (MR::isStep(this, 53)) {
        if (mHasPowerStar > 0) {
            TVec3f spawnPos;
            MR::copyJointPos(this, "Cockpit", &spawnPos);
            //MR::startAfterBossBGM();
            MR::requestAppearPowerStar(this, mHasPowerStar, spawnPos);
        }
        if (mUseRiddleSfx){
            MR::startSystemSE("SE_SY_READ_RIDDLE_S", -1, -1);
        }
    }

    if (MR::isBckStopped(mBreakModel)) {
        mBreakModel->kill();
        setNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakEnd::sInstance);
    }
}

void WaterBazooka::exeDemoBreakEnd () {
    if (MR::isStep(this, 60)) {
        if (MR::isValidSwitchDead(this)) {
            MR::onSwitchDead(this);
        }
        kill();
    }
}

void WaterBazooka::exeWaitForLaugh () {
    if (mShooter->isLaughed()) {
        mShotNum = 0;
    }
    else {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance);
    }
}

void WaterBazooka::exePanic () {
    if (MR::isFirstStep(this)) {
        MR::tryStartBck(this, "Wait", nullptr);
        mShotNum = 0;
    }

    aimAtMario();

    bool playerOn = false;
    if (MR::isOnPlayer(getSensor("cannon")) || mCapsule->isPlayerOnCapsule()) {
        playerOn = true;
    }

    if (!playerOn && MR::isOnGroundPlayer()) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance);
        return;
    }

    if (MR::isStep(this, 180)) {
        mShooter->stormStart();
        startBrk("Spin");
        MR::startActionSound(this, "MoguPreStorm", -1, -1, -1);
    }

    if (MR::isGreaterEqualStep(this, 180)) {
        MR::startActionSound(this, "LvPreStorm", -1, -1, -1);
    }

    if (MR::isStep(this, 300)) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvStorm::sInstance);
    }
}

void WaterBazooka::exeStorm () {
    if (MR::isFirstStep(this)) {
        mShooter->storm();
        MR::emitEffect(this, "Spin");
    }

    TVec3f pos;
    TVec3f side;
    mBaseMtx.getTrans(pos);
    mBaseMtx.getXDir(side);

    f32 deg = MR::calcNerveEaseOutValue(this, 150, 25.0f, 0.0f);
    MR::rotateVecDegree(&side, mGravity, deg);
    TVec3f up;
    JGeometry::negateInternal(&mGravity.x, &up.x);
    MR::makeMtxSideUpPos(&mBaseMtx, side, up, pos);

    MR::startActionSound(this, "LvStorm", (deg * 100.0f) / 25.0f, -1, -1);

    if (MR::isStep(this, 90)) {
        MR::deleteEffect(this, "Spin");
    }

    if (MR::isStep(this, 150)) {
        setNerve(&NrvWaterBazooka::WaterBazookaNrvWait::sInstance);
    }
}

void WaterBazooka::exeOnEndStorm() {
    if (MR::isEffectValid(this, "Spin")) {
        MR::deleteEffect(this, "Spin");
    }
}

void WaterBazooka::makeActorDead () {
    mShooter->makeActorDead();
    mCapsule->makeActorDead();
    mBreakModel->makeActorDead();
    mDemoActor->makeActorDead();
    mCannonCollisionParts->_CC = false;
    LiveActor::makeActorDead();
}

void WaterBazooka::kill () {
    mDemoActor->kill();
    if (mAlreadyDoneFlag >= 0) 
        MR::onAlreadyDoneFlag(mAlreadyDoneFlag);
    LiveActor::kill();
}

void WaterBazooka::control () {
    setCameraTargetMtx();
    switchShowOrHide();
    if (!MR::isStageStateScenarioOpeningCamera() && mUsePinchBGM) {
        if (MR::isExecScenarioStarter()) {
            //MR::playLevelMarioPinchBGM(_E8);
            PinchBGM::startPinchBGM(PINCH_BGM, mIsPinchBGMStarted);
            mIsPinchBGMStarted = true;
        } else if (MR::isNearPlayer(this, 5000.0f)) {
            //MR::playLevelMarioPinchBGM(_E8);
            PinchBGM::startPinchBGM(PINCH_BGM, mIsPinchBGMStarted);
            mIsPinchBGMStarted = true;
        }
    }
    updateElectricLeak();
    if (getSmokeLevel() != -1) 
        MR::startActionSound(this, "LvSmoke", -1, -1, -1);
    tryPanic();
}

void WaterBazooka::calcAndSetBaseMtx () {
    LiveActor::calcAndSetBaseMtx();
    mJointCtrl->registerCallBack();
}

void WaterBazooka::startDemoCrackCapsule () {
    MR::requestStartDemoRegisteredMarioPuppetable(this, nullptr, nullptr, "カプセル破壊");
    mCapsule->crackCapsule();
    MR::invalidateCollisionParts(mCapsule);
    setNerve(&NrvWaterBazooka::WaterBazookaNrvDemoCrackCapsule::sInstance);
}

void WaterBazooka::startDemoAnger () {
    if (MR::isDemoPartExist(this, "乗組員怒り")) 
        MR::requestStartDemoRegisteredMarioPuppetable(this, 0, 0, "乗組員怒り");
    mCapsule->crackCapsule();
    MR::invalidateCollisionParts(mCapsule);
    setNerve(&NrvWaterBazooka::WaterBazookaNrvDemoAnger::sInstance);
}

void WaterBazooka::startDemoBreakCapsule () {
    MR::requestStartDemoRegisteredMarioPuppetable(this, 0, 0, "完全破壊");
    mShooter->panicDeath();
    mCapsule->breakCapsule();
    MR::invalidateCollisionParts(mCapsule);
    setNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakWait::sInstance);
}

bool WaterBazooka::aimAtMario () {
    if (MR::isStageStateScenarioOpeningCamera()) {
        return true;
    }

    TVec3f side;
    TVec3f cannonPos;
    mBaseMtx.getXDir(side);
    mBaseMtx.getTrans(cannonPos);

    TVec3f aimPos;
    JMAVECScaleAdd((-mGravity).toVecPtr(), MR::getPlayerPos()->toVecPtr(), aimPos.toVecPtr(), 100.0f);

    TVec3f aim;
    aim.sub(aimPos, cannonPos);
    MR::normalize(&aim);
    MR::turnVecToVecCos(&side, side, aim, MR::cosDegree(1.2f), mGravity, 0.02f);

    TVec3f v1;
    MR::turnVecToPlane(&v1, side, -mGravity);
    MR::clampVecAngleDeg(&side, v1, 15.0f);

    if (side.dot(-mGravity) > 0.0f) {
        MR::turnVecToPlane(&side, side, -mGravity);
    }

    MR::makeMtxSideUpPos(&mBaseMtx, side, -mGravity, cannonPos);

    TVec3f side2;
    mBaseMtx.getXDir(side2);
    f32 angle = JMath::sAtanTable.atan2_(side2.cross(aim).length(), side2.dot(aim));
    return __fabsf(angle) * 57.29578f <= 2.0f;
}

void WaterBazooka::switchShowOrHide() {
    if (isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakExplosion::sInstance) ||
        isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakEnd::sInstance)) {
        MR::hideModel(this);
        return;
    }

    if (MR::isJudgedToClipFrustum(mTranslation, 800.0f)) {
        if (!MR::isHiddenModel(this)) {
            MR::hideModel(this);
        }
    }
    else {
        if (MR::isHiddenModel(this)) {
            MR::showModel(this);
        }
    }
}

void WaterBazooka::updateElectricLeak () {
    if (mIsElectric && mElectricTime >= 0 && !isNrvDemo() && isBazookaPinch()) {
        if (isElectricLeakSign() && !MR::isEffectValid(this, "AngrySign")) {
            MR::emitEffect(this, "AngrySign");
            MR::deleteEffect(this, "Angry");
        }
        else if (isElectricLeak() && !MR::isEffectValid(this, "Angry")) {
            MR::emitEffect(this, "Angry");
            MR::deleteEffect(this, "AngrySign");
        }

        if (isElectricLeakSign()) {
            MR::startActionSound(this, "LvElecPreLeak", -1, -1, -1);
        }
        else if (isElectricLeak()) {
            MR::startActionSound(this, "LvElecLeak", -1, -1, -1);
        }

        if (mElectricTime < 600) {
            mElectricTime++;
        }
        else {
            mElectricTime = 0;
            MR::deleteEffect(this, "Angry");
        }
    }
}

void WaterBazooka::damageBazooka (HitSensor *pSender, HitSensor *pReceiver) {
    startBrk("Damage");
    MR::emitEffectHitBetweenSensors(this, pSender, pReceiver, 0.0f, nullptr);

    if (mLife == 3) {
        startDemoCrackCapsule();
        MR::emitEffect(this, "DamageSmokeS1");
    }
    else if (mLife == 2) {
        startDemoAnger();
        MR::emitEffect(this, "DamageSmokeS2");
        MR::emitEffect(this, "DamageSmokeL1");

        if (MR::isValidSwitchB(this))
            MR::onSwitchB(this);
    }
    else if (mLife == 1) {
        startDemoBreakCapsule();
        MR::emitEffect(this, "DamageSmokeL2");
    }

    mLife--;
}

bool WaterBazooka::calcJointCannon(TPos3f *pMtx, const JointControllerInfo &v1) {
    pMtx->setInline(mBaseMtx);
    return true;
}

s32 WaterBazooka::getSmokeLevel () const {
    if (mLife == 4) {
        return 0;
    }

    if (mLife == 3) {
        return 1;
    }

    if (mLife == 2) {
        return 2;
    }

    if (mLife == 1) {
        return 3;
    }

    return -1;
}

bool WaterBazooka::tryWaitForBattle () {
    if (MR::isStageStateScenarioOpeningCamera()) 
        return false;
    if (!MR::isValidSwitchA(this)) 
        return false;
    if (MR::isValidSwitchA(this) && MR::isOnSwitchA(this)) 
        return false;
    setNerve(&NrvWaterBazooka::WaterBazookaNrvWaitForBattle::sInstance);
    return true;
}

bool WaterBazooka::tryPanic () {
    if (isNrvDemo()) {
        return false;
    }

    bool b1 = false;
    if (isNerve(&NrvWaterBazooka::WaterBazookaNrvPanic::sInstance) || isNerve(&NrvWaterBazooka::WaterBazookaNrvStorm::sInstance)) {
        b1 = true;
    }

    if (b1) {
        return false;
    }

    bool playerOn = false;
    if (MR::isOnPlayer(getSensor("cannon")) || mCapsule->isPlayerOnCapsule()) {
        playerOn = true;
    }

    if (!playerOn) {
        return false;
    }

    setNerve(&NrvWaterBazooka::WaterBazookaNrvPanic::sInstance);
    return true;
}

bool WaterBazooka::isNrvDemo () const {
    return isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoCrackCapsule::sInstance) || isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoAnger::sInstance) ||
        isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakWait::sInstance) || isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakSign::sInstance) ||
        isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakExplosion::sInstance) ||
        isNerve(&NrvWaterBazooka::WaterBazookaNrvDemoBreakEnd::sInstance);
}

bool WaterBazooka::isElectricLeakSign () const {
    if (!mIsElectric) {
        return false;
    }

    return mElectricTime >= 300 && mElectricTime < 420;
}

bool WaterBazooka::isElectricLeak () const {
    if (!mIsElectric) {
        return false;
    }

    return mElectricTime >= 420 && mElectricTime < 600;
}

void WaterBazooka::initShooter () {
    mShooter = new MogucchiShooterBazooka(this, "ウォーターバズーカ乗組員モグッチ");
    mShooter->initFixedPosition("Cockpit");
    mShooter->initWithoutIter();
}

void WaterBazooka::initBazookaCapsule () {
    mCapsule = new WaterBazookaCapsule(this, "ウォーターバズーカのカプセル");
    mCapsule->initFixedPosition("Top");
    mCapsule->initWithoutIter();
}

void WaterBazooka::initBreakModel (const char* pName) {
    char buf[0x60];
    snprintf(buf, 0x60, "%sBreak", pName);
#if DEBUG
    OSReport("WaterBazooka::initBreakModel -> %s\n", buf);
#endif
    mBreakModel = MR::createModelObjEnemy("ウォーターバズーカ壊れモデル", buf, getBaseMtx());
    mBreakModel->initWithoutIter();
    MR::invalidateClipping(mBreakModel);
    mBreakModel->makeActorDead();
}

void WaterBazooka::initCameraTarget () {
    mDemoActor = new CameraTargetDemoActor(MR::getJointMtx(this, "Cockpit"), "バズーカカメラターゲット");
    mDemoActor->initWithoutIter();
    setCameraTargetMtx();
}

void WaterBazooka::initBullet (const JMapInfoIter &rIter) {
    if (mIsElectric) {
        mBullets = new ElectricPressureBullet*[5];
        for (s32 i = 0; i < 5; i++) {
            mBullets[i] = new ElectricPressureBullet("エレクトリックバズーカの弾");
            mBullets[i]->initWithoutIter();
            MR::tryRegisterDemoCast(mBullets[i], rIter);
        }
    }
    else if (mIsFire) {
        MR::createSceneObj(SCENE_OBJ_FIRE_PRESSURE_BULLET_HOLDER);
    }
    else 
        MR::createSceneObj(SCENE_OBJ_WATER_PRESSURE_BULLET_HOLDER);
}

bool WaterBazooka::tryShotBullet () {
    LiveActor *pBullet = 0;
    if (mIsElectric) 
        pBullet = selectBulletElectric();
    else if (mIsFire) {
        FirePressureBulletHolder* pHolder = (FirePressureBulletHolder*)MR::getSceneObjHolder()->getObj(SCENE_OBJ_FIRE_PRESSURE_BULLET_HOLDER);
        pBullet = pHolder->callEmptyBullet();
    }
    else {
        WaterPressureBulletHolder *pHolder = (WaterPressureBulletHolder *)MR::getSceneObjHolder()->getObj(SCENE_OBJ_WATER_PRESSURE_BULLET_HOLDER);
        pBullet = pHolder->callEmptyBullet();
    }
    if (!pBullet) 
        return false;

    TPos3f v0;
    calcGunPointFromCannon(&v0);
    if (mIsElectric) {
        ElectricPressureBullet *pElectricBullet = (ElectricPressureBullet *)pBullet;
        // false or true???
        pElectricBullet->shotElectricBullet(this, v0, ::sElectricBulletSpeed, false);
    }
    else if (mIsFire) {
        FirePressureBullet* pFireBullet = (FirePressureBullet*)pBullet;
        pFireBullet->shotFireBullet(this, v0, 20.0f, true, false);
        MR::setAltFly(pFireBullet);
    }
    else {
        WaterPressureBullet *pWaterBullet = (WaterPressureBullet *)pBullet;
        pWaterBullet->shotWaterBullet(this, v0, 40.0f, true, true, false, &mCameraInfo);
    }
    return true;
}

ElectricPressureBullet *WaterBazooka::selectBulletElectric () {
    for (s32 idx = 0; idx < 5; idx++) {
        if (MR::isDead(mBullets[idx])) {
            return mBullets[idx];
        }
    }
    return nullptr;
}

bool WaterBazooka::tryJumpBackPlayerFromBazooka () const {
    bool playerOn = false;
    if (MR::isOnPlayer(getSensor("cannon")) || mCapsule->isPlayerOnCapsule()) {
        playerOn = true;
    }

    if (!playerOn && !mCapsule->isPlayerOnCapsule()) {
        return false;
    }

    TVec3f dropPoint;
    calcNearDropPoint(&dropPoint);
    TVec3f jumpDir;
    jumpDir.sub(dropPoint, *MR::getPlayerPos());
    const TVec3f& grav = mGravity;
    TVec3f rej;
    rej.rejection(jumpDir, grav);
    TVec3f rej2(rej);
    rej2.negate();

    MR::offBind(MR::getPlayerDemoActor());
    MR::unlockPlayerAnimation();
    MR::forceFlyPlayer(dropPoint, rej2, MR::calcDistanceToPlayer(dropPoint) / 20.0f);
    return true;
}

void WaterBazooka::calcNearDropPoint (TVec3f *v0) const {
    f32 v3 = 3.4028235e38f;
    TVec3f v2;
    for (s32 i = 0; i < 4; i++) {
        TVec3f v1;
        if (MR::tryFindNamePos(sDropPointStringTable[i], &v1, 0) && MR::isNearPlayer(v1, v3)) {
            v2.set(v1);
            v3 = MR::calcDistanceToPlayer(v2);
        }
    }
    if (v3 != 3.4028235e38f) 
        v0->set(v2);
    else {
        TVec3f v4;
        v4.sub(*MR::getPlayerPos(), mTranslation);
        JMAVECScaleAdd(mGravity, v4, v4, -mGravity.dot(v4));
        MR::normalizeOrZero(&v4);
        if (MR::isNearZero(v4, 0.001f)) {
            MR::getPlayerFrontVec(&v4);
            JGeometry::negateInternal(v4, v4);
        }
        JMAVECScaleAdd(v4, mTranslation, *v0, 800.0f);
    }
}

void WaterBazooka::calcGunPointFromCannon (TPos3f * pMtx) {
    TPos3f mtx(MR::getJointMtx(this, "Cannon1"));
    TVec3f side;
    mtx.getXDir(side);
    TVec3f pos;
    mtx.getTrans(pos);
    JMAVECScaleAdd(side, pos, pos, 550.0f);
    pMtx->setInline(mtx);
    TVec3f up;
    mtx.getYDir(up);
    TVec3f front;
    mtx.getZDir(front);
    MR::makeMtxFrontUpPos(pMtx, side, up, pos);
}

void WaterBazooka::setCameraTargetMtx () {
    TPos3f mtx(mShooter->getBaseMtx());

    mtx.setTrans(mTranslation);
    TVec3f up;
    MR::calcUpVec(&up, this);
    TVec3f front;
    mtx.getZDir(front);
    MR::makeMtxUpFrontPos(&mtx, up, front, mTranslation);
    mDemoActor->setTargetMtx(mtx);
}