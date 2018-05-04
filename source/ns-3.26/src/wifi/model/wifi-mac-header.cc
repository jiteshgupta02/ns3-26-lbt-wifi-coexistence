/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2017 Cisco and/or its affiliates
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Authors: Mirko Banchi <mk.banchi@gmail.com>
 * 
 * Modified in 2017 by                  
 *          Balamurugan Ramachandran,
 *          Ramachandra Murthy,
 *          Bibek Sahu,
 *          Mukesh Taneja
 *
 */

/* 
 * This file is modified for OFDMA/802.11ax type of systems. It is not
 * fully compliant to IEEE 802.11ax standards.
 */

#include "ns3/assert.h"
#include "ns3/address-utils.h"
#include "wifi-mac-header.h"
#include "wifi-phy.h"
#include "ns3/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMacHeader");

NS_OBJECT_ENSURE_REGISTERED (WifiMacHeader);

enum
{
  TYPE_MGT = 0,
  TYPE_CTL  = 1,
  TYPE_DATA = 2
};

enum
{
  //Reserved: 0 - 6
  SUBTYPE_CTL_CTLWRAPPER = 7,
  SUBTYPE_CTL_BACKREQ = 8,
  SUBTYPE_CTL_BACKRESP = 9,
  SUBTYPE_CTL_RTS = 11,
  SUBTYPE_CTL_CTS = 12,
  SUBTYPE_CTL_ACK = 13,
  SUBTYPE_CTL_TRIGGER = 3
};

enum
{
  TRIGGER_SUBTYPE_BASIC_TRIGGER = 0,
  TRIGGER_SUBTYPE_BEAMFORMING_RP = 1,
  TRIGGER_SUBTYPE_MU_BAR = 2,
  TRIGGER_SUBTYPE_MU_RTS = 3,
  TRIGGER_SUBTYPE_BSRP = 4,
  TRIGGER_SUBTYPE_GCR_MU_BAR = 5,
  TRIGGER_SUBTYPE_BQRP = 6
  //Reserved: 7 - 15
};

WifiMacHeader::WifiMacHeader ()
  : m_ctrlMoreData (0),
    m_ctrlWep (0),
    m_ctrlOrder (1),
    m_amsduPresent (0)
{
  m_htControlType = 0;
  m_htControlId = 0;
  m_qsize_vo = 0;
  m_qsize_vi = 0;
  m_qsize_be = 0;
  m_qsize_bk = 0;
}

WifiMacHeader::~WifiMacHeader ()
{
}

void
WifiMacHeader::SetDsFrom (void)
{
  m_ctrlFromDs = 1;
}

void
WifiMacHeader::SetDsNotFrom (void)
{
  m_ctrlFromDs = 0;
}

void
WifiMacHeader::SetDsTo (void)
{
  m_ctrlToDs = 1;
}

void
WifiMacHeader::SetDsNotTo (void)
{
  m_ctrlToDs = 0;
}

void
WifiMacHeader::SetAddr1 (Mac48Address address)
{
  m_addr1 = address;
}

void
WifiMacHeader::SetAddr2 (Mac48Address address)
{
  m_addr2 = address;
}

void
WifiMacHeader::SetAddr3 (Mac48Address address)
{
  m_addr3 = address;
}

void
WifiMacHeader::SetAddr4 (Mac48Address address)
{
  m_addr4 = address;
}

void
WifiMacHeader::SetAssocReq (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0;
}

void
WifiMacHeader::SetAssocResp (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 1;
}

void
WifiMacHeader::SetProbeReq (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 4;
}

void
WifiMacHeader::SetProbeResp (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 5;
}

void
WifiMacHeader::SetBeacon (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 8;
}

void
WifiMacHeader::SetBlockAckReq (void)
{
  m_ctrlType = TYPE_CTL;
  m_ctrlSubtype = 8;
}

void
WifiMacHeader::SetBlockAck (void)
{
  m_ctrlType = TYPE_CTL;
  m_ctrlSubtype = 9;
}

void
WifiMacHeader::SetTypeData (void)
{
  m_ctrlType = TYPE_DATA;
  m_ctrlSubtype = 0;
}

void
WifiMacHeader::SetAction (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0x0D;
}

void
WifiMacHeader::SetMultihopAction (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0x0F;
}

void
WifiMacHeader::SetType (enum WifiMacType type)
{
  switch (type)
    {
    case WIFI_MAC_CTL_CTLWRAPPER:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_CTLWRAPPER;
      break;
    case WIFI_MAC_CTL_BACKREQ:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKREQ;
      break;
    case WIFI_MAC_CTL_BACKRESP:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKRESP;
      break;
    case WIFI_MAC_CTL_RTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_RTS;
      break;
    case WIFI_MAC_CTL_CTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_CTS;
      break;
    case WIFI_MAC_CTL_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_ACK;
      break;
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 0;
      break;
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 1;
      break;
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 2;
      break;
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 3;
      break;
    case WIFI_MAC_MGT_PROBE_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 4;
      break;
    case WIFI_MAC_MGT_PROBE_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 5;
      break;
    case WIFI_MAC_MGT_BEACON:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 8;
      break;
    case WIFI_MAC_MGT_DISASSOCIATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 10;
      break;
    case WIFI_MAC_MGT_AUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 11;
      break;
    case WIFI_MAC_MGT_DEAUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 12;
      break;
    case WIFI_MAC_MGT_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 13;
      break;
    case WIFI_MAC_MGT_ACTION_NO_ACK:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 14;
      break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 15;
      break;
    case WIFI_MAC_DATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 0;
      break;
    case WIFI_MAC_DATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 1;
      break;
    case WIFI_MAC_DATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 2;
      break;
    case WIFI_MAC_DATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 3;
      break;
    case WIFI_MAC_DATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 4;
      break;
    case WIFI_MAC_DATA_NULL_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 5;
      break;
    case WIFI_MAC_DATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 6;
      break;
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 7;
      break;
    case WIFI_MAC_QOSDATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 8;
      break;
    case WIFI_MAC_QOSDATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 9;
      break;
    case WIFI_MAC_QOSDATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 10;
      break;
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 11;
      break;
    case WIFI_MAC_QOSDATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 12;
      break;
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 14;
      break;
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 15;
      break;
    case WIFI_MAC_CTL_HE_TRIGGER:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_TRIGGER;
    }
  m_ctrlToDs = 0;
  m_ctrlFromDs = 0;
}

void
WifiMacHeader::SetRawDuration (uint16_t duration)
{
  m_duration = duration;
}

void
WifiMacHeader::SetDuration (Time duration)
{
  int64_t duration_us = ceil ((double)duration.GetNanoSeconds () / 1000);
  NS_ASSERT (duration_us >= 0 && duration_us <= 0x7fff);
  m_duration = static_cast<uint16_t> (duration_us);
}

void WifiMacHeader::SetId (uint16_t id)
{
  m_duration = id;
}

void WifiMacHeader::SetSequenceNumber (uint16_t seq)
{
  m_seqSeq = seq;
}

