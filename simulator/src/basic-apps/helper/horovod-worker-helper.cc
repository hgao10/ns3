/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Simon
 * Adapted from UdpEchoHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "horovod-worker-helper.h"
#include "ns3/horovod-worker.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/inet-socket-address.h"
#include "ns3/string.h"

namespace ns3 {

HorovodWorkerHelper::HorovodWorkerHelper(std::string protocol, \
                                          Address local_address, \
                                          Address remote_address, \
                                          uint32_t rank,\
                                          std::string baseLogsDir, \
                                          uint8_t priority, \
                                          uint32_t port)
{
//   m_factory.SetTypeId (HorovodWorker::GetTypeId());
  m_factory.SetTypeId ("ns3::HorovodWorker");
  m_factory.Set ("Protocol", StringValue (protocol));
  m_factory.Set ("Local", AddressValue (local_address));
  m_factory.Set ("Remote", AddressValue (remote_address));
  m_factory.Set("Rank", UintegerValue(rank));
  m_factory.Set ("BaseLogsDir", StringValue (baseLogsDir));
  m_factory.Set ("Priority", UintegerValue (priority));
  m_factory.Set ("Port", UintegerValue (port));
    
}

void 
HorovodWorkerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
HorovodWorkerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
HorovodWorkerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
HorovodWorkerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
HorovodWorkerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<HorovodWorker> ();
  node->AddApplication (app);

  return app;
}


} // namespace ns3
