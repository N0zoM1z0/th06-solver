#include "Chain.hpp"
#include "stdafx.h"
#include "th06/resources.hpp"
#include <stdexcept>

// Chain, RNG and timer implementations imported from N0zoM1z0/th06 9e79eb8.
namespace th06 {
Chain g_FuncChain;
Supervisor g_Supervisor;
SoundPlayer g_SoundPlayer;
Rng g_Rng;
ZunMemory g_ZunMemoryMgr;
GameErrorContext g_GameErrorContext;
u16 g_PreviousFrameKeys, g_CurFrameInput, g_IsEigthFrameInput, g_CountFramesInputsWereHeld;
long g_ReadFileSize;
Chain::~Chain() {}

ChainElem::ChainElem() {
    prev = NULL;
    next = NULL;
    callback = NULL;
    unkPtr = this;
    addedCallback = NULL;
    deletedCallback = NULL;
    priority = 0;

    isHeapAllocated = false;
}

ChainElem::~ChainElem() {
    if (deletedCallback != NULL) {
        this->deletedCallback(this->arg);
    }

    prev = NULL;
    next = NULL;
    callback = NULL;
    addedCallback = NULL;
    deletedCallback = NULL;
}

Chain::Chain() {
    midiOutputDeviceCount = 0;
    unk = 0;
}

ZunResult Chain::AddToCalcChain(ChainElem *elem, int priority) {
    ChainElem *cur = &this->calcChain;
#if BUILT_SINCE(102g)
    DebugPrint("add calc chain (pri = %d)\n", priority);
#endif
    elem->priority = priority;

    while (cur->next != NULL) {
        if (cur->priority > priority) {
            break;
        }

        cur = cur->next;
    }

    if (cur->priority > priority) {
        elem->next = cur;
        elem->prev = cur->prev;

        if (elem->prev != NULL) {
            elem->prev->next = elem;
        }

        cur->prev = elem;
    } else {
        elem->next = NULL;
        elem->prev = cur;
        cur->next = elem;
    }

    if (elem->addedCallback != NULL) {
        ZunResult res = elem->addedCallback(elem->arg);
        elem->addedCallback = NULL;

        return res;
    } else {
        return ZUN_SUCCESS;
    }
}

ZunResult Chain::AddToDrawChain(ChainElem *elem, int priority) {
    ChainElem *cur = &this->drawChain;
#if BUILT_SINCE(102g)
    DebugPrint("add draw chain (pri = %d)\n", priority);
#endif
    elem->priority = priority;

    while (cur->next != NULL) {
        if (cur->priority > priority) {
            break;
        }

        cur = cur->next;
    }

    if (cur->priority > priority) {
        elem->next = cur;
        elem->prev = cur->prev;

        if (elem->prev != NULL) {
            elem->prev->next = elem;
        }

        cur->prev = elem;
    } else {
        elem->next = NULL;
        elem->prev = cur;
        cur->next = elem;
    }

    if (elem->addedCallback != NULL) {
        return elem->addedCallback(elem->arg);
    } else {
        return ZUN_SUCCESS;
    }
}

int Chain::RunCalcChain(void) {
    ChainElem *tmp1;
    ChainElem *current;
    int updatedCount;

restart_from_first_job:
    updatedCount = 0;
    current = &this->calcChain;

    while (current != NULL) {
        if (current->callback != NULL) {
        execute_again:
            switch (current->callback(current->arg)) {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                tmp1 = current;
                current = current->next;
                Cut(tmp1);

                updatedCount++;
                continue;

            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;

            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;

            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;

            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;

            case CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB:
                goto restart_from_first_job;

            default:
                break;
            }

            updatedCount++;
        }

        current = current->next;
    }

    return updatedCount;
}

int Chain::RunDrawChain(void) {
    ChainElem *tmp1;

    int updatedCount = 0;
    ChainElem *current = &this->drawChain;

    while (current != NULL) {
        if (current->callback != NULL) {
        execute_again:
            switch (current->callback(current->arg)) {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                tmp1 = current;
                current = current->next;
                Cut(tmp1);

                updatedCount++;
                continue;

            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;

            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;

            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;

            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;

            default:
                break;
            }

            updatedCount++;
        }

        current = current->next;
    }

    return updatedCount;
}

void Chain::ReleaseSingleChain(ChainElem *root) {
    // NOTE: Those names are like this to get perfect stack frame matching
    // TODO: Give meaningfull names that still match.
    ChainElem a0;
    ChainElem *current;
    ChainElem *tmp;
    ChainElem *wasNext;

    tmp = ZUN_NEW(ChainElem);
    a0.next = tmp;

    current = root;
    while (current != NULL) {
        tmp->unkPtr = current;
        tmp->next = ZUN_NEW(ChainElem);
        tmp = tmp->next;
        current = current->next;
    }

    current = &a0;
    while (current != NULL) {
        Cut(current->unkPtr);
        current = current->next;
    }

    tmp = a0.next;

    while (tmp != NULL) {
        wasNext = tmp->next;

        ZUN_DELETE(tmp);

        tmp = wasNext;
    }
}

void Chain::Release(void) {
    ReleaseSingleChain(&this->calcChain);
    ReleaseSingleChain(&this->drawChain);
}

ChainElem *Chain::CreateElem(ChainCallback callback) {
    ChainElem *elem = ZUN_NEW(ChainElem);

    elem->SetCallback(callback);
    elem->isHeapAllocated = true;

    return elem;
}

void Chain::Cut(ChainElem *to_remove) {
#if BUILT_SINCE(013a) || BUILD_VERSION == BUILD_VERSION_013
    ZunBool isDrawChain = false;
#endif

    if (to_remove == NULL) {
        return;
    }

    ChainElem *tmp = &this->calcChain;

    while (tmp != NULL) {
        if (tmp == to_remove) {
            goto destroy_elem;
        }

        tmp = tmp->next;
    }

    {
#if BUILT_SINCE(013a) || BUILD_VERSION == BUILD_VERSION_013
        isDrawChain = true;
#endif

        tmp = &this->drawChain;
        while (tmp != NULL) {
            if (tmp == to_remove) {
                goto destroy_elem;
            }

            tmp = tmp->next;
        }
    }

    return;

destroy_elem:
#if BUILT_SINCE(013a) || BUILD_VERSION == BUILD_VERSION_013
    if (!isDrawChain) {
        DebugPrint("calc cut Chain (Pri = %d)\n", to_remove->priority);
    } else {
        DebugPrint("draw cut Chain (Pri = %d)\n", to_remove->priority);
    }
#endif

    if (to_remove->prev != NULL) {
        to_remove->callback = NULL;
        to_remove->prev->next = to_remove->next;

        if (to_remove->next != NULL) {
            to_remove->next->prev = to_remove->prev;
        }

        to_remove->prev = NULL;
        to_remove->next = NULL;

        if (to_remove->isHeapAllocated) {
            delete to_remove;
            to_remove = NULL;
        } else {
            if (to_remove->deletedCallback != NULL) {
                ChainDeletedCallback callback = to_remove->deletedCallback;
                to_remove->deletedCallback = NULL;
                callback(to_remove->arg);
            }
        }
    }
}

void AnmManager::ReleaseVertexBuffer() {
    SAFE_RELEASE(this->vertexBuffer);
}

void ZunTimer::Initialize() {
    this->current = 0;
    this->previous = -1;
    this->subFrame = 0.0f;
}

void ZunTimer::Increment(i32 value) {
    if (g_Supervisor.framerateMultiplier > 0.99f) {
        this->current += value;

        return;
    }

    if (value < 0) {
        Decrement(-value);

        return;
    }

    this->previous = this->current;
    this->subFrame += g_Supervisor.effectiveFramerateMultiplier * (float)value;

    while (this->subFrame >= 1.0f) {
        this->current += 1;
        this->subFrame -= 1.0f;
    }
}

void ZunTimer::Decrement(i32 value) {
    if (g_Supervisor.framerateMultiplier > 0.99f) {
        this->current -= value;

        return;
    }

    if (value < 0) {
        Increment(-value);

        return;
    }

    this->previous = this->current;
    this->subFrame -= g_Supervisor.effectiveFramerateMultiplier * (float)value;

    while (this->subFrame < 0.0f) {
        this->current -= 1;
        this->subFrame += 1.0f;
    }
}

void Supervisor::TickTimer(i32 *frames, f32 *subframes) {
    if (this->framerateMultiplier <= 0.99f) {
        *subframes += this->effectiveFramerateMultiplier;
        if (*subframes >= 1.0f) {
            *frames += 1;
            *subframes -= 1.0f;
        }
    } else {
        *frames += 1;
    }
}

u16 Rng::GetRandomU16(void) {
    u16 a = (this->seed ^ 0x9630) - 0x6553;

    this->seed = (((a & 0xc000) >> 14) + a * 4) & 0xFFFF;
    this->generationCount++;
    return this->seed;
}

u32 Rng::GetRandomU32(void) {
    return GetRandomU16() << 16 | GetRandomU16();
}

f32 Rng::GetRandomF32ZeroToOne(void) {
    return (f32)GetRandomU32() / (f32)0xFFFFFFFF;
}

namespace utils {
f32 AddNormalizeAngle(f32 a, f32 b) {
    i32 i = 0;
    a += b;
    while (a > ZUN_PI) {
        a -= ZUN_2PI;
        if (i++ > 16)
            break;
    }
    while (a < -ZUN_PI) {
        a += ZUN_2PI;
        if (i++ > 16)
            break;
    }
    return a;
}

#pragma var_order(sinOut, cosOut)
void Rotate(D3DXVECTOR3 *outVector, D3DXVECTOR3 *point, f32 angle) {
    f32 sinOut = sinf(angle);
    f32 cosOut = cosf(angle);
    outVector->x = cosOut * point->x + sinOut * point->y;
    outVector->y = cosOut * point->y - sinOut * point->x;
}
} // namespace utils

} // namespace th06