void WifiMacHeader::SetFragmentNumber (uint8_t frag)
{
  m_seqFrag = frag;
}

void WifiMacHeader::SetNoMoreFragments (void)
{
  m_ctrlMoreFrag = 0;
}

void WifiMacHeader::SetMoreFragments (void)
{
  m_ctrlMoreFrag = 1;
}

void WifiMacHeader::SetOrder (void)
{
  m_ctrlOrder = 1;
}

void WifiMacHeader::SetNoOrder (void)
{
  m_ctrlOrder = 0;
}

void WifiMacHeader::SetRetry (void)
{
  m_ctrlRetry = 1;
}

void WifiMacHeader::SetNoRetry (void)
{
  m_ctrlRetry = 0;
}

void WifiMacHeader::SetQosTid (uint8_t tid)
{
  m_qosTid = tid;
}

void WifiMacHeader::SetQosEosp ()
{
  m_qosEosp = 1;
}

void WifiMacHeader::SetQosNoEosp ()
{
  m_qosEosp = 0;
}

void WifiMacHeader::SetQosAckPolicy (enum QosAckPolicy policy)
{
  switch (policy)
    {
    case NORMAL_ACK:
      m_qosAckPolicy = 0;
      break;
    case NO_ACK:
      m_qosAckPolicy = 1;
      break;
    case NO_EXPLICIT_ACK:
      m_qosAckPolicy = 2;
      break;
    case BLOCK_ACK:
      m_qosAckPolicy = 3;
      break;
    }
}

void
WifiMacHeader::SetQosNormalAck ()
{
  m_qosAckPolicy = 0;
}

void
WifiMacHeader::SetQosBlockAck ()
{
  m_qosAckPolicy = 3;
}

void
WifiMacHeader::SetQosNoAck ()
{
  m_qosAckPolicy = 1;
}

void WifiMacHeader::SetQosAmsdu (void)
{
  m_amsduPresent = 1;
}

void WifiMacHeader::SetQosNoAmsdu (void)
{
  m_amsduPresent = 0;
}

void WifiMacHeader::SetQosTxopLimit (uint8_t txop)
{
  m_qosStuff = txop;
}

void WifiMacHeader::SetQosMeshControlPresent (void)
{
  //Mark bit 0 of this variable instead of bit 8, since m_qosStuff is
  //shifted by one byte when serialized
  m_qosStuff = m_qosStuff | 0x01; //bit 8 of QoS Control Field
}

void WifiMacHeader::SetQosNoMeshControlPresent ()
{
  //Clear bit 0 of this variable instead of bit 8, since m_qosStuff is
  //shifted by one byte when serialized
  m_qosStuff = m_qosStuff & 0xfe; //bit 8 of QoS Control Field
}


Mac48Address
WifiMacHeader::GetAddr1 (void) const
{
  return m_addr1;
}

Mac48Address
WifiMacHeader::GetAddr2 (void) const
{
  return m_addr2;
}

Mac48Address
WifiMacHeader::GetAddr3 (void) const
{
  return m_addr3;
}

Mac48Address
WifiMacHeader::GetAddr4 (void) const
{
  return m_addr4;
}

enum WifiMacType
WifiMacHeader::GetType (void) const
{
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      switch (m_ctrlSubtype)
        {
        case 0:
          return WIFI_MAC_MGT_ASSOCIATION_REQUEST;
          break;
        case 1:
          return WIFI_MAC_MGT_ASSOCIATION_RESPONSE;
          break;
        case 2:
          return WIFI_MAC_MGT_REASSOCIATION_REQUEST;
          break;
        case 3:
          return WIFI_MAC_MGT_REASSOCIATION_RESPONSE;
          break;
        case 4:
          return WIFI_MAC_MGT_PROBE_REQUEST;
          break;
        case 5:
          return WIFI_MAC_MGT_PROBE_RESPONSE;
          break;
        case 8:
          return WIFI_MAC_MGT_BEACON;
          break;
        case 10:
          return WIFI_MAC_MGT_DISASSOCIATION;
          break;
        case 11:
          return WIFI_MAC_MGT_AUTHENTICATION;
          break;
        case 12:
          return WIFI_MAC_MGT_DEAUTHENTICATION;
          break;
        case 13:
          return WIFI_MAC_MGT_ACTION;
          break;
        case 14:
          return WIFI_MAC_MGT_ACTION_NO_ACK;
          break;
        case 15:
          return WIFI_MAC_MGT_MULTIHOP_ACTION;
          break;
        }
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_BACKREQ:
          return WIFI_MAC_CTL_BACKREQ;
          break;
        case SUBTYPE_CTL_BACKRESP:
          return WIFI_MAC_CTL_BACKRESP;
          break;
        case SUBTYPE_CTL_RTS:
          return WIFI_MAC_CTL_RTS;
          break;
        case SUBTYPE_CTL_CTS:
          return WIFI_MAC_CTL_CTS;
          break;
        case SUBTYPE_CTL_ACK:
          return WIFI_MAC_CTL_ACK;
          break;
        case SUBTYPE_CTL_TRIGGER:
          return WIFI_MAC_CTL_HE_TRIGGER;
          break;  
        }
      break;
    case TYPE_DATA:
      switch (m_ctrlSubtype)
        {
        case 0:
          return WIFI_MAC_DATA;
          break;
        case 1:
          return WIFI_MAC_DATA_CFACK;
          break;
        case 2:
          return WIFI_MAC_DATA_CFPOLL;
          break;
        case 3:
          return WIFI_MAC_DATA_CFACK_CFPOLL;
          break;
        case 4:
          return WIFI_MAC_DATA_NULL;
          break;
        case 5:
          return WIFI_MAC_DATA_NULL_CFACK;
          break;
        case 6:
          return WIFI_MAC_DATA_NULL_CFPOLL;
          break;
        case 7:
          return WIFI_MAC_DATA_NULL_CFACK_CFPOLL;
          break;
        case 8:
          return WIFI_MAC_QOSDATA;
          break;
        case 9:
          return WIFI_MAC_QOSDATA_CFACK;
          break;
        case 10:
          return WIFI_MAC_QOSDATA_CFPOLL;
          break;
        case 11:
          return WIFI_MAC_QOSDATA_CFACK_CFPOLL;
          break;
        case 12:
          return WIFI_MAC_QOSDATA_NULL;
          break;
        case 14:
          return WIFI_MAC_QOSDATA_NULL_CFPOLL;
          break;
        case 15:
          return WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL;
          break;
        }
      break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (enum WifiMacType) -1;
}

bool
WifiMacHeader::IsFromDs (void) const
{
  return m_ctrlFromDs == 1;
}

bool
WifiMacHeader::IsToDs (void) const
{
  return m_ctrlToDs == 1;
}

bool
WifiMacHeader::IsData (void) const
{
  return (m_ctrlType == TYPE_DATA);

}

bool
WifiMacHeader::IsSetOrder (void) const
{
  return (m_ctrlOrder == 1); 
}

