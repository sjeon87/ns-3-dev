/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 * 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 * Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef LTP_CONVERGENCE_LAYER_ADAPTER_H
#define LTP_CONVERGENCE_LAYER_ADAPTER_H

#include "generic-convergence-layer-adapter.h"
#include "ltp-header.h"

#include "ns3/address.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"

#include <map>
#include <queue>
#include <vector>
#include <set>

namespace ns3
{

/**
 * @enum StatusNotificationCode
 * @brief Notices to client service. Defined in RFC-5326 Sections 7.1-7.7.
 */
enum StatusNotificationCode
{
    SESSION_START = 0,      //!< Session has started
    GP_SEGMENT_RCV = 1,     //!< Green part segment received
    RED_PART_RCV = 2,       //!< Red part segment received
    TX_COMPLETED = 3,       //!< Transmission successfully completed
    TX_SESSION_CANCEL = 4,  //!< Transmission session canceled
    RX_SESSION_CANCEL = 5,  //!< Reception session canceled
    SESSION_END = 6         //!< Session has ended
};

/**
 * @enum TimerCode
 * @brief Defines the several timer types used in LTP sessions.
 */
enum TimerCode
{
    CHECKPOINT = 0, //!< Checkpoint Timer code.
    REPORT = 1,     //!< Report Timer code.
    CANCEL = 2      //!< Cancel timer code.
};

/**
 * @enum CancellationState
 * @brief Defines the status and reason of session cancellation.
 */
enum CancellationState
{
    NOT_CANCELED = 0,  //!< Session Active
    REMOTE_CANCEL = 1, //!< Remote LTP engine canceled the session
    LOCAL_CANCEL = 2,  //!< Local LTP engine canceled the session
};

/**
 * @brief Struct that stores specific values from report or checkpoint
 * segments, used for retransmission logic.
 */
struct RedSegmentInfo
{
    uint32_t CpserialNum;
    uint32_t RpserialNum;
    uint32_t low_bound;
    uint32_t high_bound;
    std::set<LtpContentHeader::ReceptionClaim> claims;
};

/**
 * @ingroup dtn
 *
 * @brief Queue set class containing the two queues for outbound traffic.
 * Represents the dual queue structure and priority en/de-queueing policy described in
 * RFC 5325 - 3.1.2 Deferred Transmission.
 */
class LtpQueueSet : public Object
{
  public:
    /**
     * @brief Get Type Id.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief LtpQueueSet Constructor
     *
     * Create a ltp queue pair
     */
    LtpQueueSet();

    /**
     * @brief Destructor
     * Destructor
     */
    virtual ~LtpQueueSet();
    /**
     * Push a packet in the queue set, this method checks the LTP Segment type and enqueues
     * it in the corresponding priority queue.
     * @param p the packet to enqueue
     * @return true if success, false on failure.
     */
    bool Enqueue(Ptr<Packet> p);
    /**
     * Pull a packet from the queue based on priority (internal operation queue packets are
     * extracted first)
     * @return the packet.
     */
    Ptr<Packet> Dequeue(void);
    /**
     * Peek the front packet based on priority
     * @return the packet.
     */
    Ptr<const Packet> Peek(void) const;
    /**
     * Pull a packet from the queue based on priority (internal operation queue packets are
     * extracted first)
     * @return the packet.
     */
    Ptr<Packet> Remove(void);
    /**
     * Get N packets from the queue
     * @return the packet.
     */
    uint32_t GetNPackets(void) const;

    std::queue<Ptr<Packet>> m_internalOps; //!< Internal Operation Queue.
    std::queue<Ptr<Packet>> m_appData;     //!< Application Data Queue.
};

/**
 * @ingroup dtn
 *
 * @brief Class representing active client service instances registered within the LTP protocol.
 *
 * This class is used to keep track of active sessions that are being used by each 
 * client service instance. It contains methods to report the changes on session status.
 */
class ClientServiceStatus : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief Default constructor.
     */
    ClientServiceStatus();

    /**
     * @brief Destructor.
     */
    ~ClientServiceStatus() override;

    /**
     * @brief Reports changes in session state to a client service instance.
     *
     * @param id Session Id.
     * @param code StatusNotificationCode indicating the event type.
     * @param data Data to deliver to the client service instance.
     * @param dataLength Length of the delivered data.
     * @param endFlag Indicates the end of the block/part.
     * @param srcLtpEngine Address of the source LTP Engine.
     * @param offset Offset within the block of the delivered data.
     */
    void ReportStatus(SessionId id,
                      StatusNotificationCode code,
                      std::vector<uint8_t> data = std::vector<uint8_t>(),
                      uint32_t dataLength = 0,
                      bool endFlag = false,
                      Address srcLtpEngine = Address(),
                      uint32_t offset = 0);

