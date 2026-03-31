/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 */

#ifndef LTP_SESSION_STATE_RECORD_IMPL_H
#define LTP_SESSION_STATE_RECORD_IMPL_H

namespace ns3
{
namespace ltp
{

template <typename FN>
void
SessionStateRecord::SetTimerFunction(FN fn, const Time delay, TimerCode type)
{
    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.SetFunction(fn);
        m_CpTimer.SetDelay(delay);
        break;
    case REPORT:
        m_RsTimer.SetFunction(fn);
        m_RsTimer.SetDelay(delay);
        break;
    case CANCEL:
        m_CxTimer.SetFunction(fn);
        m_CxTimer.SetDelay(delay);
        break;
    default:
        break;
    }
}

template <typename MEM_PTR, typename OBJ_PTR>
void
SessionStateRecord::SetTimerFunction(MEM_PTR memPtr,
                                     OBJ_PTR objPtr,
                                     const Time delay,
                                     TimerCode type)
{
    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.SetFunction(memPtr, objPtr);
        m_CpTimer.SetDelay(delay);
        break;
    case REPORT:
        m_RsTimer.SetFunction(memPtr, objPtr);
        m_RsTimer.SetDelay(delay);
        break;
    case CANCEL:
        m_CxTimer.SetFunction(memPtr, objPtr);
        m_CxTimer.SetDelay(delay);
        break;
    default:
        break;
    }
}

template <typename MEM_PTR, typename OBJ_PTR, typename T1>
void
SessionStateRecord::SetTimerFunction(MEM_PTR memPtr,
                                     OBJ_PTR objPtr,
                                     T1 param,
                                     const Time delay,
                                     TimerCode type)
{
    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.SetFunction(memPtr, objPtr);
        m_CpTimer.SetArguments(param);
        m_CpTimer.SetDelay(delay);
        break;
    case REPORT:
        m_RsTimer.SetFunction(memPtr, objPtr);
        m_RsTimer.SetArguments(param);
        m_RsTimer.SetDelay(delay);
        break;
    case CANCEL:
        m_CxTimer.SetFunction(memPtr, objPtr);
        m_CxTimer.SetArguments(param);
        m_CxTimer.SetDelay(delay);
        break;
    default:
        break;
    }
}

template <typename MEM_PTR, typename OBJ_PTR, typename T1, typename T2>
void
SessionStateRecord::SetTimerFunction(MEM_PTR memPtr,
                                     OBJ_PTR objPtr,
                                     T1 param,
                                     T2 param2,
                                     const Time delay,
                                     TimerCode type)
{
    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.SetFunction(memPtr, objPtr);
        m_CpTimer.SetDelay(delay);
        m_CpTimer.SetArguments(param, param2);
        break;
    case REPORT:
        m_RsTimer.SetFunction(memPtr, objPtr);
        m_RsTimer.SetDelay(delay);
        m_RsTimer.SetArguments(param, param2);
        break;
    case CANCEL:
        m_CxTimer.SetFunction(memPtr, objPtr);
        m_CxTimer.SetDelay(delay);
        m_CxTimer.SetArguments(param, param2);
        break;
    default:
        break;
    }
}

} // namespace ltp
} // namespace ns3

#endif /* LTP_SESSION_STATE_RECORD_IMPL_H */