bool
WifiMacHeader::IsCtrlBsrHdr (void) const
{
  return ((m_htControlType == 3) && (m_htControlId == 3)); 
}

bool
WifiMacHeader::IsCtrlBsrMtid (void) const
{
  return ((m_htControlType == 3) && (m_htControlId == 7));
}

bool
WifiMacHeader::IsQosData (void) const
{
  return (m_ctrlType == TYPE_DATA && (m_ctrlSubtype & 0x08));
}

bool
WifiMacHeader::IsCtl (void) const
{
  return (m_ctrlType == TYPE_CTL);
}

bool
WifiMacHeader::IsMgt (void) const
{
  return (m_ctrlType == TYPE_MGT);
}

bool
WifiMacHeader::IsCfpoll (void) const
{
  switch (GetType ())
    {
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      return true;
      break;
    default:
      return false;
      break;
    }
}

bool
WifiMacHeader::IsRts (void) const
{
  return (GetType () == WIFI_MAC_CTL_RTS);
}

bool
WifiMacHeader::IsHeTrigger (void) const
{
  return (GetType () == WIFI_MAC_CTL_HE_TRIGGER);
}

bool
WifiMacHeader::IsCts (void) const
{
  return (GetType () == WIFI_MAC_CTL_CTS);
}

bool
WifiMacHeader::IsAck (void) const
{
  return (GetType () == WIFI_MAC_CTL_ACK);
}

bool
WifiMacHeader::IsAssocReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_ASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsAssocResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsReassocReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_REASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsReassocResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsProbeReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_PROBE_REQUEST);
}

bool
WifiMacHeader::IsProbeResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_PROBE_RESPONSE);
}

bool
WifiMacHeader::IsBeacon (void) const
{
  return (GetType () == WIFI_MAC_MGT_BEACON);
}

bool
WifiMacHeader::IsDisassociation (void) const
{
  return (GetType () == WIFI_MAC_MGT_DISASSOCIATION);
}

bool
WifiMacHeader::IsAuthentication (void) const
{
  return (GetType () == WIFI_MAC_MGT_AUTHENTICATION);
}

bool
WifiMacHeader::IsDeauthentication (void) const
{
  return (GetType () == WIFI_MAC_MGT_DEAUTHENTICATION);
}

bool
WifiMacHeader::IsAction (void) const
{
  return (GetType () == WIFI_MAC_MGT_ACTION);
}

bool
WifiMacHeader::IsMultihopAction (void) const
{
  return (GetType () == WIFI_MAC_MGT_MULTIHOP_ACTION);
}

bool
WifiMacHeader::IsBlockAckReq (void) const
{
  return (GetType () == WIFI_MAC_CTL_BACKREQ) ? true : false;
}

bool
WifiMacHeader::IsBlockAck (void) const
{
  return (GetType () == WIFI_MAC_CTL_BACKRESP) ? true : false;
}

uint16_t
WifiMacHeader::GetRawDuration (void) const
{
  return m_duration;
}

Time
WifiMacHeader::GetDuration (void) const
{
  return MicroSeconds (m_duration);
}

uint16_t
WifiMacHeader::GetSequenceControl (void) const
{
  return (m_seqSeq << 4) | m_seqFrag;
}

uint16_t
WifiMacHeader::GetSequenceNumber (void) const
{
  return m_seqSeq;
}

uint16_t
WifiMacHeader::GetFragmentNumber (void) const
{
  return m_seqFrag;
}

bool
WifiMacHeader::IsRetry (void) const
{
  return (m_ctrlRetry == 1);
}

bool
WifiMacHeader::IsMoreFragments (void) const
{
  return (m_ctrlMoreFrag == 1);
}

bool
WifiMacHeader::IsQosBlockAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 3);
}

bool
WifiMacHeader::IsQosNoAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 1);
}

bool
WifiMacHeader::IsQosAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 0);
}

bool
WifiMacHeader::IsQosEosp (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosEosp == 1);
}

bool
WifiMacHeader::IsQosAmsdu (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_amsduPresent == 1);
}

uint8_t
WifiMacHeader::GetQosTid (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosTid;
}