    /**
     * @brief Reports session cancellation to a client service instance.
     * @param id Session Id.
     * @param code StatusNotificationCode indicating the cancellation.
     * @param cx Cancellation reason code.
     */
    void ReportCancelStatus(SessionId id, StatusNotificationCode code, CxReasonCode cx);

    /**
     * @brief Add an active session id to this client service instance.
     * @param id The session id to add.
     */
    void AddSession(SessionId id);

    /**
     * @brief Remove all active sessions from this client service instance.
     */
    void ClearSessions();

    /**
     * @brief Get the total number of active sessions.
     * @return The number of sessions.
     */
    uint32_t GetNSessions() const;

    /**
     * @brief Get the session id for a given index.
     * @param index The index of the session.
     * @return The corresponding Session Id.
     */
    SessionId GetSession(uint32_t index);

  private:
    std::vector<SessionId> m_activeSessions; //!< Client Service Instance Active Sessions

    TracedCallback<SessionId,
                   StatusNotificationCode,
                   std::vector<uint8_t>,
                   uint32_t,
                   bool,
                   Address,
                   uint32_t>
        m_reportStatus; //!< Callback used to report events to the client service instances
};

/**
 * @ingroup dtn
 *
 * @brief Abstract class to keep track of LTP session state, contains shared
 * flags, counters, timers and lists used in both sender and receiver sessions
 */
