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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
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

#include "ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "wifi-phy.h"
#include "dcf-manager.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "mgt-headers.h"
#include "mac-low.h"
#include "amsdu-subframe-header.h"
#include "msdu-aggregator.h"
#include "msdu-standard-aggregator.h"
#include "mpdu-standard-aggregator.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ApWifiMac");

NS_OBJECT_ENSURE_REGISTERED (ApWifiMac);

TypeId
ApWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ApWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ApWifiMac> ()
    .AddAttribute ("BeaconInterval",
                   "Delay between two beacons",
                   TimeValue (MicroSeconds (102400)),
                   MakeTimeAccessor (&ApWifiMac::GetBeaconInterval,
                                     &ApWifiMac::SetBeaconInterval),
                   MakeTimeChecker ())
    .AddAttribute ("BSSColor",
                   "BSS color of this AP",
                   UintegerValue (0x0),
                   MakeUintegerAccessor(&ApWifiMac::m_color),
		   MakeUintegerChecker <uint8_t> ())
    .AddAttribute ("BeaconJitter",
                   "A uniform random variable to cause the initial beacon starting time (after simulation time 0) "
                   "to be distributed between 0 and the BeaconInterval.",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&ApWifiMac::m_beaconJitter),
                   MakePointerChecker<UniformRandomVariable> ())
    .AddAttribute ("EnableBeaconJitter",
                   "If beacons are enabled, whether to jitter the initial send event.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&ApWifiMac::m_enableBeaconJitter),
                   MakeBooleanChecker ())
    .AddAttribute ("BeaconGeneration",
                   "Whether or not beacons are generated.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::SetBeaconGeneration,
                                        &ApWifiMac::GetBeaconGeneration),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonErpProtection", "Whether or not protection mechanism should be used when non-ERP STAs are present within the BSS."
                   "This parameter is only used when ERP is supported by the AP.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::m_enableNonErpProtection),
                   MakeBooleanChecker ())
  ;
  return tid;
}

ApWifiMac::ApWifiMac ()
{
  NS_LOG_FUNCTION (this);
  m_beaconDca = CreateObject<DcaTxop> ();
  m_beaconDca->SetAifsn (1);
  m_beaconDca->SetMinCw (0);
  m_beaconDca->SetMaxCw (0);
  m_beaconDca->SetLow (m_low);
  m_beaconDca->SetManager (m_dcfManager);
  m_beaconDca->SetTxMiddle (m_txMiddle);

  //Let the lower layers know that we are acting as an AP.
  SetTypeOfStation (AP);

  m_enableBeaconGeneration = false;
}

Ptr<MacLow>
ApWifiMac::GetMacLow(void) const
{
  return m_low;
}

ApWifiMac::~ApWifiMac ()
{
  NS_LOG_FUNCTION (this);
  m_staList.clear();
  m_nonErpStations.clear ();
  m_nonHtStations.clear ();
}

void
ApWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_beaconDca = 0;
  m_enableBeaconGeneration = false;
  m_beaconEvent.Cancel ();
  RegularWifiMac::DoDispose ();
}

void
ApWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  //As an AP, our MAC address is also the BSSID. Hence we are
  //overriding this function and setting both in our parent class.
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}

void
ApWifiMac::SetBeaconGeneration (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  if (!enable)
    {
      m_beaconEvent.Cancel ();
    }
  else if (enable && !m_enableBeaconGeneration)
    {
      m_beaconEvent = Simulator::ScheduleNow (&ApWifiMac::SendOneBeacon, this);
    }
  m_enableBeaconGeneration = enable;
}

bool
ApWifiMac::GetBeaconGeneration (void) const
{
  NS_LOG_FUNCTION (this);
  return m_enableBeaconGeneration;
}

Time
ApWifiMac::GetBeaconInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beaconInterval;
}

void
ApWifiMac::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_beaconDca->SetWifiRemoteStationManager (stationManager);
  RegularWifiMac::SetWifiRemoteStationManager (stationManager);
  if (m_heSupported)
    {
      m_stationManager->SetupDcfManager(m_dcfManager);
      //Lets setup the broadcast queue here for OFDMA access
      SetupStationQueue(0,Mac48Address::GetBroadcast());
    }
}

void
ApWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  //The approach taken here is that, from the point of view of an AP,
  //the link is always up, so we immediately invoke the callback if
  //one is set
  linkUp ();
  m_phy->SetColor(m_color);
}

void
ApWifiMac::SetBeaconInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  if ((interval.GetMicroSeconds () % 1024) != 0)
    {
      NS_LOG_WARN ("beacon interval should be multiple of 1024us (802.11 time unit), see IEEE Std. 802.11-2012");
    }
  m_beaconInterval = interval;
}

