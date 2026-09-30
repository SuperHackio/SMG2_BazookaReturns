#include "MogucchiShooterBazooka.h"
#include "WaterBazooka.h"

#include "Game/NameObj/NameObjCategories.h"
#include "Game/Util.h"

namespace {
    static const s32 sStepForLaugh = 120;
    static const s32 sStepToAngerBlur = 40;
    static const s32 sAngerBlurTime = 60;
    volatile static const f32 sAngerBlurOffset = 100.0f;
    static const s32 sExplosionBlurTime = 50;
    volatile static const f32 sExplosionBlurOffset = 100.0f;
    static const f32 sTurnRate = 1.0f;
    static const f32 sLimitAngleOfTurn = 30.0f;
};  // namespace

namespace NrvMogucchiShooterBazooka {
    FULL_NERVE(MogucchiShooterNrvWait, MogucchiShooterBazooka, Wait);
    FULL_NERVE(MogucchiShooterNrvShot, MogucchiShooterBazooka, Shot);
    FULL_NERVE(MogucchiShooterNrvTire, MogucchiShooterBazooka, Tire);
    FULL_NERVE(MogucchiShooterNrvDeathPanic, MogucchiShooterBazooka, DeathPanic);
    FULL_NERVE(MogucchiShooterNrvExplosion, MogucchiShooterBazooka, Explosion);
    FULL_NERVE(MogucchiShooterNrvLaugh, MogucchiShooterBazooka, Laugh);
    FULL_NERVE(MogucchiShooterNrvShock, MogucchiShooterBazooka, Shock);
    FULL_NERVE(MogucchiShooterNrvPanic, MogucchiShooterBazooka, Panic);
    FULL_NERVE(MogucchiShooterNrvHitShock, MogucchiShooterBazooka, HitShock);
    FULL_NERVE(MogucchiShooterNrvHitPanic, MogucchiShooterBazooka, HitPanic);
    FULL_NERVE(MogucchiShooterNrvAnger, MogucchiShooterBazooka, Anger);
    FULL_NERVE(MogucchiShooterNrvStormStart, MogucchiShooterBazooka, StormStart);
    FULL_NERVE(MogucchiShooterNrvStorm, MogucchiShooterBazooka, Storm);
};  // namespace NrvMogucchiShooter

MogucchiShooterBazooka::MogucchiShooterBazooka (LiveActor *pHost, const char *pName) : PartsModel(pHost, pName, "MogucchiShooter", nullptr, MR::DrawBufferType_Enemy, false) {
    mFront.zero();
}

void MogucchiShooterBazooka::init (const JMapInfoIter &rIter) {
    initEffectKeeper(1, 0, false);
    MR::addEffectHitNormal(this, 0);
    MR::calcFrontVec(&mFront, this);
    MR::createCenterScreenBlur();
    MR::startBrk(this, "Normal");
    initNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance, 0);
    makeActorAppeared();
}

void MogucchiShooterBazooka::panicDeath () {
    setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvDeathPanic::sInstance);
}

void MogucchiShooterBazooka::explosion () {
    resetDirection();
    setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvExplosion::sInstance);
}

void MogucchiShooterBazooka::hitShock () {
    resetDirection();
    setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvHitShock::sInstance);
}

void MogucchiShooterBazooka::anger () {
    resetDirection();
    setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvAnger::sInstance);
}

void MogucchiShooterBazooka::stormStart () {
    setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvStormStart::sInstance);
}

void MogucchiShooterBazooka::storm () {
    setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvStorm::sInstance);
}

bool MogucchiShooterBazooka::isLaughed () const {
    return isNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvLaugh::sInstance);
}

void MogucchiShooterBazooka::exeWait () {
    WaterBazooka* bazooka = static_cast<WaterBazooka*>(mHost);
    if (MR::isFirstStep(this)) {
        if (bazooka->isBazookaPinch()) {
            MR::startAction(this, "WaitPinch");
            return;
        } else {
            MR::startAction(this, "Wait");
            return;
        }
    }
    faceToMario();

    if (MR::isPlayerInRush()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvLaugh::sInstance);
        return;
    }

    if (bazooka->isPanic()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvShock::sInstance);
        return;
    }

    if (bazooka->isTired()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvTire::sInstance);
        return;
    }

    if (bazooka->isFirstShoot()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvShot::sInstance);
        return;
    }
}

