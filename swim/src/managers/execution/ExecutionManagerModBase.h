/*******************************************************************************
 * Simulator of Web Infrastructure and Management
 * Copyright (c) 2016 Carnegie Mellon University.
 * All Rights Reserved.
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS," WITH NO WARRANTIES WHATSOEVER. CARNEGIE
 * MELLON UNIVERSITY EXPRESSLY DISCLAIMS TO THE FULLEST EXTENT PERMITTED BY LAW
 * ALL EXPRESS, IMPLIED, AND STATUTORY WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, AND NON-INFRINGEMENT OF PROPRIETARY RIGHTS.
 *  
 * Released under a BSD license, please see license.txt for full terms.
 * DM-0003883
 *******************************************************************************/
#ifndef EXECUTIONMANAGERMODBASE_H_
#define EXECUTIONMANAGERMODBASE_H_

#include <omnetpp.h>
#include <set>
#include "BootComplete_m.h"
#include <model/Model.h>
#include "ExecutionManager.h"
#include "AllTactics.h"
#include "assert.h"

class ExecutionManagerModBase : public omnetpp::cSimpleModule, public ExecutionManager {
    int serverRemoveInProgress;
    omnetpp::simsignal_t serverRemovedSignal;
    omnetpp::simsignal_t serverAddedSignal;
    omnetpp::simsignal_t serverActivatedSignal;
    omnetpp::simsignal_t brownoutSetSignal;

  protected:
    typedef std::set<BootComplete*> BootCompletes;
    BootCompletes pendingMessages;

    Model* pModel;
    omnetpp::cMessage* testMsg;

    virtual void initialize();
    virtual void handleMessage(omnetpp::cMessage *msg);

    /**
     * When a remove server completes, the implementation must call this function
     *
     * This is used to, for example, wait for a server to complete ongoing requests
     * doRemoveServer() can disconnect the server from the load balancer
     * and when all requests in the server have been processed, call this method,
     * so that the server is removed from the model
     * TODO there should be a state in the model to mark servers being shutdown
     */
    void notifyRemoveServerCompleted(MTServerType::ServerType serverType);

    double getMeanAndVarianceFromParameter(const cPar& par, double& variance) const;


    // target-specific methods (e.g., actual servers, sim servers, etc.)

    /**
     * @return BootComplete* to be handled later by doAddServerBootComplete()
     */
    virtual BootComplete* doAddServer(MTServerType::ServerType serverType, bool instantaneous = false) = 0;
    virtual void doAddServerBootComplete(BootComplete* bootComplete) = 0;
    virtual MTServerType::ServerType getServerTypeFromName(const char* name) const;

    /**
     * @return BootComplete* identical in content (not the pointer itself) to
     *   what doAddServer() would have returned for this server
     */
    virtual BootComplete* doRemoveServer(MTServerType::ServerType serverType) = 0;
    virtual void doSetBrownout(double factor) = 0;

  public:
    static const char* SIG_SERVER_REMOVED;
    static const char* SIG_SERVER_ADDED;
    static const char* SIG_SERVER_ACTIVATED;
    static const char* SIG_BROWNOUT_SET;

    ExecutionManagerModBase();
    virtual ~ExecutionManagerModBase();
    virtual void addServerLatencyOptional(MTServerType::ServerType serverType, bool instantaneous = false);

    virtual void addServer() {assert(false);} 
    virtual void removeServer() {assert(false);} 

    // ExecutionManager interface
    virtual void addServer(MTServerType::ServerType serverType);
    virtual void removeServer(MTServerType::ServerType serverType);
    virtual void setBrownout(double factor);
    virtual void divertTraffic(LoadBalancer::TrafficLoad serverA, LoadBalancer::TrafficLoad serverB, LoadBalancer::TrafficLoad serverC);
};

#endif /* EXECUTIONMANAGERMODBASE_H_ */



/* remove server process
 * -disconnect from load balancer
 * -mark as to be removed
 * -wait until empty: busy=0 signal from server && queue empty && marked to be removed
 * -delete module
 * -remove from model
 *
 */
