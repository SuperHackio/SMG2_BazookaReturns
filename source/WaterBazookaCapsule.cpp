#include "WaterBazookaCapsule.h"

#include "Game/NameObj/NameObjCategories.h"
#include "Game/Util.h"

namespace NrvWaterBazookaCapsule {
    FULL_NERVE(WaterBazookaCapsuleNrvNormal, WaterBazookaCapsule, Normal);
    FULL_NERVE(WaterBazookaCapsuleNrvCrack, WaterBazookaCapsule, Crack);
    FULL_NERVE(WaterBazookaCapsuleNrvBreak, WaterBazookaCapsule, Break);
};  // namespace NrvWaterBazookaCapsule

WaterBazookaCapsule::WaterBazookaCapsule (LiveActor *pHost, const char *pName) : PartsModel(pHost, pName, "WaterBazookaCapsule", nullptr, MR::DrawBufferType_Enemy, false) {

}

void WaterBazookaCapsule::init (const JMapInfoIter &rIter) {
    initHitSensor(1);
    MR::addHitSensor(this, "body", ATYPE_WATER_BAZOOKA_CAPSULE, 4, 50.0f, TVec3f(0.0f, 0.0f, 0.0f));
    
    MR::initCollisionParts(this, "WaterBazookaCapsule", getSensor(nullptr), nullptr);
    MR::addToAttributeGroupSearchTurtle(this);

    MR::startBrk(this, "Damage");
    MR::setBrkFrameAndStop(this, 0.0f);
    initNerve(&NrvWaterBazookaCapsule::WaterBazookaCapsuleNrvNormal::sInstance, 0);
    makeActorAppeared();
}

void WaterBazookaCapsule::crackCapsule () {
    setNerve(&NrvWaterBazookaCapsule::WaterBazookaCapsuleNrvCrack::sInstance);
}

void WaterBazookaCapsule::breakCapsule () {
    setNerve(&NrvWaterBazookaCapsule::WaterBazookaCapsuleNrvBreak::sInstance);
}

bool WaterBazookaCapsule::isPlayerOnCapsule () const {
    return MR::isOnPlayer(getSensor(nullptr));
}

void WaterBazookaCapsule::exeNormal() {
    if (MR::isFirstStep(this))
        MR::startBva(this, "Normal");
}

void WaterBazookaCapsule::exeCrack () {
    if (MR::isFirstStep(this)) {
        MR::stopScene(12);
        MR::startBva(this, "Crack");
        MR::tryEmitEffect(this, "Crack");
        MR::startSound(this, "SE_EM_WATERBAZ_CAPSULE_DAMAGE", -1, -1);
    }
}

void WaterBazookaCapsule::exeBreak () {
    if (MR::isFirstStep(this)) {
        MR::stopScene(12);
        MR::startBva(this, "Break");
        MR::tryEmitEffect(this, "Break");
        MR::startSound(this, "SE_EM_WATERBAZ_CAPSULE_BREAK", -1, -1);
        MR::invalidateCollisionParts(this);
    }
}

bool WaterBazookaCapsule::receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (isNerve(&NrvWaterBazookaCapsule::WaterBazookaCapsuleNrvBreak::sInstance)) 
        return false;

    if (MR::isEffectValid(this, "Crack")) 
        return false;
    
    return mHost->receiveMsgPlayerAttack(msg, pSender, pReceiver);
}

bool WaterBazookaCapsule::receiveMsgEnemyAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (isNerve(&NrvWaterBazookaCapsule::WaterBazookaCapsuleNrvBreak::sInstance)) 
        return false;

    if (MR::isEffectValid(this, "Crack")) 
        return false;
    
    return mHost->receiveMsgEnemyAttack(msg, pSender, pReceiver);
}

bool WaterBazookaCapsule::receiveOtherMsg(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (isNerve(&NrvWaterBazookaCapsule::WaterBazookaCapsuleNrvBreak::sInstance)) 
        return false;

    if (MR::isEffectValid(this, "Crack")) 
        return false;
    
    return mHost->receiveOtherMsg(msg, pSender, pReceiver);
}