enum WifiMacHeader::QosAckPolicy
WifiMacHeader::GetQosAckPolicy (void) const
{
  switch (m_qosAckPolicy)
    {
    case 0:
      return NORMAL_ACK;
      break;
    case 1:
      return NO_ACK;
      break;
    case 2:
      return NO_EXPLICIT_ACK;
      break;
    case 3:
      return BLOCK_ACK;
      break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (enum QosAckPolicy) -1;
}

uint8_t
WifiMacHeader::GetQosTxopLimit (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosStuff;
}

uint16_t
WifiMacHeader::GetFrameControl (void) const
{
  uint16_t val = 0;
  val |= (m_ctrlType << 2) & (0x3 << 2);
  val |= (m_ctrlSubtype << 4) & (0xf << 4);
  val |= (m_ctrlToDs << 8) & (0x1 << 8);
  val |= (m_ctrlFromDs << 9) & (0x1 << 9);
  val |= (m_ctrlMoreFrag << 10) & (0x1 << 10);
  val |= (m_ctrlRetry << 11) & (0x1 << 11);
  val |= (m_ctrlMoreData << 13) & (0x1 << 13);
  val |= (m_ctrlWep << 14) & (0x1 << 14);
  val |= (m_ctrlOrder << 15) & (0x1 << 15);
  return val;
}

uint16_t
WifiMacHeader::GetQosControl (void) const
{
  uint16_t val = 0;
  val |= m_qosTid;
  val |= m_qosEosp << 4;
  val |= m_qosAckPolicy << 5;
  val |= m_amsduPresent << 7;
  val |= m_qosStuff << 8;
  return val;
}

void
WifiMacHeader::UpdateControlHeaderBsrp(uint8_t aciBitmap, uint8_t maxAc, uint32_t maxQueueSf, uint32_t totalSf)
{
  m_htControlType = 3;   /* HT + HE Control header */
  m_htControlAcibitmap = aciBitmap;
  m_htControlId = 3;     /* For Bsr Id */
  m_htControlDeltatid = 0;
  m_htControlAcihigh = maxAc;
  m_htControlSf = 0;
  m_htControlQueueHigh = maxQueueSf;
  m_htControlQueueAll = totalSf;
}

uint32_t
WifiMacHeader::GetControlHeader (void) const
{
  uint32_t val = 0;

  val |= m_htControlType;
  val |= m_htControlId << 2;
  if (m_htControlId == 3) //BSR bitmap status
    {
      /* This function is only for Buffer Status Report. */
      val |= m_htControlAcibitmap << 6;     /* 2 + 4 */
      val |= m_htControlDeltatid << 10;     /* 2 + 4 + 4 */
      val |= m_htControlAcihigh << 12;      /* 2 + 4 + 4 + 2 */
      val |= m_htControlSf << 14;           /* 2 + 4 + 4 + 2 + 2 */
      val |= m_htControlQueueHigh << 16;    /* 2 + 4 + 4 + 2 + 2 + 2 */
      val |= m_htControlQueueAll << 24;     /* 2 + 4 + 4 + 2 + 2 + 2 + 8 */
    }
  else if (m_htControlId == 7) //BSR multi tid status
    {
      /* This function is multi TID Buffer Status Report. */
      val |= m_qsize_be << 6;               /* 2 + 4 */
      val |= m_qsize_bk << 12;              /* 2 + 4 + 6 */
      val |= m_qsize_vi << 18;              /* 2 + 4 + 6 + 6 */
      val |= m_qsize_vo << 24;              /* 2 + 4 + 6 +6 +6 */
    }
  return val;
}

void
WifiMacHeader::SetMultiQueueInfo (WifiMacHeader hdr)
{
  m_htControlType = 3;
  m_htControlId = 7;
  m_qsize_vo = hdr.GetVOSize();
  m_qsize_vi = hdr.GetVISize();
  m_qsize_be = hdr.GetBESize();
  m_qsize_bk = hdr.GetBKSize();
}

void
WifiMacHeader::SetVOSize (uint8_t qsize)
{
  m_htControlType = 3;
  m_htControlId = 7;
  m_qsize_vo = qsize;
}

void
WifiMacHeader::SetVISize (uint8_t qsize)
{
  m_htControlType = 3;
  m_htControlId = 7;
  m_qsize_vi = qsize;
}

void
WifiMacHeader::SetBESize (uint8_t qsize)
{
  m_htControlType = 3;
  m_htControlId = 7;
  m_qsize_be = qsize;
}

void
WifiMacHeader::SetBKSize (uint8_t qsize)
{
  m_htControlType = 3;
  m_htControlId = 7;
  m_qsize_bk = qsize;
}

uint8_t
WifiMacHeader::GetVOSize (void) const
{
  return m_qsize_vo;
}

uint8_t
WifiMacHeader::GetVISize (void) const
{
  return m_qsize_vi;
}

uint8_t
WifiMacHeader::GetBESize (void) const
{
  return m_qsize_be;
}

uint8_t
WifiMacHeader::GetBKSize (void) const
{
  return m_qsize_bk;
}

uint16_t
WifiMacHeader::GetAcibitmap ()
{
  return m_htControlAcibitmap;
}

uint16_t
WifiMacHeader::GetAcihigh ()
{
  return m_htControlAcihigh;
} 

uint16_t
WifiMacHeader::GetSfQueuehigh ()
{
  return m_htControlQueueHigh;
} 

uint16_t
WifiMacHeader::GetSfQueueAll ()
{
  return m_htControlQueueAll;
} 

void
WifiMacHeader::SetControlHeader (uint32_t htCtrlHeader)
{
  m_htControlType = htCtrlHeader & 0x00000003;
  m_htControlId = (htCtrlHeader >> 2) & 0x0000000f;
  if (m_htControlId == 3) //BSR bitmap status
    {
      m_htControlAcibitmap = (htCtrlHeader >> 6) & 0x0000000f;
      m_htControlDeltatid = (htCtrlHeader >> 10) & 0x00000003;
      m_htControlAcihigh = (htCtrlHeader >> 12) & 0x00000003;
      m_htControlSf = (htCtrlHeader >> 14) & 0x00000003;
      m_htControlQueueHigh = (htCtrlHeader >> 16) & 0x000000ff;
      m_htControlQueueAll = (htCtrlHeader >> 24) & 0x000000ff;
    }
  else if (m_htControlId == 7) //BSR multi tid status
    {
      /* This function is multi TID Buffer Status Report. */
      m_qsize_be = (htCtrlHeader >> 6) & 0x0000003f;
      m_qsize_bk = (htCtrlHeader >> 12) & 0x0000003f;
      m_qsize_vi = (htCtrlHeader >> 18) & 0x0000003f;
      m_qsize_vo = (htCtrlHeader >> 24) & 0x0000003f;
    }
}

void
WifiMacHeader::SetFrameControl (uint16_t ctrl)
{
  m_ctrlType = (ctrl >> 2) & 0x03;
  m_ctrlSubtype = (ctrl >> 4) & 0x0f;
  m_ctrlToDs = (ctrl >> 8) & 0x01;
  m_ctrlFromDs = (ctrl >> 9) & 0x01;
  m_ctrlMoreFrag = (ctrl >> 10) & 0x01;
  m_ctrlRetry = (ctrl >> 11) & 0x01;
  m_ctrlMoreData = (ctrl >> 13) & 0x01;
  m_ctrlWep = (ctrl >> 14) & 0x01;
  m_ctrlOrder = (ctrl >> 15) & 0x01;
}
void
WifiMacHeader::SetSequenceControl (uint16_t seq)
{
  m_seqFrag = seq & 0x0f;
  m_seqSeq = (seq >> 4) & 0x0fff;
}
void
WifiMacHeader::SetQosControl (uint16_t qos)
{
  m_qosTid = qos & 0x000f;
  m_qosEosp = (qos >> 4) & 0x0001;
  m_qosAckPolicy = (qos >> 5) & 0x0003;
  m_amsduPresent = (qos >> 7) & 0x0001;
  m_qosStuff = (qos >> 8) & 0x00ff;
}

uint32_t
WifiMacHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_TRIGGER:
          size = 2 + 2 + 6 + 6;
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          size = 2 + 2 + 6;
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          size = 2 + 2 + 6 + 6;
          break;
        case SUBTYPE_CTL_CTLWRAPPER:
          size = 2 + 2 + 6 + 2 + 4;
          break;
        }
      break;
    case TYPE_DATA:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          size += 6;
        }
      if (m_ctrlSubtype & 0x08)
        {
          size += 2;
          if(m_ctrlOrder) {
              size += 4;
          }
        }
      break;
    }
  return size;
}

const char *
WifiMacHeader::GetTypeString (void) const
{
#define FOO(x) \
case WIFI_MAC_ ## x: \
  return # x; \
  break;

  switch (GetType ())
    {
      FOO (CTL_RTS);
      FOO (CTL_CTS);
      FOO (CTL_ACK);
      FOO (CTL_BACKREQ);
      FOO (CTL_BACKRESP);

      FOO (MGT_BEACON);
      FOO (MGT_ASSOCIATION_REQUEST);
      FOO (MGT_ASSOCIATION_RESPONSE);
      FOO (MGT_DISASSOCIATION);
      FOO (MGT_REASSOCIATION_REQUEST);
      FOO (MGT_REASSOCIATION_RESPONSE);
      FOO (MGT_PROBE_REQUEST);
      FOO (MGT_PROBE_RESPONSE);
      FOO (MGT_AUTHENTICATION);
      FOO (MGT_DEAUTHENTICATION);
      FOO (MGT_ACTION);
      FOO (MGT_ACTION_NO_ACK);
      FOO (MGT_MULTIHOP_ACTION);

      FOO (DATA);
      FOO (DATA_CFACK);
      FOO (DATA_CFPOLL);
      FOO (DATA_CFACK_CFPOLL);
      FOO (DATA_NULL);
      FOO (DATA_NULL_CFACK);
      FOO (DATA_NULL_CFPOLL);
      FOO (DATA_NULL_CFACK_CFPOLL);
      FOO (QOSDATA);
      FOO (QOSDATA_CFACK);
      FOO (QOSDATA_CFPOLL);
      FOO (QOSDATA_CFACK_CFPOLL);
      FOO (QOSDATA_NULL);
      FOO (QOSDATA_NULL_CFPOLL);
      FOO (QOSDATA_NULL_CFACK_CFPOLL);
    default:
      return "ERROR";
    }
#undef FOO
  // needed to make gcc 4.0.1 ppc darwin happy.
  return "BIG_ERROR";
}