void
ApWifiMac::StartBeaconing (void)
{
  NS_LOG_FUNCTION (this);
  SendOneBeacon ();
}

int64_t
ApWifiMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_beaconJitter->SetStream (stream);
  return 1;
}

bool
ApWifiMac::GetShortSlotTimeEnabled (void) const
{
  if (m_nonErpStations.size () != 0)
    {
      return false;
    }
  if (m_erpSupported == true && GetShortSlotTimeSupported () == true)
    {
      for (std::list<Mac48Address>::const_iterator i = m_staList.begin (); i != m_staList.end (); i++)
      {
        if (m_stationManager->GetShortSlotTimeSupported (*i) == false)
          {
            return false;
          }
      }
      return true;
    }
  return false;
}

bool
ApWifiMac::GetShortPreambleEnabled (void) const
{
  if (m_erpSupported || m_phy->GetShortPlcpPreambleSupported ())
    {
      for (std::list<Mac48Address>::const_iterator i = m_nonErpStations.begin (); i != m_nonErpStations.end (); i++)
      {
        if (m_stationManager->GetShortPreambleSupported (*i) == false)
          {
            return false;
          }
      }
      return true;
    }
  return false;
}

void
ApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from,
                        Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  //If we are not a QoS AP then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  //If we are a QoS AP then we attempt to get a TID for this packet
  if (m_qosSupported)
    {
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which'll
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
    }

  ForwardDown (packet, from, to, tid);
}

void
ApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from,
                        Mac48Address to, uint8_t tid)
{
  NS_LOG_FUNCTION (this << packet << from << to << static_cast<uint32_t> (tid));
  WifiMacHeader hdr;

  //For now, an AP that supports QoS does not support non-QoS
  //associations, and vice versa. In future the AP model should
  //support simultaneously associated QoS and non-QoS STAs, at which
  //point there will need to be per-association QoS state maintained
  //by the association state machine, and consulted here.
  if (m_qosSupported)
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      //Transmission of multiple frames in the same Polled TXOP is not supported for now
      hdr.SetQosTxopLimit (0);
      //Fill in the QoS control field in the MAC header
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetTypeData ();
    }

  if (m_htSupported || m_vhtSupported || m_heSupported)
    {
      hdr.SetNoOrder ();
    }
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (from);
  hdr.SetDsFrom ();
  hdr.SetDsNotTo ();

  if (m_qosSupported)
    {
      //Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      if (m_heSupported)
	{
	  std::map<uint16_t, EdcaStaQueues>::iterator it = m_ofdmaMap.find(GetAid(to));
	  if(it != m_ofdmaMap.end())
	    {
	      it->second[QosUtilsMapTidToAc (tid)]->Queue(packet, hdr);
	    }
	  else
	    {
	      NS_LOG_ERROR("No Queue exits for the station");
	    }
	}
      else
	{
	  m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
	}
    }
  else
    {
      m_dca->Queue (packet, hdr);
    }
}

void
ApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << to << from);
  if (to.IsBroadcast () || m_stationManager->IsAssociated (to))
    {
      ForwardDown (packet, from, to);
    }
}

void
ApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  //We're sending this packet with a from address that is our own. We
  //get that address from the lower MAC and make use of the
  //from-spoofing Enqueue() method to avoid duplicated code.
  Enqueue (packet, to, m_low->GetAddress ());
}

bool
ApWifiMac::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

SupportedRates
ApWifiMac::GetSupportedRates (void) const
{
  NS_LOG_FUNCTION (this);
  SupportedRates rates;
  //If it is an HT-AP or VHT-AP, then add the BSSMembershipSelectorSet
  //The standard says that the BSSMembershipSelectorSet
  //must have its MSB set to 1 (must be treated as a Basic Rate)
  //Also the standard mentioned that at least 1 element should be included in the SupportedRates the rest can be in the ExtendedSupportedRates
  if (m_htSupported || m_vhtSupported || m_heSupported)
    {
      for (uint32_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          rates.AddBssMembershipSelectorRate (m_phy->GetBssMembershipSelector (i));
        }
    }
  // 
  uint8_t nss = 1;  // Number of spatial streams is 1 for non-MIMO modes
  //Send the set of supported rates and make sure that we indicate
  //the Basic Rate set in this set of supported rates.
  for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
    {
      WifiMode mode = m_phy->GetMode (i);
      uint64_t modeDataRate = mode.GetDataRate (m_phy->GetChannelWidth (), false, nss);
      NS_LOG_DEBUG ("Adding supported rate of " << modeDataRate);
      rates.AddSupportedRate (modeDataRate);
      //Add rates that are part of the BSSBasicRateSet (manufacturer dependent!)
      //here we choose to add the mandatory rates to the BSSBasicRateSet,
      //except for 802.11b where we assume that only the non HR-DSSS rates are part of the BSSBasicRateSet
      if (mode.IsMandatory () && (mode.GetModulationClass () != WIFI_MOD_CLASS_HR_DSSS))
        {
          NS_LOG_DEBUG ("Adding basic mode " << mode.GetUniqueName ());
          m_stationManager->AddBasicMode (mode);
        }
    }
  //set the basic rates
  for (uint32_t j = 0; j < m_stationManager->GetNBasicModes (); j++)
    {
      WifiMode mode = m_stationManager->GetBasicMode (j);
      uint64_t modeDataRate = mode.GetDataRate (m_phy->GetChannelWidth (), false, nss);
      NS_LOG_DEBUG ("Setting basic rate " << mode.GetUniqueName ());
      rates.SetBasicRate (modeDataRate);
    }

  return rates;
}

