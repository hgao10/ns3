/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Doreid AMMAR - INRIA RESO team, LIP laboratory
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
 * Author: Sharan Naribole <nsharan@rice.edu>
 * Extended from PPBP Application provided for older ns-3 versions at
 * http://perso.ens-lyon.fr/thomas.begin/NS3-PPBP.zip
 */

#ifndef __PPBP_application_h__
#define __PPBP_application_h__

#include "ns3/address.h"
#include "ns3/string.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traced-callback.h"
#include "ns3/data-rate.h"
#include "ns3/sequence-number.h"

namespace ns3 {

	class Socket;

	/**
	 * \ingroup applications
	 * \defgroup PPBP PPBP-application
	 *
	 * The traffic generator follows a Poisson Pareto Burst Process (PPBP).
	 * This a process based on the overlapping of multiple bursts with
	 * heavy-tailed distributed lengths.
	 * Events in this process represent points of time at which one of an
	 * infinite population of users begins or stops transmitting a traffic burst.
	 * The PPBP is closely related to the M/G/Infinity queue model.
	 */

	/**
	 * \ingroup PPBP
	 *
	 * \brief Generate traffic to a single destination according to a
	 *        Poisson Pareto Burst Process (PPBP).
	 *
	 * The PPBP is a process based on the overlapping of multiple bursts
	 * with heavy-tailed distributed lengths [1], [2].
	 * Events in this process represent points of time at which one of an
	 * infinite population of users begins or stops transmitting a traffic burst.
	 * The PPBP is closely related to the M/G/Infinity queue model.
	 *
	 * In the PPBP model, bursts arrive according to a Poisson process with rate lambda_p,
	 * and their length follows a Pareto distribution characterized by Hurst parameter
	 * H, typically between 0.5 and 0.9, and a mean Ton. Each burst is modeled by a
	 * flow with a constant bit-rate r. Then, the overlapping bursts form in
	 * aggregate a Long-Range Dependent traffic provided the burst length have
	 * infinite variance.
	 *
	 * Summary of parameters:
	 * - - - - - - - - - - -
	 * lambda_p:	mean rate of burst arrivals
	 * Ton:			mean burst time length
	 * r:			Burst intensity
	 *
	 * Based on Littles law, the average number of active bursts is given by:
	 * E[n] = Ton x lambda_p
	 * Since each burst gives birth to a flow with a constant bit-rate, it is then
	 * straightforward to compute the overall rate of the PPBP, lambda:
	 * lambda = Ton x lambda_p x r
	 *
	 * References:
	 * - - - - - -
	 * [1]	A new tool for generating realistic Internet traffic in NS-3,
	 *		D. Ammar, T. Begin and I. Guerin Lassous.
	 *		4th International ICST Conference on Simulation Tools and Techniques (SIMUTools),
	 *		Barcelona, Spain, March 21-25, 2011, Poster.
	 *
	 * [2]	M. Zukerman, T. D. Neame and R. G. Addie,
	 *		Internet Traffic Modeling and Future Technology Implications.
	 *		Proceedings of INFOCOM 2003, San Francisco, USA, April 2003.
	 */

	class PPBPApplication : public Application
	{
	public:
		static TypeId GetTypeId (void);

		PPBPApplication ();

		virtual ~PPBPApplication();

		/**
		 * \brief Return total bytes sent by this object.
		 */
		uint32_t      GetTotalBytes() const;

	protected:
		virtual void DoDispose ();

	private:
		// Inherited from Application base class.
		virtual void StartApplication ();				// Called at time specified by Start
		virtual void StopApplication ();				// Called at time specified by Stop

		// Helpers
		void CancelEvents ();

		// Event handlers
		void StartSending();
		void StopSending();
		void SendPacket();

		Ptr<Socket>     m_socket;						// Associated socket
        Ptr<Socket>     m_receive_socket;         //!< Listening socket

		TypeId          m_protocolTid;					// protocol type id
        Address         m_local;         //!< Local address to bind to

		Address         m_peer;							// Peer address
		bool            m_connected;					// True if connected

		uint32_t        m_totalBytes;					// Total bytes sent so far

		Time            m_lastStartTime;				// Time last packet sent
		EventId         m_startStopEvent;				// Event id for next start or stop event
		EventId         m_sendEvent;					// Event id of pending "send packet" event
		EventId			    m_getUtilization;				// Event id to get the utilization factor
		EventId			    m_PoissonArrival;				// Event id for next burst arrival
		EventId			    m_ppbp;							// Event id for the length of a pending burst arrival
		EventId			    m_ParetoDeparture;				// Event id of burst departure

		uint32_t		    m_pktSize;						// Size of packets

		TracedCallback< Ptr<const Packet> > m_txTrace;	// Trace callback for each sent packet

		double	        m_burstArrivals;				// Mean rate of burst arrivals
		double	        m_burstLength;					// Mean burst time length
		DataRate        m_cbrRate;						// Burst intensity (constant bit-rate)

		double			    m_h;							// Hurst parameter	(Pareto distribution)
		double			    m_shape;						// Shape			(Pareto distribution)
		Time			      m_timeSlot;						// The time slot
		int				      m_activebursts;					// Number of active bursts at time t
		bool			      m_offPeriod;
        std::string         m_baseLogsDir;               //!< Where the flow logs will be written to:
        uint32_t            m_node_id;
		uint64_t			m_totalRx;
	private:
		void ScheduleStartEvent();
		void ScheduleStopEvent();
		void ConnectionSucceeded(Ptr<Socket>);
		void ConnectionFailed(Ptr<Socket>);

		/**
		 * \ Functions that allows to keep track of the current number of active bursts at time t, nt,
		 * taking into account that their arrival process follows a Poisson process and that their
		 * length is determined by a Pareto distribution.
		 */
		void PPBP();
		void PoissonArrival();
		void ParetoDeparture();
		void HandleRead(Ptr<Socket> socket);
		void HandleAccept(Ptr<Socket> socket, const Address &from);
		/**
		 * \ Function thet generates the packets departure at a constant bit-rate nt x r.
		 */
		void ScheduleNextTx();

        void RttChange(Time oldRtt, Time newRtt);
        void CwndChange(uint32_t oldCwnd, uint32_t newCwnd);
        void HighestRxAckChange(SequenceNumber<unsigned int, int> oldHighestRxAck, SequenceNumber<unsigned int, int> newHighestRxAck);

	};

} // namespace ns3
#endif