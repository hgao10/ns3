/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

#ifndef IPV4_ARBITRARY_ROUTING_HELPER_H
#define IPV4_ARBITRARY_ROUTING_HELPER_H

#include "ns3/ipv4.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-arbiter-routing.h"

namespace ns3 {

/**
 * \ingroup ipv4Helpers
 *
 * \brief Helper class that adds ns3::Ipv4FastStaticHostRouting objects
 *
 * This class is expected to be used in conjunction with 
 * ns3::InternetStackHelper::SetRoutingHelper
 */
class Ipv4ArbiterRoutingHelper : public Ipv4RoutingHelper
{
public:
  /*
   * Construct an Ipv4ArbiterRoutingHelper object, used to make configuration
   * of static routing easier.
   */
  Ipv4ArbiterRoutingHelper ();

  /**
   * \brief Construct an Ipv4ArbiterRoutingHelper from another previously
   * initialized instance (Copy Constructor).
   */
  Ipv4ArbiterRoutingHelper (const Ipv4ArbiterRoutingHelper &);

  /**
   * \returns pointer to clone of this Ipv4ArbiterRoutingHelper
   *
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  Ipv4ArbiterRoutingHelper* Copy (void) const;

  /**
   * \param node the node on which the routing protocol will run
   * \returns a newly-created routing protocol
   *
   * This method will be called by ns3::InternetStackHelper::Install
   */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

private:
  /**
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   * \returns
   */
  Ipv4ArbiterRoutingHelper &operator = (const Ipv4ArbiterRoutingHelper &);
};

} // namespace ns3

#endif /* IPV4_ARBITRARY_ROUTING_HELPER_H */