DsssParameterSet
ApWifiMac::GetDsssParameterSet (void) const
{
  DsssParameterSet dsssParameters;
  if (m_dsssSupported)
    {
      dsssParameters.SetDsssSupported (1);
      dsssParameters.SetCurrentChannel (m_phy->GetChannelNumber ());
    }
  return dsssParameters;
}

CapabilityInformation
ApWifiMac::GetCapabilities (void) const
{
  CapabilityInformation capabilities;
  capabilities.SetShortPreamble (GetShortPreambleEnabled ());
  capabilities.SetShortSlotTime (GetShortSlotTimeEnabled ());
  return capabilities;
}

ErpInformation
ApWifiMac::GetErpInformation (void) const
{
  ErpInformation information;
  information.SetErpSupported (1);
  if (m_erpSupported)
    {
      information.SetNonErpPresent (!m_nonErpStations.empty ());
      information.SetUseProtection (GetUseNonErpProtection ());
      if (GetShortPreambleEnabled ())
        {
          information.SetBarkerPreambleMode (0);
        }
      else
        {
          information.SetBarkerPreambleMode (1);
        }
    }
  return information;
}

EdcaParameterSet
ApWifiMac::GetEdcaParameterSet (void) const
{
  EdcaParameterSet edcaParameters;
  edcaParameters.SetQosSupported (1);
  if (m_qosSupported)
    {
      Ptr<EdcaTxopN> edca;
      Time txopLimit;

      edca = m_edca.find (AC_BE)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters.SetBeAci(0);
      edcaParameters.SetBeCWmin(edca->GetMinCw ());
      edcaParameters.SetBeCWmax(edca->GetMaxCw ());
      edcaParameters.SetBeAifsn(edca->GetAifsn ());
      edcaParameters.SetBeTXOPLimit(txopLimit.GetMicroSeconds () / 32);

      edca = m_edca.find (AC_BK)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters.SetBkAci(1);
      edcaParameters.SetBkCWmin(edca->GetMinCw ());
      edcaParameters.SetBkCWmax(edca->GetMaxCw ());
      edcaParameters.SetBkAifsn(edca->GetAifsn ());
      edcaParameters.SetBkTXOPLimit(txopLimit.GetMicroSeconds () / 32);
      
      edca = m_edca.find (AC_VI)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters.SetViAci(2);
      edcaParameters.SetViCWmin(edca->GetMinCw ());
      edcaParameters.SetViCWmax(edca->GetMaxCw ());
      edcaParameters.SetViAifsn(edca->GetAifsn ());
      edcaParameters.SetViTXOPLimit(txopLimit.GetMicroSeconds () / 32);
      
      edca = m_edca.find (AC_VO)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters.SetVoAci(3);
      edcaParameters.SetVoCWmin(edca->GetMinCw ());
      edcaParameters.SetVoCWmax(edca->GetMaxCw ());
      edcaParameters.SetVoAifsn(edca->GetAifsn ());
      edcaParameters.SetVoTXOPLimit(txopLimit.GetMicroSeconds () / 32);
    }
  return edcaParameters;
}

HtOperations
ApWifiMac::GetHtOperations (void) const
{
  HtOperations operations;
  operations.SetHtSupported (1);
  if (m_htSupported)
    {
      if (!m_nonHtStations.empty ())
        {
          operations.SetHtProtection (MIXED_MODE_PROTECTION);
        }
      else
        {
          operations.SetHtProtection (NO_PROTECTION);
        }
    }
  return operations;
}

