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
#ifndef EXECUTIONMANAGERMOD_H_
#define EXECUTIONMANAGERMOD_H_

#include <omnetpp.h>
#include <set>
#include "BootComplete_m.h"
#include <model/Model.h>
#include <string>
#include "ExecutionManagerModBase.h"
using namespace std;
class ExecutionManagerMod : public ExecutionManagerModBase, omnetpp::cListener {
    omnetpp::simsignal_t serverBusySignalId;
    int serverBeingRemovedModuleId;
    omnetpp::cMessage* completeRemoveMsg;

    /**
     * Sends a message so that when received (immediately) will complete the removal
     * This is used so that the signal handler can do it
     */
    void completeServerRemoval();


    bool isServerBeingRemoveEmpty();
  protected:
    virtual void initialize();
    virtual void handleMessage(omnetpp::cMessage *msg);

    // target-specific methods (e.g., actual servers, sim servers, etc.)

    /**
     * @return BootComplete* to be handled later by doAddServerBootComplete()
     */
    virtual BootComplete* doAddServer(MTServerType::ServerType serverType, bool instantaneous = false);
    virtual void doAddServerBootComplete(BootComplete* bootComplete);

    /**
     * @return BootComplete* identical in content (not the pointer itself) to
     *   what doAddServer() would have return for this server
     */
    virtual BootComplete* doRemoveServer(MTServerType::ServerType serverType);
    virtual void doSetBrownout(MTServerType::ServerType serverType, double factor);
    virtual void doSetBrownout(double factor);
    virtual string getModuleStr(MTServerType::ServerType serverType) const;

  public:
    ExecutionManagerMod();
    virtual ~ExecutionManagerMod();

    virtual void receiveSignal(omnetpp::cComponent *source, omnetpp::simsignal_t signalID, bool value, cObject *details) override;
    virtual string getServerString(MTServerType::ServerType serverType, bool internal=false) const;
};

#endif /* EXECUTIONMANAGERMOD_H_ */