class SessionStateRecord : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief Default Constructor
     */
    SessionStateRecord();

    /**
     * @brief Parameterized Constructor Session State Record segment
     * @param localLtpEngine Address of the local LTP Engine.
     * @param localClientServiceId Local client service id.
     * @param peerLtpEngine Address of the remote LTP engine.
     */
    SessionStateRecord(Address localLtpEngine,
                       uint64_t localClientServiceId,
                       Address peerLtpEngine);

    /**
     * @brief Destructor
     */
    ~SessionStateRecord() override;

    /* Timer management functions */

    /**
     * @brief Start the timer based on specified parameters.
     * @param fn Function to call on timer expiration.
     * @param delay Time to wait.
     * @param type Type of timer to schedule.
     */
    template <typename FN>
    void SetTimerFunction(FN fn, const Time delay, TimerCode type);

    /**
     * @brief Start the timer based on specified parameters for a class method.
     * @param memPtr Pointer to the class method.
     * @param objPtr Pointer to the object instance.
     * @param delay Time to wait.
     * @param type Type of timer to schedule.
     */
    template <typename MEM_PTR, typename OBJ_PTR>
    void SetTimerFunction(MEM_PTR memPtr, OBJ_PTR objPtr, const Time delay, TimerCode type);

    /**
     * @brief Start the timer with one parameter.
     * @param memPtr Pointer to the class method.
     * @param objPtr Pointer to the object instance.
     * @param param The argument to pass to the function.
     * @param delay Time to wait.
     * @param type Type of timer to schedule.
     */
    template <typename MEM_PTR, typename OBJ_PTR, typename T1>
    void SetTimerFunction(MEM_PTR memPtr, OBJ_PTR objPtr, T1 param, const Time delay, TimerCode type);

    /**
     * @brief Start the timer with two parameters.
     * @param memPtr Pointer to the class method.
     * @param objPtr Pointer to the object instance.
     * @param param The first argument to pass.
     * @param param2 The second argument to pass.
     * @param delay Time to wait.
     * @param type Type of timer to schedule.
     */
    template <typename MEM_PTR, typename OBJ_PTR, typename T1, typename T2>
    void SetTimerFunction(MEM_PTR memPtr, OBJ_PTR objPtr, T1 param, T2 param2, const Time delay, TimerCode type);

    /**
     * @brief Start timer specified by parameter, if already running, it is stopped and restarted.
     * @param type Code of timer to start.
     */
    void StartTimer(TimerCode type);

    /**
     * @brief Cancel timer
     * @param type Code of timer to cancel.
     */
    void CancelTimer(TimerCode type);

    /**
     * @brief Suspend timer
     * @param type Code of timer to suspend.
     */
    void SuspendTimer(TimerCode type);

    /**
     * @brief Resume timer.
     * @param type Code of timer to resume.
     */
    void ResumeTimer(TimerCode type);

    /* Data management functions */

    /**
     * @brief Enqueue a packet for transmission
     * @param p Packet to Enqueue.
     * @return True if enqueued successfully.
     */
    bool Enqueue(Ptr<Packet> p);

    /**
     * @brief Dequeue a packet to transmit
     * @return Dequeued Packet.
     */
    Ptr<Packet> Dequeue(void);

    /**
     * @brief Get number of queued packets
     * @return Number of packets.
     */
    uint32_t GetNPackets(void) const;

    /**
     * @brief Insert a reception claim. Should be called upon reception of a data
     * segment if used by the receiver, or upon reception of a report segment if
     * used by the sender.
     * @param serialNum Serial Number Identifier to locate claims.
     * @param low Lower Bound of stored claims.
     * @param high Higher bound of stored claims.
     * @param claim Reception claim including offset and length of the data.
     * @return True if inserted.
     */
    bool InsertClaim(uint32_t serialNum, uint32_t low, uint32_t high, LtpContentHeader::ReceptionClaim claim);

    /**
     * @brief Store claims for a given serial number.
     * @param reportHeader packet corresponding to a report (contains claims, bounds and serial number).
     * @return True if stored successfully.
     */
    bool StoreClaims(LtpContentHeader reportHeader);

    /**
     * @brief Get the number of claims stored for a given serial number.
     * @param serialNum Serial Number Identifier to locate claims.
     * @return Number of claims
     */
    uint32_t GetNClaims(uint32_t serialNum) const;

    /**
     * @brief Get the claims upper bound for a given serial number.
     * @param serialNum Serial Number Identifier to locate claims.
     * @return upper bound
     */
    uint32_t GetClaimsUpperBound(uint32_t serialNum);

    /**
     * @brief Get the claims lower bound for a given serial number.
     * @param serialNum Serial Number Identifier to locate claims.
     * @return lower bound
     */
    uint32_t GetClaimsLowerBound(uint32_t serialNum);

    /**
     * @brief Get list of claims stored for a given serial number.
     * @param reportSerialNumber Serial Number Identifier to locate claims.
     * @return list of claims.
     */
    std::set<LtpContentHeader::ReceptionClaim> GetClaims(uint64_t reportSerialNumber);

    /**
     * @brief Find missing claims between the upper and lower bound of a given serial number.
     * @param serialNum Serial Number Identifier to locate claims.
     * @return report information about missing claims.
     */
    RedSegmentInfo FindMissingClaims(uint32_t serialNum);

    /* Setter Methods */

    /**
     * @brief Increment current Checkpoint serial number.
     * @return Value before increase.
     */
    uint64_t IncrementCpCurrentSerialNumber();
    
    /**
     * @brief Increment current Report serial number.
     * @return Value before increase.
     */
    uint64_t IncrementRpCurrentSerialNumber();

    /**
     * @brief Increment retransmission counter.
     */
    void IncrementRtxNumber();

    /**
     * @brief Signal the successful transmission of the red part data.
     */
    void SetRedPartFinished();

    /**
     * @brief Signal the successful transmission of the full block of data.
     */
    void SetBlockFinished();

    /**
     * @brief Signal that the block only contained red data.
     */
    void SetFullRed();
    
    /**
     * @brief Signal that the block only contained green data.
     */
    void SetFullGreen();
    
    /**
     * @brief Set length of the red part of the block.
     * @param len Length of red data.
     */
    void SetRedPartLength(uint32_t len);

    /**
     * @brief Set starting checkpoint serial number.
     * @param serialNum Serial number to start from.
     */
    void SetCpStartSerialNumber(uint64_t serialNum);

    /**
     * @brief Set starting report serial number.
     * @param serialNum Serial number to start from.
     */
    void SetRpStartSerialNumber(uint64_t serialNum);

    /**
     * @brief Cancel Transmission Session
     * @param s Indicates which peer requested the cancellation.
     * @param r Indicates the reason for cancellation.
     */
    void Cancel(CancellationState s, CxReasonCode r);

    /**
     * @brief Suspend Transmission Session
     */
    void Suspend();

    /**
     * @brief Resume Transmission Session
     */
    void Resume();

    /* Getter Methods */

    /**
     * @brief Get the checkpoint starting serial number.
     * @return Checkpoint Starting serial number
     */
    uint64_t GetCpStartSerialNumber() const;
    
    /**
     * @brief Get the current checkpoint serial number.
     * @return Checkpoint current serial number
     */
    uint64_t GetCpCurrentSerialNumber() const;
    
    /**
     * @brief Get the report starting serial number.
     * @return Report Starting serial number
     */
    uint64_t GetRpStartSerialNumber() const;
    
    /**
     * @brief Get the current report serial number.
     * @return Report current serial number
     */
    uint64_t GetRpCurrentSerialNumber() const;
    
    /**
     * @brief Get the peer LTP Engine ID.
     * @return Remote LTP engine ns3::Address
     */
    Address GetPeerLtpEngineId() const;
    
    /**
     * @brief Get the local client service ID.
     * @return Local Client Service Instance id
     */
    uint64_t GetLocalClientServiceId() const;

    /**
     * @brief Check if the red part is finished.
     * @return true if Red part transmitted successfully, false otherwise
     */
    bool IsRedPartFinished() const;
    
    /**
     * @brief Check if the entire block is finished.
     * @return true if whole data block transmitted successfully, false otherwise
     */
    bool IsBlockFinished() const;
    
    /**
     * @brief Check if the session is canceled.
     * @return true if session has been canceled, false otherwise
     */
    bool IsCanceled() const;
    
    /**
     * @brief Check if the session is suspended.
     * @return true if session is suspended, false if active.
     */
    bool IsSuspended() const;
    
    /**
     * @brief Check if the block consists of entirely red data.
     * @return true if the block only contains red data, false if active.
     */
    bool IsFullRed() const;
    
    /**
     * @brief Check if the block consists of entirely green data.
     * @return true if the block only contains green data, false if active.
     */
    bool IsFullGreen() const;

    /**
     * @brief Get the reason code for cancellation.
     * @return Reason for cancellation.
     */
    CxReasonCode GetCancellationReason() const;

    /**
     * @brief Get the session ID.
     * @return SessionId object
     */
    SessionId GetSessionId() const;

    /**
     * @brief Get the number of retransmissions.
     * @return Number of retransmissions.
     */
    uint64_t GetRTxNumber() const;

    /**
     * @brief Get the length of the red data part.
     * @return Length of the red part of the data block.
     */
    uint32_t GetRedPartLength() const;

    /* Constants */
    static const uint32_t MIN_INITIAL_SERIAL_NUMBER = 1;      //!< Minimum bound for random serial number
    static const uint32_t MAX_INITIAL_SERIAL_NUMBER = 16383;  //!< Maximum bound for random serial number
    static const uint64_t MAX_SERIAL_NUMBER = 4294967296LLU;  //!< Absolute limit for a serial number before forced reset

  protected:
    SessionId m_sessionId; //!< Session Identifier
    LtpQueueSet m_txQueue; //!< Outgoing Packet Queue

    Address m_localLtpEngine;      //!< Address of the local Ltp Engine
    Address m_peerLtpEngine;       //!< Address of the peer Ltp Engine
    uint64_t m_localClientService; //!< Local Client Service Id

    /* Timers */
    Timer m_CpTimer; //!< Checkpoint timer.
    Timer m_RsTimer; //!< Report timer.
    Timer m_CxTimer; //!< Cancel timer.

    /* Checkpoints*/
    uint64_t m_firstCpSerialNumber;   //!< First checkpoint serial number chosen(sender)/received(receiver)
    uint64_t m_currentCpSerialNumber; //!< Next checkpoint serial number (sender) / Last checkpoint received (receiver)

    /* Reception Reports */
    uint64_t m_firstRpSerialNumber;   //!< First report serial number chosen(receiver)/received (sender)
    uint64_t m_currentRpSerialNumber; //!< Next report serial number (receiver) / Last report received (sender)

    std::map<uint64_t, RedSegmentInfo>
        m_rcvSegments; //!< Track Received (receiver) or ACKed (sender) segments - First : Serial Number , Second: ReceptionClaims

    bool m_redpartSuccess; //!< Red part Transmitted/Received successfully
    bool m_blockSuccess;   //!< Full block Transmitted/Received successfully
    bool m_fullRedData;    //!< True if the block only contains red data
    bool m_fullGreenData;  //!< True if the block only contains green data

    uint32_t m_redPartLength; //!< Length of the red part
    uint32_t m_lowBound;      //!< Smallest non acknowledged offset
    uint32_t m_highBound;     //!< Biggest non acknowledged offset

    /* Retransmission */
    uint64_t m_rTxCnt; //!< Count Number of Retransmissions.

    /* Session state */
    CancellationState m_canceled;  //!< Session Canceled state
    CxReasonCode m_canceledReason; //!< Reason of Cancellation
    bool m_suspended;              //!< Session Suspended state
};