TypeId
WifiMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiMacHeader> ()
  ;
  return tid;
}

TypeId
WifiMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiMacHeader::PrintFrameControl (std::ostream &os) const
{
  os << "ToDS=" << std::hex << (int) m_ctrlToDs << ", FromDS=" << std::hex << (int) m_ctrlFromDs
     << ", MoreFrag=" <<  std::hex << (int) m_ctrlMoreFrag << ", Retry=" << std::hex << (int) m_ctrlRetry
     << ", MoreData=" <<  std::hex << (int) m_ctrlMoreData << std::dec
  ;
}

void
WifiMacHeader::Print (std::ostream &os) const
{
  os << GetTypeString () << " ";
  switch (GetType ())
    {
    case WIFI_MAC_CTL_RTS:
    case WIFI_MAC_CTL_HE_TRIGGER:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1 << ", TA=" << m_addr2;
      break;
    case WIFI_MAC_CTL_CTS:
    case WIFI_MAC_CTL_ACK:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1;
      break;
    case WIFI_MAC_CTL_BACKREQ:
      break;
    case WIFI_MAC_CTL_BACKRESP:
      break;
    case WIFI_MAC_CTL_CTLWRAPPER:
      break;
    case WIFI_MAC_MGT_BEACON:
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_DISASSOCIATION:
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_PROBE_REQUEST:
    case WIFI_MAC_MGT_PROBE_RESPONSE:
    case WIFI_MAC_MGT_AUTHENTICATION:
    case WIFI_MAC_MGT_DEAUTHENTICATION:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << ", DA=" << m_addr1 << ", SA=" << m_addr2
         << ", BSSID=" << m_addr3 << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_MGT_ACTION:
    case WIFI_MAC_MGT_ACTION_NO_ACK:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << ", DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
      os << " Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1 << ", TA=" << m_addr2 << ", DA=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_DATA:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us";
      if (!m_ctrlToDs && !m_ctrlFromDs)
        {
          os << ", DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3;
        }
      else if (!m_ctrlToDs && m_ctrlFromDs)
        {
          os << ", DA=" << m_addr1 << ", SA=" << m_addr3 << ", BSSID=" << m_addr2;
        }
      else if (m_ctrlToDs && !m_ctrlFromDs)
        {
          os << ", DA=" << m_addr3 << ", SA=" << m_addr2 << ", BSSID=" << m_addr1;
        }
      else if (m_ctrlToDs && m_ctrlFromDs)
        {
          os << ", DA=" << m_addr3 << ", SA=" << m_addr4 << ", RA=" << m_addr1 << ", TA=" << m_addr2;
        }
      else
        {
          NS_FATAL_ERROR ("Impossible ToDs and FromDs flags combination");
        }
      os << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_DATA_CFACK:
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL:
    case WIFI_MAC_DATA_NULL_CFACK:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA:
    case WIFI_MAC_QOSDATA_CFACK:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      break;
    }
}

uint32_t
WifiMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

void
WifiMacHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtolsbU16 (GetFrameControl ());
  i.WriteHtolsbU16 (m_duration);
  WriteTo (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      WriteTo (i, m_addr2);
      WriteTo (i, m_addr3);
      i.WriteHtolsbU16 (GetSequenceControl ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_TRIGGER:
          WriteTo (i, m_addr2);
          break;
        
        default:
          //NOTREACHED
          NS_ASSERT (false);
          break;
        }
      break;
    case TYPE_DATA:
      {
        WriteTo (i, m_addr2);
        WriteTo (i, m_addr3);
        i.WriteHtolsbU16 (GetSequenceControl ());
        if (m_ctrlToDs && m_ctrlFromDs)
          {
            WriteTo (i, m_addr4);
          }
        if (m_ctrlSubtype & 0x08)
          {
            i.WriteHtolsbU16 (GetQosControl ());
            /* If data, if order bit set, then HT Control Header is present. */
            if(m_ctrlOrder) {
                i.WriteHtolsbU32 (GetControlHeader ());
            }
          }
      } break;
    default:
      //NOTREACHED
      NS_ASSERT (false);
      break;
    }
}

uint32_t
WifiMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t frame_control = i.ReadLsbtohU16 ();
  SetFrameControl (frame_control);
  m_duration = i.ReadLsbtohU16 ();
  ReadFrom (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_TRIGGER: 
          ReadFrom (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          ReadFrom (i, m_addr2);
          break;
        }
      break;
    case TYPE_DATA:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          ReadFrom (i, m_addr4);
        }
      if (m_ctrlSubtype & 0x08)
        {
          SetQosControl (i.ReadLsbtohU16 ());
          if(m_ctrlOrder) { 
             SetControlHeader(i.ReadLsbtohU32());
          }
        }
      break;
    }
  return i.GetDistanceFrom (start);
}

WifiHeTriggerMacHeader::WifiHeTriggerMacHeader()
{
}

WifiHeTriggerMacHeader::~WifiHeTriggerMacHeader()
{
}

bool
WifiHeTriggerMacHeader::IsMuRts (void) const
{
  return (GetType () == WIFI_MAC_CTL_TRIGGER_HE_MU_RTS);
}

bool
WifiHeTriggerMacHeader::IsBasicTrigger (void) const
{
  return (GetType () == WIFI_MAC_CTL_TRIGGER_HE_BASIC_TRIGGER);
}

bool
WifiHeTriggerMacHeader::IsBsrpTrigger (void) const
{
  return (GetType () == WIFI_MAC_CTL_TRIGGER_HE_BSRP);
}

void
WifiHeTriggerMacHeader::SetType (enum WifiHeTriggerMacType type)
{
  switch (type)
    {
    case WIFI_MAC_CTL_TRIGGER_HE_MU_RTS:
      m_trgSubType = TRIGGER_SUBTYPE_MU_RTS;
      break;
    case WIFI_MAC_CTL_TRIGGER_HE_BASIC_TRIGGER:
      m_trgSubType = TRIGGER_SUBTYPE_BASIC_TRIGGER;
      break;
    case WIFI_MAC_CTL_TRIGGER_HE_MU_BAR:
      m_trgSubType = TRIGGER_SUBTYPE_MU_BAR;
      break;
    case WIFI_MAC_CTL_TRIGGER_HE_BSRP:
      m_trgSubType = TRIGGER_SUBTYPE_BSRP;
      break;
    case WIFI_MAC_CTL_TRIGGER_HE_BEGIN:
    case WIFI_MAC_CTL_TRIGGER_HE_END:
      break;
    }
}

