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
 * Author: Hanjing
 * Adapted from UdpEchoHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef HOROVOD_WORKER_HELPER_H
#define HOROVOD_WORKER_HELPER_H

#include <stdint.h>

#include "ns3/application-container.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

namespace ns3 {

/**
 * \ingroup udprtt
 * \brief Create a server application which waits for input UDP packets
 *        and sends them back to the original sender.
 */
class HorovodWorkerHelper {
 public:
  /**
   * Create HorovodWorkerHelper which will make life easier for people trying
   * to set up simulations with rtts.
   *
   * \param port The port the server will wait on for incoming packets
   */
  HorovodWorkerHelper(std::string protocol, Address local_address,
                      Address remote_address, uint32_t rank,
                      std::string baseLogsDir, uint8_t priority, uint32_t port,
                      uint32_t num_workers, uint32_t fusion_size_bytes, std::string runDir);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute(std::string name, const AttributeValue &value);

  /**
   * Create HorovodWorkerApplication on the specified Node.
   *
   * \param node The node on which to create the Application.  The node is
   *             specified by a Ptr<Node>.
   *
   * \returns An ApplicationContainer holding the Application created,
   */
  ApplicationContainer Install(Ptr<Node> node) const;

  /**
   * Create a HorovodWorkerApplication on specified node
   *
   * \param nodeName The node on which to create the application.  The node
   *                 is specified by a node name previously registered with
   *                 the Object Name Service.
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer Install(std::string nodeName) const;

  /**
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   *
   * Create one udp rtt server application on each of the Nodes in the
   * NodeContainer.
   *
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install(NodeContainer c) const;

 private:
  /**
   * Install an ns3::HorovodWorker on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an HorovodWorker will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv(Ptr<Node> node) const;

  ObjectFactory m_factory;  //!< Object factory.
};

}  // namespace ns3

#endif /* HOROVOD_WORKER_HELPER_H */