/**
 * @ingroup dtn
 *
 * @brief The Sender Session State Record class to
 * store sender specific variables and methods.
 */
class SenderSessionStateRecord : public SessionStateRecord
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief Default Constructor
     */
    SenderSessionStateRecord();
    
    /**
     * @brief Constructs a Sender session state record for the given destination
     * @param localLtpEngineId Local LTP Engine ns3::Address
     * @param localClientServiceId Local Client Service Instance
     * @param destinationClientService Destination Client Service Instance
     * @param destinationLtpEngine Destination LTP Engine ns3::Address
     * @param number Random variable used for sequence numbering
     */
    SenderSessionStateRecord(Address localLtpEngineId,
                             uint64_t localClientServiceId,
                             uint64_t destinationClientService,
                             Address destinationLtpEngine,
                             Ptr<UniformRandomVariable> number);
                             
    /**
     * @brief Default Destructor
     */
    ~SenderSessionStateRecord() override;

    /**
     * @brief Get the destination client service ID.
     * @return Destination Client Service Instance
     */
    uint64_t GetDestination() const;
    
    /**
     * @brief Get the number of checkpoint retransmissions.
     * @return Number of checkpoint retransmissions during this session.
     */
    uint64_t GetCpRtxNumber() const;
    
    /**
     * @brief Check if the red part was successfully acknowledged.
     * @return true if Red part acknowledged successfully, false otherwise
     */
    bool IsRedPartAck() const;
    
    /**
     * @brief Get the block of data to be transmitted.
     * @return block data to be transmitted.
     */
    std::vector<uint8_t> GetBlockData();
    
    /**
     * @brief Get the block of data to be transmitted within the bounds specified
     * by offset and length.
     * @param offset starting index of the data.
     * @param length length of the data.
     * @return block data to be transmitted.
     */
    std::vector<uint8_t> GetBlockData(uint32_t offset, uint32_t length);

    /**
     * @brief Store a copy of the block data to be transmitted.
     * @param data vector containing the data.
     */
    void CopyBlockData(std::vector<uint8_t> data);
    
    /**
     * @brief Signal the successful acknowledgment of the red part data.
     */
    void SetRedPartAck();
    
    /**
     * @brief Increment Checkpoint retransmission counter.
     */
    void IncrementCpRtxNumber();

  private:
    uint64_t m_destinationClientServiceId; //!< Destination Client Service instance
    std::vector<uint8_t> m_txData;         //!< Block Data to transmit
    uint64_t m_cpTxCnt;                    //!< Count Number of retransmitted checkpoints
    bool m_redpartAckSuccess;              //!< Red part Acknowledged successfully
};