TypeId
WifiHeTriggerMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiHeTriggerMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiHeTriggerMacHeader> ()
  ;
  return tid;
}

TypeId
WifiHeTriggerMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiHeTriggerMacHeader::Print (std::ostream &os) const
{
  os << GetTypeString () << " ";
  switch (GetType ())
    {
    case WIFI_MAC_CTL_TRIGGER_HE_MU_RTS:
    case WIFI_MAC_CTL_TRIGGER_HE_BASIC_TRIGGER:
    case WIFI_MAC_CTL_TRIGGER_HE_MU_BAR:
    case WIFI_MAC_CTL_TRIGGER_HE_BSRP:
      os   << "length=" << m_length << ", BW="
         << m_bw << "Num Users=" << m_numOfUsers;
      break;
    case WIFI_MAC_CTL_TRIGGER_HE_BEGIN:
    case WIFI_MAC_CTL_TRIGGER_HE_END:
      break;
    }
  return;
}

uint32_t
WifiHeTriggerMacHeader::GetSize (void) const
{
  uint32_t size;
  
  size  =  8 + 1;  //Common Info + no of users
  return size;
}

uint32_t
WifiHeTriggerMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

const char *
WifiHeTriggerMacHeader::GetTypeString (void) const
{
#define FOO(x) \
case WIFI_MAC_ ## x: \
  return # x; \
  break;

  switch (GetType ())
    {
      FOO (CTL_TRIGGER_HE_MU_RTS);
    default:
      return "ERROR";
    }
#undef FOO
  return "BIG_ERROR";
} 

void
WifiHeTriggerMacHeader::Serialize (Buffer::Iterator i) const
{
  uint32_t val = 0;
  // common info
  i.WriteHtolsbU32 (GetCommonInfo());
  i.WriteHtolsbU32 (val);
 
  i.WriteU8(m_numOfUsers);
  
  return;
}

void
WifiHeTriggerMacHeader::ConfigTriggerSubType(uint16_t triggerSubTypes)
{
   m_trgSubType = triggerSubTypes; 
}

uint32_t
WifiHeTriggerMacHeader::GetCommonInfo (void) const
{
  uint32_t val = 0;

  val |= ((m_trgSubType) & (0xf));  // Trigger Type
//  val |= ((m_length<<4) & (0xfff<<4));  // Length Type
//  val |= ((m_bw<<18) & (0x3<<18));  // BW 

  return val;
}

enum WifiHeTriggerMacType 
WifiHeTriggerMacHeader::GetType (void) const
{
   switch (m_trgSubType)
   {
        case TRIGGER_SUBTYPE_MU_RTS:
          return WIFI_MAC_CTL_TRIGGER_HE_MU_RTS;
          break;
        case TRIGGER_SUBTYPE_BASIC_TRIGGER:
          return WIFI_MAC_CTL_TRIGGER_HE_BASIC_TRIGGER;
          break;
        case TRIGGER_SUBTYPE_MU_BAR:
          return WIFI_MAC_CTL_TRIGGER_HE_MU_BAR;
          break;
        case TRIGGER_SUBTYPE_BSRP:
          return WIFI_MAC_CTL_TRIGGER_HE_BSRP;
          break;
   }
   return (enum WifiHeTriggerMacType) -1;
}

uint32_t
WifiHeTriggerMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t common_info = i.ReadLsbtohU32 ();
  SetCommonInfo (common_info);

  common_info =  i.ReadLsbtohU32 ();
  m_numOfUsers = i.ReadU8 ();

  return i.GetDistanceFrom (start);
}

void
WifiHeTriggerMacHeader::SetCommonInfo (uint32_t commonInfo)
{
  m_trgSubType = commonInfo & 0xf; 
  m_length = (commonInfo >> 4) & 0x0f;
  m_bw = (commonInfo >> 18) & 0xfff;
}

uint8_t 
WifiHeTriggerMacHeader::GetNumOfUsers (void)
{
  return m_numOfUsers;
}

void 
WifiHeTriggerMacHeader::SetNumOfUsers (uint8_t users)
{
  m_numOfUsers = users;
}

/* MU RTS Header */
WifiHeMuRtsHeader::WifiHeMuRtsHeader()
{
}

WifiHeMuRtsHeader::~WifiHeMuRtsHeader()
{
}

TypeId
WifiHeMuRtsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiHeMuRtsHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiHeMuRtsHeader> ()
  ;
  return tid;
}

TypeId
WifiHeMuRtsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiHeMuRtsHeader::Print (std::ostream &os) const
{
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      os << "WifiHeMuRtsHeader:Print Addreess: " << j->first
         << "aid :" << j->second.m_aid << "RU Index :"
         << j->second.index << ", MCS=" << j->second.mcs
         << ", coding type=" << j->second.codingType << ", DCM="
         << j->second.dcm << ", SS Allocation=" << j->second.ssAllocation;;
  }
  return;
}

uint32_t
WifiHeMuRtsHeader::GetSize (void) const
{
  uint32_t size = 0;

  // Each User Info is 5 bytes * number of User Info.
  size = 5 * staRuMapInfo.size();

  return size;
}

uint32_t
WifiHeMuRtsHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

void
WifiHeMuRtsHeader::SetStaRuMap(staRuMap staMap) 
{
    staRuMapInfo = staMap;
}


void
WifiHeMuRtsHeader::Serialize (Buffer::Iterator i) const
{
  uint32_t value = 0;  
  uint8_t  reserved = 0;

  // User Info
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      value = 0;
      // Get Aid from MacAddress - 12 bits.
      value |= (j->second.m_aid) & (0xfff);
      // RU Allocation - 8 bits

      value |= (j->second.index << 12) & (0xff << 12);
      // Coding Type - 1 bit
      value |= (j->second.codingType << 20) & (0x1 << 20);
      // MCS  - 4 bits
      value |= (j->second.mcs << 21) & (0xf << 21);
      // DCM - 1 bit
      value |= (j->second.dcm << 25) & (0x1 << 25);
      // SS Allocation - 6 bits
      value |= (j->second.ssAllocation << 26) & (0x3f << 26);

      i.WriteHtolsbU32 (value);
      i.WriteU8(reserved);   //Reserved one byte

  }
  
  return;
}

void 
WifiHeMuRtsHeader::SetNumOfUsers (uint8_t users)
{
  m_numOfUsers = users;
}

staRuMap 
WifiHeMuRtsHeader::GetRuMap()
{
   return staRuMapInfo;
}

