/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "epc-x2-sap.h"

namespace ns3
{

EpcX2Sap::~EpcX2Sap()
{
}

EpcX2Sap::ErabToBeSetupItem::ErabToBeSetupItem()
    : erabLevelQosParameters(EpsBearer(EpsBearer::GBR_CONV_VOICE))
{
}

EpcX2SapProvider::~EpcX2SapProvider()
{
}

EpcX2SapUser::~EpcX2SapUser()
{
}

// MC primitives (added to support MC functionalities)
EpcX2PdcpProvider::~EpcX2PdcpProvider()
{
}

EpcX2PdcpUser::~EpcX2PdcpUser()
{
}

EpcX2RlcProvider::~EpcX2RlcProvider()
{
}

EpcX2RlcUser::~EpcX2RlcUser()
{
}

} // namespace ns3