/**
 * @ingroup dtn
 *
 * @brief The Receiver Session State Record class to store
 * receiver specific variables and methods.
 */
class ReceiverSessionStateRecord : public SessionStateRecord
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief Default Constructor
     */
    ReceiverSessionStateRecord();
    
    /**
     * @brief Constructs a Receiver session state record for the given destination
     * @param localLtpEngineId Local LTP Engine ns3::Address
     * @param localClientServiceId Local client service id
     * @param session Session ID obtained from the sender LTP engine
     * @param number Random variable stream
     */
    ReceiverSessionStateRecord(Address localLtpEngineId,
                               uint64_t localClientServiceId,
                               SessionId session,
                               Ptr<UniformRandomVariable> number);
                               
    /**
     * @brief Default Destructor
     */
    ~ReceiverSessionStateRecord() override;

    /* Data Storage */

    /**
     * @brief Store a received red data segment in the inbound traffic queue.
     * @param p received packet.
     */
    void StoreRedDataSegment(Ptr<Packet> p);
    
    /**
     * @brief Store a received green data segment in the inbound traffic queue.
     * @param p received packet.
     */
    void StoreGreenDataSegment(Ptr<Packet> p);

    /**
     * @brief remove a received red data segment from the inbound traffic queue.
     * @return p received packet.
     */
    Ptr<Packet> RemoveRedDataSegment();
    
    /**
     * @brief remove a received green data segment from the inbound traffic queue.
     * @return p received packet.
     */
    Ptr<Packet> RemoveGreenDataSegment();

    /* Setter functions */

    /**
     * @brief Set lower bound of received data
     * @param offset Lower bound.
     */
    void SetLowBound(uint32_t offset);

    /**
     * @brief Set higher bound of received data
     * @param offset higher bound.
     */
    void SetHighBound(uint32_t offset);
    
    /**
     * @brief Increment Report retransmission counter.
     */
    void IncrementRpRtxNumber();

    /* Getter functions */

    /**
     * @brief Get lower bound of received data
     * @return Lower bound.
     */
    uint32_t GetLowBound() const;
    
    /**
     * @brief Get higher bound of received data
     * @return High bound.
     */
    uint32_t GetHighBound() const;
    
    /**
     * @brief Get the number of report retransmissions.
     * @return Number of checkpoint retransmissions during this session.
     */
    uint64_t GetRpRtxNumber() const;

  private:
    uint64_t m_rpTxCnt; //!< Count Number of retransmitted reports.
    std::map<uint32_t, Ptr<Packet>> m_rxRedBuffer; //!< Storage for received red segments
    std::queue<Ptr<Packet>> m_rxGreendBuffer;      //!< Storage for received green segments
};