uint32_t
WifiHeMuRtsHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t j = 0;
  RuInfo ruInfo;
  uint32_t value = 0, valueTmp = 0;  
  uint8_t reserved = 0;
 
  staRuMapInfo.clear();
  for(j = 0; j < m_numOfUsers; j ++) {
      value = 0;
      valueTmp = 0;
      value = i.ReadLsbtohU32 ();

      valueTmp = value;
      ruInfo.m_aid = (valueTmp) & (0xfff);
      valueTmp = value;
      ruInfo.index = (valueTmp >> 12) & (0xff);
      valueTmp = value;
      ruInfo.codingType = (valueTmp >> 20) & (0x1);
      valueTmp = value;
      ruInfo.mcs = (valueTmp >> 21) & (0xf);
      valueTmp = value;
      ruInfo.dcm = (valueTmp >> 25) & (0x1);
      valueTmp = value;
      ruInfo.mcs = (valueTmp >> 26) & (0x3f);

      staRuMapInfo.insert (std::make_pair (ruInfo.m_aid, ruInfo));
      reserved = 0;
      reserved = i.ReadU8 ();
      if (reserved) {
      }
  }

  return i.GetDistanceFrom (start);
}

/* Basic Trigger */

WifiHEBasicTriggerMacHeader::WifiHEBasicTriggerMacHeader()
{
}

WifiHEBasicTriggerMacHeader::~WifiHEBasicTriggerMacHeader()
{
}

void
WifiHEBasicTriggerMacHeader::SetStaRuMap(staRuMap staMap) 
{
    staRuMapInfo = staMap;
}

TypeId
WifiHEBasicTriggerMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiHEBasicTriggerMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiHEBasicTriggerMacHeader> ()
  ;
  return tid;
}

TypeId
WifiHEBasicTriggerMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiHEBasicTriggerMacHeader::Print (std::ostream &os) const
{
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      os << "WifiHeMuRtsHeader:Print Addreess: " << j->first
         << "aid :" << j->second.m_aid << "RU Index :"
         << j->second.index << ", MCS=" << j->second.mcs
         << ", coding type=" << j->second.codingType << ", DCM="
         << j->second.dcm << ", SS Allocation=" << j->second.ssAllocation
         << ", Spacing Factor=" << j->second.mpduMuSpacingFactor
         << ", TID Aggregation Limit=" << j->second.tidAggregationLimit
         << ", AC Pref Level=" << j->second.acPreferenceLevel
         << ", Preferred AC=" << j->second.preferredAc;
  }
  return;
}

uint32_t
WifiHEBasicTriggerMacHeader::GetSize (void) const
{
  uint32_t size = 0;

  // Each User Info is 6 bytes * number of User Info.
  size = 6 * staRuMapInfo.size();

  return size;
}

uint32_t
WifiHEBasicTriggerMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

staRuMap 
WifiHEBasicTriggerMacHeader::GetRuMap()
{
   return staRuMapInfo;
}

void
WifiHEBasicTriggerMacHeader::Serialize (Buffer::Iterator i) const
{
  uint32_t value = 0;  
  uint8_t  reserved = 0;
  uint8_t  triggerDependent = 0;

  // User Info
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      value = 0;
      // Get Aid from MacAddress - 12 bits.
      value |= (j->second.m_aid) & (0xfff);
      // RU Allocation - 8 bits
      value |= (j->second.index << 12) & (0xff << 12);
      // Coding Type - 1 bit
      value |= (j->second.codingType << 20) & (0x1 << 20);
      // MCS  - 4 bits
      value |= (j->second.mcs << 21) & (0xf << 21);
      // DCM - 1 bit
      value |= (j->second.dcm << 25) & (0x1 << 25);
      // SS Allocation - 6 bits
      value |= (j->second.ssAllocation << 26) & (0x3f << 26);

      i.WriteHtolsbU32 (value);
      i.WriteU8(reserved);   //Reserved one byte

      // MU Spacing factor;
      triggerDependent |= (j->second.mpduMuSpacingFactor) & (0x3);
      triggerDependent |= (j->second.tidAggregationLimit << 2) & (0x7 << 2);
      triggerDependent |= (j->second.acPreferenceLevel << 5) & (0x1 << 5);
      triggerDependent |= (j->second.preferredAc << 6) & (0x3 << 6);

      i.WriteU8(triggerDependent);   //Reserved one byte
  }
  
  return;
}

uint32_t
WifiHEBasicTriggerMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t j = 0;
  RuInfo ruInfo;
  Mac48Address  macAddress;
  uint32_t value = 0, valueTmp = 0;  
 
  staRuMapInfo.clear();
  for(j = 0; j < m_numOfUsers; j ++) {
      value = 0;
      valueTmp = 0;
      value = i.ReadLsbtohU32 ();

      valueTmp = value;
      ruInfo.m_aid = (valueTmp) & (0xfff);
      valueTmp = value;
      ruInfo.index = (valueTmp >> 12) & (0xff);

      valueTmp = value;
      ruInfo.codingType = (valueTmp >> 20) & (0x1);
      valueTmp = value;
      ruInfo.mcs = (valueTmp >> 21) & (0xf);
      valueTmp = value;
      ruInfo.dcm = (valueTmp >> 25) & (0x1);
      valueTmp = value;
      ruInfo.ssAllocation = (valueTmp >> 26) & (0x3f);

      value = 0;
      value = i.ReadU8 ();
      value = 0;
      value = i.ReadU8 ();

      valueTmp = value;
      ruInfo.mpduMuSpacingFactor = (valueTmp) & (0x3);
      valueTmp = value;
      ruInfo.tidAggregationLimit = (valueTmp >> 2) & (0x7);
      valueTmp = value;
      ruInfo.acPreferenceLevel = (valueTmp >> 5) & (0x1);
      valueTmp = value;
      ruInfo.preferredAc = (valueTmp >> 6) & (0x3);

      staRuMapInfo.insert (std::make_pair (ruInfo.m_aid, ruInfo));
  }

  return i.GetDistanceFrom (start);
}

void 
WifiHEBasicTriggerMacHeader::SetNumOfUsers (uint8_t users)
{
  m_numOfUsers = users;
}

/* MU-BAR Trigger */

WifiHEMuBarTriggerMacHeader::WifiHEMuBarTriggerMacHeader()
{
}

WifiHEMuBarTriggerMacHeader::~WifiHEMuBarTriggerMacHeader()
{
}

TypeId
WifiHEMuBarTriggerMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiHEMuBarTriggerMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiHEMuBarTriggerMacHeader> ()
  ;
  return tid;
}

TypeId
WifiHEMuBarTriggerMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiHEMuBarTriggerMacHeader::Print (std::ostream &os) const
{
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      os << "WifiHeMuRtsHeader:Print Addreess: " << j->first
         << "aid :" << j->second.m_aid << "RU Index :"
         << j->second.index << ", MCS=" << j->second.mcs
         << ", coding type=" << j->second.codingType << ", DCM="
         << j->second.dcm << ", SS Allocation=" << j->second.ssAllocation
         << ", m_baAckPolicy " << j->second.m_baAckPolicy
         << ", m_multiTid " << j->second.m_multiTid
         << ", m_compressed " << j->second.m_compressed
         << ", m_tidInfo " << j->second.m_tidInfo;
  }
  return;
}

uint32_t
WifiHEMuBarTriggerMacHeader::GetSize (void) const
{
  uint32_t size = 0;

  // Each User Info is 5 + 2 bytes * number of User Info.
  size = 7 * staRuMapInfo.size();

  return size;
}