void
ApWifiMac::SendProbeResp (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetProbeResp ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeResponseHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetSupportedRates (GetSupportedRates ());
  probe.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());
  probe.SetCapabilities (GetCapabilities ());
  m_stationManager->SetShortPreambleEnabled (GetShortPreambleEnabled ());
  m_stationManager->SetShortSlotTimeEnabled (GetShortSlotTimeEnabled ());
  if (m_dsssSupported)
    {
      probe.SetDsssParameterSet (GetDsssParameterSet ());
    }
  if (m_erpSupported)
    {
      probe.SetErpInformation (GetErpInformation ());
    }
  if (m_qosSupported)
    {
      probe.SetEdcaParameterSet (GetEdcaParameterSet ());
    }
  if (m_htSupported || m_vhtSupported || m_heSupported)
    {
      probe.SetHtCapabilities (GetHtCapabilities ());
      probe.SetHtOperations (GetHtOperations ());
      hdr.SetNoOrder ();
    }
  if (m_vhtSupported)
    {
      probe.SetVhtCapabilities (GetVhtCapabilities ());
    }
  if (m_heSupported)
    {

    }
  packet->AddHeader (probe);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

uint16_t
ApWifiMac::GetAid(Mac48Address mac)
{
  uint16_t aid = 0;
  std::map<Mac48Address, uint16_t>::iterator it;
  it = m_aidMap.find(mac);
  if(it != m_aidMap.end())
    {
      aid=it->second;
    }
  else
    {

     // aid = AllocateAid(mac); ///XXX: this is temp code till beacon exits
     // SetupStationQueue(aid, mac); //XXX: as above
     // NS_FATAL_ERROR("No map between Mac and Aid exists");
    }
  return aid;
}

uint16_t
ApWifiMac::AllocateAid(Mac48Address mac)
{
  static uint16_t aid = 0;
  std::map<Mac48Address, uint16_t>::iterator it;
  it = m_aidMap.find(mac);
  if(it != m_aidMap.end())
    {
      NS_LOG_UNCOND("Reassociated mac : " << mac << " Aid : " << it->second);
      return it->second;
    }
  aid++;
  m_aidMap.insert(std::make_pair(mac, aid));
  SetupStationQueue(aid, mac);
  NS_LOG_UNCOND("AllocateAid mac : " << mac << " Aid : " << aid);
  return aid;
}

Ptr<EdcaTxopN>
ApWifiMac::SetupStationEdcaQueue (uint16_t aid, Mac48Address mac, enum AcIndex ac)
{
  Ptr<EdcaTxopN> edca = CreateObject<EdcaTxopN> ();
  edca->Initialize();
  edca->SetTypeOfStation(AP);   // Just to denote that this edca resides in AP
  edca->SetHeSupported(true);
  edca->SetAid(mac, aid);
  edca->SetLow (m_low);
  edca->SetManager (m_dcfManager);
  edca->SetTxMiddle (m_txMiddle);
  edca->SetTxOkCallback (MakeCallback (&ApWifiMac::TxOk, this));
  edca->SetTxFailedCallback (MakeCallback (&ApWifiMac::TxFailed, this));
  edca->SetAccessCategory (ac);
  edca->SetWifiRemoteStationManager(m_stationManager);
  edca->CompleteConfig ();

  Ptr<MsduStandardAggregator> msduAggregator = CreateObject<MsduStandardAggregator> ();
  edca->SetMsduAggregator (msduAggregator);
  Ptr<MpduStandardAggregator> mpduAggregator = CreateObject<MpduStandardAggregator> ();
  edca->SetMpduAggregator (mpduAggregator);
#if 0
  if (ac == AC_VO)
    {
      edca->GetMsduAggregator ()->SetMaxAmsduSize (m_voMaxAmsduSize);
      edca->GetMpduAggregator ()->SetMaxAmpduSize (m_voMaxAmpduSize);
    }
  else if (ac == AC_VI)
    {
      edca->GetMsduAggregator ()->SetMaxAmsduSize (m_viMaxAmsduSize);
      edca->GetMpduAggregator ()->SetMaxAmpduSize (m_viMaxAmpduSize);
    }
  else if (ac == AC_BE)
    {
      edca->GetMsduAggregator ()->SetMaxAmsduSize (m_beMaxAmsduSize);
      edca->GetMpduAggregator ()->SetMaxAmpduSize (m_beMaxAmpduSize);
    }
  else
    {
      edca->GetMsduAggregator ()->SetMaxAmsduSize (m_bkMaxAmsduSize);
      edca->GetMpduAggregator ()->SetMaxAmpduSize (m_bkMaxAmpduSize);
    }
#endif
  return edca;
}

