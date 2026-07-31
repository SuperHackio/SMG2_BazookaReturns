#pragma once

#include "revolution.h"
#include "Game/Util/JointController.h"

namespace MR {
    // inlined funcs
    template <class T>
    inline JointControlDelegator<T>* createJointDelegatorWithNullChildFunc(T *pHost, bool (T::*calcFunc)(TPos3f *, const JointControllerInfo &), const char *pName) {
        JointControlDelegator<T>* delegator = new JointControlDelegator<T>();
        delegator->mObjPtr = pHost;
        delegator->mCalcJointMtxFunc = calcFunc;
        delegator->mCalcJointMtxAfterChildFunc = 0;
        setJointControllerParam(delegator, pHost, pName);
        return delegator;
    }
    
    template <class T>
    inline JointControlDelegator<T>* createJointDelegatorWithNullMtxFunc(T *pHost, bool (T::*calcFunc)(TPos3f *, const JointControllerInfo &), const char *pName) {
        JointControlDelegator<T>* delegator = new JointControlDelegator<T>(calcFunc, pHost, 0);
        setJointControllerParam(delegator, pHost, pName);
        return delegator;
    }
}