// ==============================================================================
// LtpBundleCla Adapter
// ==============================================================================

/** * @ingroup dtn
 *
 * @brief Ltp protocol core class. Contains the protocol logic and the
 * sender and receiver state machines.
 */
class LtpBundleCla : public BundleCla
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Default constructor. Initializes internal structures.
     */
    LtpBundleCla();

    /**
     * @brief Destructor. Closes all active sockets and cleans up resources.
     */
    ~LtpBundleCla() override;

    // --- 1. Core CLA Interface ---

    /**
     * @brief Setup a connection between a local and remote address.
     * @param node The local node instance where the sockets will be created.
     * @param localAddress The local ns3::Address to bind and listen on.
     * @param remoteAddress The destination ns3::Address to connect to.
     */
    void Setup(Ptr<Node> node, Address localAddress, Address remoteAddress);

    /**
     * @brief Send a serialized bundle packet via the LTP connection.
     * @param packet The serialized bundle packet to send over the network.
     */
    void Send(Ptr<Packet> packet) override;

    /**
     * @brief Check if the CLA has been configured and is ready to process data.
     * @return True if Setup() has been successfully called.
     */
    bool IsUp() const override;

    // --- 2. Configuration & Properties ---

    /**
     * @brief Set the node associated with this LTP protocol instance.
     * @param node Pointer to the local Node.
     */
    void SetNode(Ptr<Node> node);

    /**
     * @brief Get the node associated with this LTP protocol instance.
     * @return Pointer to the local Node.
     */
    Ptr<Node> GetNode() const;

    /**
     * @brief Get the local engine Address.
     * @return The local ns3::Address of the engine.
     */
    Address GetLocalEngineId() const;

    /**
     * @brief Get the checkpoint retransmission limit.
     * @return The checkpoint retransmission limit.
     */
    uint32_t GetCheckPointRetransLimit() const;

    /**
     * @brief Get the report segment retransmission limit.
     * @return The report segment retransmission limit.
     */
    uint32_t GetReportRetransLimit() const;

    /**
     * @brief Get the reception problem limit.
     * @return The reception problem limit.
     */
    uint32_t GetReceptionProblemLimit() const;

    /**
     * @brief Get the cancellation segment retransmission limit.
     * @return The cancellation retransmission limit.
     */
    uint32_t GetCancellationRetransLimit() const;

    /**
     * @brief Get the retransmission cycle limit.
     * @return The retransmission cycle limit.
     */
    uint32_t GetRetransCycleLimit() const;

    // --- 3. Session API ---

    /**
     * @brief Requests the cancellation of a specific session.
     * @param id Session Id of the session to cancel.
     */
    void CancelSession(SessionId id);

  private:
    typedef std::map<SessionId, Ptr<SessionStateRecord>> SessionStateRecords;
    typedef std::map<uint64_t, Ptr<ClientServiceStatus>> ClientServiceInstances; 

    // --- Protocol Core Logic ---
    
    /**
     * @brief Encapsulate block data into MTU-sized transmission segments.
     * @param remoteAddress The remote address.
     * @param ssr The session state record.
     * @param p The packet containing the payload.
     * @param rdSize The size of the reliable red data part.
     * @param rtx Whether this is a retransmission.
     */
    void EncapsulateBlockData(Address remoteAddress, Ptr<SessionStateRecord> ssr, Ptr<Packet> p, uint64_t rdSize, bool rtx = false);
    
    /**
     * @brief Terminate and clean up an active session.
     * @param id The session ID.
     */
    void CloseSession(SessionId id);
    
    /**
     * @brief Formally indicate the complete reception of the red data block.
     * @param id The session ID.
     */
    void SignifyRedPartReception(SessionId id);
    
    /**
     * @brief Formally indicate the arrival of a green data segment.
     * @param id The session ID.
     */
    void SignifyGreenPartSegmentArrival(SessionId id);
    
    /**
     * @brief Evaluate the session state to determine if all red data has been successfully received.
     * @param id The session ID.
     */
    void CheckRedPartReceived(SessionId id);

    // --- Transmission & Retransmission ---
    
    /**
     * @brief Fire a report segment over the network.
     * @param id The session ID.
     * @param cpSerialNum The checkpoint serial number.
     */
    void ReportSegmentTransmission(SessionId id, uint64_t cpSerialNum);
    
    /**
     * @brief Transmit an acknowledgement for a received report segment.
     * @param id The session ID.
     * @param rpSerialNum The report serial number.
     */
    void ReportSegmentAckTransmission(SessionId id, uint64_t rpSerialNum);
    
    /**
     * @brief Retransmit missing reliable segment data.
     * @param id The session ID.
     * @param info Red Segment data containing missing claim info.
     */
    void RetransmitSegment(SessionId id, RedSegmentInfo info);
    
    /**
     * @brief Retransmit a missing report segment.
     * @param id The session ID.
     * @param info Red Segment info struct.
     */
    void RetransmitReport(SessionId id, RedSegmentInfo info);
    
    /**
     * @brief Retransmit a missing checkpoint segment.
     * @param id The session ID.
     * @param info Red Segment info struct.
     */
    void RetransmitCheckpoint(SessionId id, RedSegmentInfo info);

    // --- Socket & Network Handlers ---
    
    /**
     * @brief Handle raw incoming packets from the TCP/UDP socket.
     * @param socket The underlying ns3::Socket.
     */
    void HandleRead(Ptr<Socket> socket);
    
    /**
     * @brief Calculate the active Maximum Transmission Unit for this interface.
     * @return The MTU in bytes.
     */
    virtual uint16_t GetMtu() const;

    // --- Timers & State Updates ---
    
    /**
     * @brief Fire the timer to track checkpoint timeouts.
     * @param id The session ID.
     * @param info The segment info being tracked.
     */
    void SetCheckPointTransmissionTimer(SessionId id, RedSegmentInfo info);
    
    /**
     * @brief Fire the timer to track report timeouts.
     * @param id The session ID.
     * @param info The segment info being tracked.
     */
    void SetReportReTransmissionTimer(SessionId id, RedSegmentInfo info);
    
    /**
     * @brief Indicate the transmission of the final block piece.
     * @param id The session ID.
     */
    void SetEndOfBlockTransmission(SessionId id);

    // --- Internal Getters/Setters ---
    
    /**
     * @brief Get the remote engine ID.
     * @return The remote ns3::Address.
     */
    Address GetRemoteEngineId() const;
    
    /**
     * @brief Set the remote engine ID.
     * @param id The remote ns3::Address.
     */
    void SetRemoteEngineId(Address id);
    
    /**
     * @brief Get the session ID.
     * @return The active SessionId.
     */
    SessionId GetSessionId() const;
    
    /**
     * @brief Set the session ID.
     * @param id The active SessionId.
     */
    void SetSessionId(SessionId id);

    // --- Callback Setters ---
    
    /**
     * @brief Set the callback for link-up events.
     * @param cb Callback function.
     */
    void SetLinkUpCallback(Callback<void, Ptr<LtpBundleCla>> cb);
    
    /**
     * @brief Set the callback for link-down events.
     * @param cb Callback function.
     */
    void SetLinkDownCallback(Callback<void, Ptr<LtpBundleCla>> cb);
    
    /**
     * @brief Set the callback for checkpoint dispatch events.
     * @param cb Callback function.
     */
    void SetCheckPointSentCallback(Callback<void, SessionId, RedSegmentInfo> cb);
    
    /**
     * @brief Set the callback for report dispatch events.
     * @param cb Callback function.
     */
    void SetReportSentCallback(Callback<void, SessionId, RedSegmentInfo> cb);
    
    /**
     * @brief Set the callback for end-of-block dispatch events.
     * @param cb Callback function.
     */
    void SetEndOfBlockSentCallback(Callback<void, SessionId> cb);
    
    /**
     * @brief Set the callback for cancellation dispatch events.
     * @param cb Callback function.
     */
    void SetCancellationCallback(Callback<void, SessionId> cb);

    /**
     * @brief A basic struct mapping active network windows.
     */
    struct ActivationInterval
    {
        Time start; //!< Time the interval begins.
        Time stop;  //!< Time the interval ends.
    };

    // --- Network & Socket State ---
    Ptr<Node> m_node;           //!< Local Node pointer.
    Ptr<Socket> m_rcvSocket;    //!< Receiver Socket.
    uint16_t m_keepAliveValue;  //!< Keep-alive timeout.

    // --- Protocol Configuration Limits ---
    Address m_localEngineId;   //!< Local Engine Address.
    uint32_t m_cpRtxLimit;     //!< Checkpoint Retx Limit.
    uint32_t m_rpRtxLimit;     //!< Report Retx Limit.
    uint32_t m_rxProblemLimit; //!< RX Problem Limit.
    uint32_t m_cxRtxLimit;     //!< Cancel Retx Limit.
    uint32_t m_rtxCycleLimit;  //!< Global Cycle Limit.
    uint8_t m_version = 0;     //!< Engine Version.

    // --- Protocol State ---
    SessionId m_activeSessionId;            //!< Currently executing session.
    SessionStateRecords m_activeSessions;   //!< Active record mappings.
    ClientServiceInstances m_activeClients; //!< Active client mappings.

    // --- Callbacks ---
    Callback<void, SessionId, RedSegmentInfo> m_checkpointSent; //!< Checkpoint trigger.
    Callback<void, SessionId, RedSegmentInfo> m_reportSent;     //!< Report trigger.
    Callback<void, SessionId> m_endOfBlockSent;                 //!< EOB trigger.
    Callback<void, SessionId> m_cancelSent;                     //!< Cancel trigger.
    Callback<void, Ptr<LtpBundleCla>> m_linkUp;                 //!< Link up trigger.
    Callback<void, Ptr<LtpBundleCla>> m_linkDown;               //!< Link down trigger.

    // --- Timers & Scheduling ---
    Time m_localDelays;     //!< Computed local delay constraint.
    Time m_onewayLightTime; //!< Hard limit on light-time propagation.
    std::queue<ActivationInterval> m_localOperatingSchedule; //!< Active network windows.

    Address m_peerLtpEngineId; //!< Peer Engine ID.
};