uint32_t
WifiHEMuBarTriggerMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

void
WifiHEMuBarTriggerMacHeader::Serialize (Buffer::Iterator i) const
{
  uint32_t value = 0;  
  uint8_t  reserved = 0;
  uint16_t  barControl = 0;

  // User Info
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      value = 0;
      // Get Aid from MacAddress - 12 bits.
      value |= (j->second.m_aid) & (0xfff);
      // RU Allocation - 8 bits
      value |= (j->second.index << 12) & (0xff << 12);
      // Coding Type - 1 bit
      value |= (j->second.codingType << 20) & (0x1 << 20);
      // MCS  - 4 bits
      value |= (j->second.mcs << 21) & (0xf << 21);
      // DCM - 1 bit
      value |= (j->second.dcm << 25) & (0x1 << 25);
      // SS Allocation - 6 bits
      value |= (j->second.ssAllocation << 26) & (0x3f << 26);

      i.WriteHtolsbU32 (value);
      i.WriteU8(reserved);   //Reserved one byte

      if (j->second.m_baAckPolicy)
      {
          barControl |= 0x1;
      }
      if (j->second.m_multiTid)
      {
          barControl |= (0x1 << 1);
      }
      if (j->second.m_compressed)
      {
          barControl |= (0x1 << 2);
      }
      barControl |= (j->second.m_tidInfo << 12) & (0xf << 12);

      i.WriteHtolsbU16(barControl);
  }
  
  return;
}

uint32_t
WifiHEMuBarTriggerMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t j = 0;
  RuInfo ruInfo;
  Mac48Address  macAddress;
  uint32_t value = 0, valueTmp = 0;  
  uint8_t  reserved = 0;
 
  staRuMapInfo.clear();
  for(j = 0; j < m_numOfUsers; j ++) {
      value = 0;
      valueTmp = 0;
      value = i.ReadLsbtohU32 ();

      valueTmp = value;
      ruInfo.m_aid = (valueTmp) & (0xfff);
      valueTmp = value;
      ruInfo.index = (valueTmp >> 12) & (0xff);
      valueTmp = value;
      ruInfo.codingType = (valueTmp >> 20) & (0x1);
      valueTmp = value;
      ruInfo.mcs = (valueTmp >> 21) & (0xf);
      valueTmp = value;
      ruInfo.dcm = (valueTmp >> 25) & (0x1);
      valueTmp = value;
      ruInfo.ssAllocation = (valueTmp >> 26) & (0x3f);
      
      reserved = 0;
      reserved = i.ReadU8 ();
      if (reserved) {
      }

      value = 0;
      value = i.ReadU16 ();

      valueTmp = value;
      ruInfo.m_baAckPolicy = (valueTmp) & (0x1);
      valueTmp = value;
      ruInfo.m_multiTid = (valueTmp >> 1) & (0x1);
      valueTmp = value;
      ruInfo.m_compressed = (valueTmp >> 3) & (0x1);
      valueTmp = value;
      ruInfo.m_tidInfo = (valueTmp >> 12) & (0xf);

      staRuMapInfo.insert (std::make_pair (ruInfo.m_aid, ruInfo));
  }

  return i.GetDistanceFrom (start);
}

void 
WifiHEMuBarTriggerMacHeader::SetNumOfUsers (uint8_t users)
{
  m_numOfUsers = users;
}

/* BSRP */

WifiHEBsrpMacHeader::WifiHEBsrpMacHeader()
{
}

WifiHEBsrpMacHeader::~WifiHEBsrpMacHeader()
{
}

void
WifiHEBsrpMacHeader::SetStaRuMap(staRuMap staMap) 
{
    staRuMapInfo = staMap;
}

TypeId
WifiHEBsrpMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiHEBsrpMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiHEBsrpMacHeader> ()
  ;
  return tid;
}

TypeId
WifiHEBsrpMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiHEBsrpMacHeader::Print (std::ostream &os) const
{
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      os << "WifiHeBsrpMacHeader:Print Addreess: " << j->first
         << "aid :" << j->second.m_aid << "RU Index :"
         << j->second.index << ", MCS=" << j->second.mcs
         << ", coding type=" << j->second.codingType << ", DCM="
         << j->second.dcm << ", SS Allocation=" << j->second.ssAllocation;
  }
  return;
}

uint32_t
WifiHEBsrpMacHeader::GetSize (void) const
{
  uint32_t size = 0;

  // Each User Info is 5 bytes * number of User Info.
  size = 5 * staRuMapInfo.size();

  return size;
}

uint32_t
WifiHEBsrpMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

staRuMap 
WifiHEBsrpMacHeader::GetRuMap()
{
   return staRuMapInfo;
}

void
WifiHEBsrpMacHeader::Serialize (Buffer::Iterator i) const
{
  uint32_t value = 0;  
  uint8_t  reserved = 0;

  // User Info
  for (staRuMap::const_iterator j = staRuMapInfo.begin (); j != staRuMapInfo.end (); j ++)
  {
      value = 0;
      // Get Aid from MacAddress - 12 bits.
      value |= (j->second.m_aid) & (0xfff);
      // RU Allocation - 8 bits
      value |= (j->second.index << 12) & (0xff << 12);
      // Coding Type - 1 bit
      value |= (j->second.codingType << 20) & (0x1 << 20);
      // MCS  - 4 bits
      value |= (j->second.mcs << 21) & (0xf << 21);
      // DCM - 1 bit
      value |= (j->second.dcm << 25) & (0x1 << 25);
      // SS Allocation - 6 bits
      value |= (j->second.ssAllocation << 26) & (0x3f << 26);

      i.WriteHtolsbU32 (value);
      i.WriteU8(reserved);   //Reserved one byte
  }
  
  return;
}

uint32_t
WifiHEBsrpMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t j = 0;
  RuInfo ruInfo;
  Mac48Address  macAddress;
  uint32_t value = 0, valueTmp = 0;  
 
  staRuMapInfo.clear();
  for(j = 0; j < m_numOfUsers; j ++) {
      value = 0;
      valueTmp = 0;
      value = i.ReadLsbtohU32 ();

      valueTmp = value;
      ruInfo.m_aid = (valueTmp) & (0xfff);

      valueTmp = value;
      ruInfo.index = (valueTmp >> 12) & (0xff);

      valueTmp = value;
      ruInfo.codingType = (valueTmp >> 20) & (0x1);
      valueTmp = value;
      ruInfo.mcs = (valueTmp >> 21) & (0xf);
      valueTmp = value;
      ruInfo.dcm = (valueTmp >> 25) & (0x1);
      valueTmp = value;
      ruInfo.ssAllocation = (valueTmp >> 26) & (0x3f);

      i.ReadU8 ();

      staRuMapInfo.insert (std::make_pair (ruInfo.m_aid, ruInfo));
  }

  return i.GetDistanceFrom (start);
}

void 
WifiHEBsrpMacHeader::SetNumOfUsers (uint8_t users)
{
  m_numOfUsers = users;
}


} //namespace ns3