void
ApWifiMac::SetupStationQueue(uint16_t aid, Mac48Address mac)
{
  EdcaStaQueues edcaList;
  edcaList.push_back(SetupStationEdcaQueue (aid, mac, AC_BE));
  edcaList.push_back(SetupStationEdcaQueue (aid, mac, AC_BK));
  edcaList.push_back(SetupStationEdcaQueue (aid, mac, AC_VI));
  edcaList.push_back(SetupStationEdcaQueue (aid, mac, AC_VO));
  m_ofdmaMap.insert(std::make_pair(aid, edcaList));
}

void
ApWifiMac::SendAssocResp (Mac48Address to, bool success)
{
  NS_LOG_FUNCTION (this << to << success);
  WifiMacHeader hdr;
  hdr.SetAssocResp ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtAssocResponseHeader assoc;
  StatusCode code;
  if (success)
    {
      uint16_t aid = AllocateAid(to);
      code.SetSuccess ();
      m_staList.push_back (to);
      assoc.SetAid(aid);
      assoc.SetColor(m_color);
    }
  else
    {
      code.SetFailure ();
    }
  assoc.SetSupportedRates (GetSupportedRates ());
  assoc.SetStatusCode (code);
  assoc.SetCapabilities (GetCapabilities ());
  if (m_erpSupported)
    {
      assoc.SetErpInformation (GetErpInformation ());
    }
  if (m_qosSupported)
    {
      assoc.SetEdcaParameterSet (GetEdcaParameterSet ());
    }
  if (m_htSupported || m_vhtSupported)
    {
      assoc.SetHtCapabilities (GetHtCapabilities ());
      assoc.SetHtOperations (GetHtOperations ());
      hdr.SetNoOrder ();
    }
  if (m_vhtSupported)
    {
      assoc.SetVhtCapabilities (GetVhtCapabilities ());
    }
  if(m_heSupported)
    {

    }
  packet->AddHeader (assoc);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

void
ApWifiMac::SendOneBeacon (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetBeacon ();
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtBeaconHeader beacon;
  beacon.SetSsid (GetSsid ());
  beacon.SetSupportedRates (GetSupportedRates ());
  beacon.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());
  beacon.SetCapabilities (GetCapabilities ());
  m_stationManager->SetShortPreambleEnabled (GetShortPreambleEnabled ());
  m_stationManager->SetShortSlotTimeEnabled (GetShortSlotTimeEnabled ());
  if (m_dsssSupported)
    {
      beacon.SetDsssParameterSet (GetDsssParameterSet ());
    }
  if (m_erpSupported)
    {
      beacon.SetErpInformation (GetErpInformation ());
    }
  if (m_qosSupported)
    {
      beacon.SetEdcaParameterSet (GetEdcaParameterSet ());
    }
  if (m_htSupported || m_vhtSupported)
    {
      beacon.SetHtCapabilities (GetHtCapabilities ());
      beacon.SetHtOperations (GetHtOperations ());
      hdr.SetNoOrder ();
    }
  if (m_vhtSupported)
    {
      beacon.SetVhtCapabilities (GetVhtCapabilities ());
    }
  if(m_heSupported)
    {

    }
  packet->AddHeader (beacon);

  //The beacon has it's own special queue, so we load it in there
  m_beaconDca->Queue (packet, hdr);
  m_beaconEvent = Simulator::Schedule (m_beaconInterval, &ApWifiMac::SendOneBeacon, this);
  
  //If a STA that does not support Short Slot Time associates,
  //the AP shall use long slot time beginning at the first Beacon
  //subsequent to the association of the long slot time STA.
  if (m_erpSupported)
    {
    if (GetShortSlotTimeEnabled () == true)
      {
        //Enable short slot time
        SetSlot (MicroSeconds (9));
      }
    else
      {
        //Disable short slot time
        SetSlot (MicroSeconds (20));
      }
    }
}

void
ApWifiMac::TxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxOk (hdr);

  if (hdr.IsAssocResp ()
      && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("associated with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxOk (hdr.GetAddr1 ());
    }
}

void
ApWifiMac::TxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxFailed (hdr);

  if (hdr.IsAssocResp ()
      && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("assoc failed with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxFailed (hdr.GetAddr1 ());
    }
}