// ==============================================================================
// SessionStateRecord Template Implementations
// ==============================================================================

/**
 * @brief Template to assign an action to a specific protocol timer.
 * @tparam FN Function pointer type.
 * @param fn The function to execute on expiration.
 * @param delay Time offset until expiration.
 * @param type Target timer definition.
 */
template <typename FN>
void SessionStateRecord::SetTimerFunction(FN fn, const Time delay, TimerCode type)
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

/**
 * @brief Template to assign a member method to a specific protocol timer without arguments.
 * @tparam MEM_PTR Class method pointer type.
 * @tparam OBJ_PTR Object instance pointer type.
 * @param memPtr Pointer to the execution method.
 * @param objPtr Pointer to the target object.
 * @param delay Time offset until expiration.
 * @param type Target timer definition.
 */
template <typename MEM_PTR, typename OBJ_PTR>
void SessionStateRecord::SetTimerFunction(MEM_PTR memPtr, OBJ_PTR objPtr, const Time delay, TimerCode type)
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

/**
 * @brief Template to assign a member method to a specific protocol timer with one argument.
 * @tparam MEM_PTR Class method pointer type.
 * @tparam OBJ_PTR Object instance pointer type.
 * @tparam T1 Method argument type.
 * @param memPtr Pointer to the execution method.
 * @param objPtr Pointer to the target object.
 * @param param The passed argument.
 * @param delay Time offset until expiration.
 * @param type Target timer definition.
 */
template <typename MEM_PTR, typename OBJ_PTR, typename T1>
void SessionStateRecord::SetTimerFunction(MEM_PTR memPtr, OBJ_PTR objPtr, T1 param, const Time delay, TimerCode type)
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

/**
 * @brief Template to assign a member method to a specific protocol timer with two arguments.
 * @tparam MEM_PTR Class method pointer type.
 * @tparam OBJ_PTR Object instance pointer type.
 * @tparam T1 First method argument type.
 * @tparam T2 Second method argument type.
 * @param memPtr Pointer to the execution method.
 * @param objPtr Pointer to the target object.
 * @param param First passed argument.
 * @param param2 Second passed argument.
 * @param delay Time offset until expiration.
 * @param type Target timer definition.
 */
template <typename MEM_PTR, typename OBJ_PTR, typename T1, typename T2>
void SessionStateRecord::SetTimerFunction(MEM_PTR memPtr, OBJ_PTR objPtr, T1 param, T2 param2, const Time delay, TimerCode type)
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

} // namespace ns3

#endif /* LTP_CONVERGENCE_LAYER_ADAPTER_H */