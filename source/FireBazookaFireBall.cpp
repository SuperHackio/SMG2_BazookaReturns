#include "FireBazookaFireBall.h"

#include "Game/LiveActor/LiveActor.h"
#include "Game/Enemy/WalkerStateBindStarPointer.h"
#include "Game/MapObj/FirePressureBullet.h"
#include "Game/Util.h"

#include "Kamek/hooks.h"

namespace NrvFireBazookaFireBall {
	class FireBazookaFireBallNrvFly : public Nerve
	{
	public:
		virtual void execute(Spine* pSpine) const {
            FirePressureBullet* actor = reinterpret_cast<FirePressureBullet*>(pSpine->mExecutor);
			{
                if (MR::isFirstStep(actor)) {
                    MR::startAction(actor, "Shot");
                    MR::startBtk(actor, "FireBullet");
                }

                if (MR::isBckOneTimeAndStopped(actor)) {
                    MR::startAction(actor, "Move");
                }

                MR::startActionSound(actor, "OjLvFPressureBullet", -1, -1, -1);

                if (actor->_AC && !(actor->getNerveStep() % 5)) {
                    TVec3f v3;
                    TVec3f playercenter(*MR::getPlayerCenterPos());
                    playercenter.add(*MR::getPlayerGravity() * -30.0f);
                    v3.sub(playercenter, actor->mTranslation);
                    MR::normalize(&v3);
                    MR::turnVecToVecDegree(&actor->_98, actor->_98, v3, 4.0f, TVec3f(0, 1, 0));
                    actor->mVelocity.scale(actor->_A8, actor->_98);
                }

                if (actor->_94->tryStartPointBind()) {
                    actor->setNerve(&NrvFirePressureBullet::FirePressureBulletNrvSupportFreeze::sInstance);
                    return;
                }

                if (actor->isCrash()) {
                    actor->kill();
                }
                else if (MR::isStep(actor, 300)) {
                    actor->kill();
                }
			}
		};
		static FireBazookaFireBallNrvFly sInstance;
	};
    FireBazookaFireBallNrvFly FireBazookaFireBallNrvFly::sInstance;

    class FireBazookaFireBallNrvKill : public Nerve
    {
    public:
        virtual void execute(Spine* pSpine) const {
            FirePressureBullet* actor = reinterpret_cast<FirePressureBullet*>(pSpine->mExecutor);
            {
                if (MR::isFirstStep(actor))
                    MR::startAction(actor, "Break");

                actor->mVelocity.zero();

                if (MR::isActionEnd(actor))
                    actor->LiveActor::kill();

            }
        };
        static FireBazookaFireBallNrvKill sInstance;
    };
    FireBazookaFireBallNrvKill FireBazookaFireBallNrvKill::sInstance;
};  // namespace NrvFireBazookaFireBall


namespace MR {
    void setAltFly(register FirePressureBullet* pBullet) {
        MR::requestMovementOn(pBullet);
        pBullet->setNerve(&NrvFireBazookaFireBall::FireBazookaFireBallNrvFly::sInstance);
        __asm {
            li r3, 1
            stb r3, 0xAE(pBullet)
        }
    }
}


namespace {
    void clearAndSetNerve(register FirePressureBullet* pBullet, const Nerve* pNerve) {
        pBullet->setNerve(pNerve);
        __asm {
            li r3, 0
            stb r3, 0xAE(pBullet)
        }
    }
    void newSupportFreeze(register FirePressureBullet* pBullet, const Nerve* pNerve) {
        register bool isAltFly;
        __asm {
            lbz isAltFly, 0xAE(pBullet)
        }
        if (isAltFly)
            pNerve = &NrvFireBazookaFireBall::FireBazookaFireBallNrvFly::sInstance;
        MR::updateActorStateAndNextNerve(pBullet, pNerve);
    }

    void onBreak(FirePressureBullet* pBullet) {
        MR::tryEmitEffect(pBullet, "Break");
        pBullet->LiveActor::kill();
    }
}
kmCall(0x802A1B70, clearAndSetNerve);
kmCall(0x802A1D18, newSupportFreeze);
kmCall(0x802A1A50, onBreak);