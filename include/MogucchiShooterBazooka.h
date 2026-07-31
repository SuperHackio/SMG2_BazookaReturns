#pragma once

#include "Game/LiveActor/PartsModel.h"

class MogucchiShooterBazooka : public PartsModel {
public:
    MogucchiShooterBazooka(LiveActor *pHost, const char *pName);

    virtual void init(const JMapInfoIter &rIter);
    virtual void calcAndSetBaseMtx();

    void panicDeath();
    void explosion();
    void resetDirection();
    void hitShock();
    void anger();
    void stormStart();
    void storm();
    void faceToMario();
    bool isLaughed() const;

    void exeWait();
    void exeShot();
    void exeTire();
    void exeDeathPanic();
    void exeExplosion();
    void exeLaugh();
    void exeShock();
    void exePanic();
    void exeHitShock();
    void exeHitPanic();
    void exeAnger();
    void exeStormStart();
    void exeStorm();

    TVec3f mFront;
};