void MogucchiShooterBazooka::exeShot () {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Shot");
        return;
    }

    if (MR::isPlayerInRush()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvLaugh::sInstance);
        return;
    }

    if (MR::isActionEnd(this)) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance);
    }
}

void MogucchiShooterBazooka::exeTire () {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Tire");
    }

    WaterBazooka* bazooka = static_cast<WaterBazooka*>(mHost);

    if (bazooka->isPanic()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvShock::sInstance);
    }
    else if (!bazooka->isTired()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance);
    }
}

void MogucchiShooterBazooka::exeDeathPanic () {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Panic");
        MR::startBrk(this, "CoolDown");
    }
}

void MogucchiShooterBazooka::exeExplosion() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Down");
        MR::startCenterScreenBlur(::sExplosionBlurTime, ::sExplosionBlurOffset, 80, 5, 30);
    }

    if (MR::isActionEnd(this)) {
        kill();
    }
}

void MogucchiShooterBazooka::exeLaugh () {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Laugh");
    }

    if (MR::isStep(this, ::sStepForLaugh)) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance);
    }
}

void MogucchiShooterBazooka::exeShock () {
    if (MR::isFirstStep(this)) 
        MR::startAction(this, "Shock");

    if (MR::isActionEnd(this)) 
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvPanic::sInstance);
}

void MogucchiShooterBazooka::exePanic () {
    WaterBazooka* bazooka = static_cast<WaterBazooka*>(mHost);

    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Panic");

        if (bazooka->isBazookaLifeOut()) {
            MR::setBckRate(this, 2.0f);
        }
    }

    MR::startActionSound(this, "LvPanic", -1, -1, -1);
    faceToMario();

    if (!bazooka->isPanic() && MR::isOnGroundPlayer() && !bazooka->isBazookaLifeOut()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance);
    }
}

void MogucchiShooterBazooka::exeHitShock () {
    if (MR::isFirstStep(this)) 
        MR::startAction(this, "Shock");
    faceToMario();
    if (MR::isActionEnd(this)) 
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvHitPanic::sInstance);
}

void MogucchiShooterBazooka::exeHitPanic () {
    if (MR::isFirstStep(this)) 
        MR::startAction(this, "Panic");
    MR::startActionSound(this, "LvPanic", -1, -1, -1);
    if (!MR::isDemoActive()) 
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance);
}

void MogucchiShooterBazooka::exeAnger () {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Angry");
    }

    if (MR::isStep(this, ::sStepToAngerBlur)) {
        MR::startCenterScreenBlur(::sAngerBlurTime, ::sAngerBlurOffset, 80, 5, 30);
    }

    faceToMario();

    if (!MR::isDemoActive()) {
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance);
    }
}

void MogucchiShooterBazooka::exeStormStart () {
    if (MR::isFirstStep(this))
        MR::startAction(this, "SpinAttackStart");
    faceToMario();
}

void MogucchiShooterBazooka::exeStorm () {
    if (MR::isFirstStep(this)) 
        MR::startAction(this, "SpinAttack");
    faceToMario();
    if (MR::isActionEnd(this)) 
        setNerve(&NrvMogucchiShooterBazooka::MogucchiShooterNrvWait::sInstance);
}

void MogucchiShooterBazooka::calcAndSetBaseMtx () {
    if (!mCalcOwnMatrix) {
        PartsModel::calcAndSetBaseMtx();
        return;
    }

    TPos3f posMtx;
    TPos3f copyMtx;
    posMtx.identity();
    copyMtx.setInline(mFixedPos->mMtx);

    TVec3f up;
    copyMtx.getYDir(up);

    mFixedPos->mMtx.getTransInline(mTranslation);
    MR::makeMtxUpFrontPos(&posMtx, up, mFront, mTranslation);
    MR::setBaseTRMtx(this, posMtx);
}

void MogucchiShooterBazooka::resetDirection () {
    TPos3f mtx(MR::getJointMtx(mHost, "Cockpit"));
    mtx.getZDir(mFront);
}

// Matching
void MogucchiShooterBazooka::faceToMario () {
    if (MR::isPlayerHidden() || MR::isStageStateScenarioOpeningCamera()  || MR::isDemoActive())
        return;

    TPos3f mtx;
    mtx.setInline(mFixedPos->mMtx);
    MR::turnDirectionToTargetDegree(this, &mFront, *MR::getPlayerPos(), ::sTurnRate);
    TVec3f front;
    mtx.getZDir(front);
    MR::clampVecAngleDeg(&mFront, front, ::sLimitAngleOfTurn);
}