void
ApWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);

  Mac48Address from = hdr->GetAddr2 ();

  if (hdr->IsData ())
    {
      Mac48Address bssid = hdr->GetAddr1 ();
      if (!hdr->IsFromDs ()
          && hdr->IsToDs ()
          && bssid == GetAddress ()
          && m_stationManager->IsAssociated (from))
        {
          Mac48Address to = hdr->GetAddr3 ();
          if (to == GetAddress ())
            {
              NS_LOG_DEBUG ("frame for me from=" << from);
              if (hdr->IsQosData ())
                {
                  if (hdr->IsQosAmsdu ())
                    {
                      NS_LOG_DEBUG ("Received A-MSDU from=" << from << ", size=" << packet->GetSize ());
                      DeaggregateAmsduAndForward (packet, hdr);
                      packet = 0;
                    }
                  else
                    {
                      ForwardUp (packet, from, bssid);
                    }
                }
              else
                {
                  ForwardUp (packet, from, bssid);
                }
            }
          else if (to.IsGroup ()
                   || m_stationManager->IsAssociated (to))
            {
              NS_LOG_DEBUG ("forwarding frame from=" << from << ", to=" << to);
              Ptr<Packet> copy = packet->Copy ();

              //If the frame we are forwarding is of type QoS Data,
              //then we need to preserve the UP in the QoS control
              //header...
              if (hdr->IsQosData ())
                {
                  ForwardDown (packet, from, to, hdr->GetQosTid ());
                }
              else
                {
                  ForwardDown (packet, from, to);
                }
              ForwardUp (copy, from, to);
            }
          else
            {
              ForwardUp (packet, from, to);
            }
        }
      else if (hdr->IsFromDs ()
               && hdr->IsToDs ())
        {
          //this is an AP-to-AP frame
          //we ignore for now.
          NotifyRxDrop (packet);
        }
      else
        {
          //we can ignore these frames since
          //they are not targeted at the AP
          NotifyRxDrop (packet);
        }
      return;
    }
  else if (hdr->IsMgt ())
    {
      if (hdr->IsProbeReq ())
        {
          NS_ASSERT (hdr->GetAddr1 ().IsBroadcast ());
          SendProbeResp (from);
          return;
        }
      else if (hdr->GetAddr1 () == GetAddress ())
        {
          if (hdr->IsAssocReq ())
            {
              //first, verify that the the station's supported
              //rate set is compatible with our Basic Rate set
              MgtAssocRequestHeader assocReq;
              packet->RemoveHeader (assocReq);
              CapabilityInformation capabilities = assocReq.GetCapabilities ();
              m_stationManager->AddSupportedPlcpPreamble (from, capabilities.IsShortPreamble ());
              SupportedRates rates = assocReq.GetSupportedRates ();
              bool problem = false;
              bool isHtStation = false;
              bool isOfdmStation = false;
              bool isErpStation = false;
              bool isDsssStation = false;
              for (uint32_t i = 0; i < m_stationManager->GetNBasicModes (); i++)
                {
                  WifiMode mode = m_stationManager->GetBasicMode (i);
                  uint8_t nss = 1; // Assume 1 spatial stream in basic mode
                  if (!rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth (), false, nss)))
                    {
                      if ((mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
                        {
                          isDsssStation = false;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
                        {
                          isErpStation = false;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM)
                        {
                          isOfdmStation = false;
                        }
                      if (isDsssStation == false && isErpStation == false && isOfdmStation == false)
                        {
                          problem = true;
                          break;
                        }
                    }
                  else
                    {
                      if ((mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
                        {
                          isDsssStation = true;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
                        {
                          isErpStation = true;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM)
                        {
                          isOfdmStation = true;
                        }
                    }
                }
              m_stationManager->AddSupportedErpSlotTime (from, capabilities.IsShortSlotTime () && isErpStation);
              if (m_htSupported)
                {
                  //check whether the HT STA supports all MCSs in Basic MCS Set
                  HtCapabilities htcapabilities = assocReq.GetHtCapabilities ();
                  if (htcapabilities.GetHtCapabilitiesInfo () != 0)
                    {
                      isHtStation = true;
                      for (uint32_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!htcapabilities.IsSupportedMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (m_vhtSupported)
                {
                  //check whether the VHT STA supports all MCSs in Basic MCS Set
                  VhtCapabilities vhtcapabilities = assocReq.GetVhtCapabilities ();
                  if (vhtcapabilities.GetVhtCapabilitiesInfo () != 0)
                    {
                      for (uint32_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!vhtcapabilities.IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (m_heSupported)
        	{

        	}
              if (problem)
                {
                  //One of the Basic Rate set mode is not
                  //supported by the station. So, we return an assoc
                  //response with an error status.
                  SendAssocResp (hdr->GetAddr2 (), false);
                }
              else
                {
                  //station supports all rates in Basic Rate Set.
                  //record all its supported modes in its associated WifiRemoteStation
                  for (uint32_t j = 0; j < m_phy->GetNModes (); j++)
                    {
                      WifiMode mode = m_phy->GetMode (j);
                      uint8_t nss = 1; // Assume 1 spatial stream in basic mode
                      if (rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth (), false, nss)))
                        {
                          m_stationManager->AddSupportedMode (from, mode);
                        }
                    }
                  if (m_htSupported)
                    {
                      HtCapabilities htcapabilities = assocReq.GetHtCapabilities ();
                      m_stationManager->AddStationHtCapabilities (from, htcapabilities);
                      for (uint32_t j = 0; j < m_phy->GetNMcs (); j++)
                        {
                          WifiMode mcs = m_phy->GetMcs (j);
                          if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HT && htcapabilities.IsSupportedMcs (mcs.GetMcsValue ()))
                            {
                              m_stationManager->AddSupportedMcs (from, mcs);
                            }
                        }
                    }
                  if (m_vhtSupported)
                    {
                      VhtCapabilities vhtCapabilities = assocReq.GetVhtCapabilities ();
                      m_stationManager->AddStationVhtCapabilities (from, vhtCapabilities);
                      for (uint32_t i = 0; i < m_phy->GetNMcs (); i++)
                        {
                          WifiMode mcs = m_phy->GetMcs (i);
                          if (mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT && vhtCapabilities.IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                              //here should add a control to add basic MCS when it is implemented
                            }
                        }
                    }
                  if (m_heSupported)
                    {

                    }
                  m_stationManager->RecordWaitAssocTxOk (from);
                  if (!isHtStation)
                    {
                      m_nonHtStations.push_back (hdr->GetAddr2 ());
                    }
                  if (!isErpStation && isDsssStation)
                    {
                      m_nonErpStations.push_back (hdr->GetAddr2 ());
                    }
                  // send assoc response with success status.
                  SendAssocResp (hdr->GetAddr2 (), true);
                }
              return;
            }
          else if (hdr->IsDisassociation ())
            {
              m_stationManager->RecordDisassociated (from);
              for (std::list<Mac48Address>::iterator i = m_staList.begin (); i != m_staList.end (); i++)
              {
                if ((*i) == from)
                  {
                    m_staList.erase (i);
                    break;
                  }
              }
              for (std::list<Mac48Address>::iterator j = m_nonErpStations.begin (); j != m_nonErpStations.end (); j++)
              {
                if ((*j) == from)
                  {
                    m_nonErpStations.erase (j);
                    break;
                  }
              }
              for (std::list<Mac48Address>::iterator j = m_nonHtStations.begin (); j != m_nonHtStations.end (); j++)
              {
                if ((*j) == from)
                  {
                    m_nonHtStations.erase (j);
                    break;
                  }
              }
              return;
            }
          if (hdr->IsAction ())
            {
              //There is currently only any reason for Management Action
              //frames to be flying about if we are a QoS STA.
              NS_ASSERT (m_qosSupported);

              WifiActionHeader actionHdr;
              packet->PeekHeader(actionHdr);

              if (actionHdr.GetCategory () == WifiActionHeader::BLOCK_ACK)
                {
        	  if (actionHdr.GetAction ().blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE)
        	    {
            	      packet->RemoveHeader (actionHdr);
            	      MgtAddBaResponseHeader respHdr;
            	      packet->RemoveHeader (respHdr);
            	      //We've received an ADDBA Response. We assume that it
            	      //indicates success after an ADDBA Request we have
            	      //sent (we could, in principle, check this, but it
            	      //seems a waste given the level of the current model)
            	      //and act by locally establishing the agreement on
            	      //the appropriate queue.
            	      AcIndex ac = QosUtilsMapTidToAc (respHdr.GetTid ());
            	      std::map<uint16_t, EdcaStaQueues>::iterator it = m_ofdmaMap.find(GetAid(hdr->GetAddr2 ()));
            	     if (it != m_ofdmaMap.end())
            	       {
            	         it->second[ac]->GotAddBaResponse(&respHdr, from);
            	         //This frame is now completely dealt with, so we're done.
            	         return;
            	       }
            	     packet->AddHeader (respHdr);
            	     packet->AddHeader (actionHdr);
        	    }
        	  else if (actionHdr.GetAction ().blockAck == WifiActionHeader::BLOCK_ACK_DELBA)
        	    {
            	      packet->RemoveHeader (actionHdr);
                      MgtDelBaHeader delBaHdr;
                      packet->RemoveHeader (delBaHdr);

                      if (delBaHdr.IsByOriginator ())
                        {
                          //This DELBA frame was sent by the originator, so
                          //this means that an ingoing established
                          //agreement exists in MacLow and we need to
                          //destroy it.
                          m_low->DestroyBlockAckAgreement (from, delBaHdr.GetTid ());
                        }
                      else
                        {
                          //We must have been the originator. We need to
                          //tell the correct queue that the agreement has
                          //been torn down
                          AcIndex ac = QosUtilsMapTidToAc (delBaHdr.GetTid ());
                          std::map<uint16_t, EdcaStaQueues>::iterator it = m_ofdmaMap.find(GetAid(hdr->GetAddr2 ()));
                          if (it != m_ofdmaMap.end())
                            {
                              it->second[ac]->GotDelBaFrame (&delBaHdr, from);
                              //This frame is now completely dealt with, so we're done.
                              return;
                            }
                        }
                      packet->AddHeader (delBaHdr);
            	      packet->AddHeader (actionHdr);
        	    }
                }
              else
        	{
                  //packet->AddHeader(actionHdr);
        	}
            }
        }
    }
  //Invoke the receive handler of our parent class to deal with any
  //other frames. Specifically, this will handle Block Ack-related
  //Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

void
ApWifiMac::DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket,
                                       const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << aggregatedPacket << hdr);
  MsduAggregator::DeaggregatedMsdus packets =
    MsduAggregator::Deaggregate (aggregatedPacket);

  for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin ();
       i != packets.end (); ++i)
    {
      if ((*i).second.GetDestinationAddr () == GetAddress ())
        {
          ForwardUp ((*i).first, (*i).second.GetSourceAddr (),
                     (*i).second.GetDestinationAddr ());
        }
      else
        {
          Mac48Address from = (*i).second.GetSourceAddr ();
          Mac48Address to = (*i).second.GetDestinationAddr ();
          NS_LOG_DEBUG ("forwarding QoS frame from=" << from << ", to=" << to);
          ForwardDown ((*i).first, from, to, hdr->GetQosTid ());
        }
    }
}

void
ApWifiMac::SendAddBaResponse(const MgtAddBaRequestHeader *reqHdr,
                                   Mac48Address originator)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  MgtAddBaResponseHeader respHdr;
  StatusCode code;
  code.SetSuccess ();
  respHdr.SetStatusCode (code);
  //Here a control about queues type?
  respHdr.SetAmsduSupport (reqHdr->IsAmsduSupported ());

  if (reqHdr->IsImmediateBlockAck ())
    {
      respHdr.SetImmediateBlockAck ();
    }
  else
    {
      respHdr.SetDelayedBlockAck ();
    }
  respHdr.SetTid (reqHdr->GetTid ());
  //For now there's not no control about limit of reception. We
  //assume that receiver has no limit on reception. However we assume
  //that a receiver sets a bufferSize in order to satisfy next
  //equation: (bufferSize + 1) % 16 = 0 So if a recipient is able to
  //buffer a packet, it should be also able to buffer all possible
  //packet's fragments. See section 7.3.1.14 in IEEE802.11e for more details.
  respHdr.SetBufferSize (1023);
  respHdr.SetTimeout (reqHdr->GetTimeout ());

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (respHdr);
  packet->AddHeader (actionHdr);

  //We need to notify our MacLow object as it will have to buffer all
  //correctly received packets for this Block Ack session
  m_low->CreateBlockAckAgreement (&respHdr, originator,
                                  reqHdr->GetStartingSequence ());

  //It is unclear which queue this frame should go into. For now we
  //bung it into the queue corresponding to the TID for which we are
  //establishing an agreement, and push it to the head.
  AcIndex ac = QosUtilsMapTidToAc (respHdr.GetTid ());
  std::map<uint16_t, EdcaStaQueues>::iterator it = m_ofdmaMap.find(GetAid(originator));
  if (it != m_ofdmaMap.end())
     {
      it->second[ac]->PushFront (packet, hdr);
     }
  else
    {
      m_edca[ac]->PushFront (packet, hdr);
    }
}

void
ApWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_beaconDca->Initialize ();
  m_beaconEvent.Cancel ();
  if (m_enableBeaconGeneration)
    {
      if (m_enableBeaconJitter)
        {
          int64_t jitter = m_beaconJitter->GetValue (0, m_beaconInterval.GetMicroSeconds ());
          NS_LOG_DEBUG ("Scheduling initial beacon for access point " << GetAddress () << " at time " << jitter << " microseconds");
          m_beaconEvent = Simulator::Schedule (MicroSeconds (jitter), &ApWifiMac::SendOneBeacon, this);
        }
      else
        {
          NS_LOG_DEBUG ("Scheduling initial beacon for access point " << GetAddress () << " at time 0");
          m_beaconEvent = Simulator::ScheduleNow (&ApWifiMac::SendOneBeacon, this);
        }
    }
  RegularWifiMac::DoInitialize ();
}

bool
ApWifiMac::GetUseNonErpProtection (void) const
{
  bool useProtection = !m_nonErpStations.empty () && m_enableNonErpProtection;
  m_stationManager->SetUseNonErpProtection (useProtection);
  return useProtection;
}

} //namespace ns3
