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
#include "ExecutionManagerMod.h"
#include <sstream>
#include <boost/tokenizer.hpp>
#include <cstdlib>
#include "PassiveQueue.h"
#include "modules/MTServer.h"
#include "modules/MTBrownoutServer.h"
#include <util/Utils.h>


using namespace std;
using namespace omnetpp;

Define_Module(ExecutionManagerMod);

const char* SERVER_MODULE_NAME = "server";
const char* LOAD_BALANCER_MODULE_NAME = "loadBalancer";
const char* INTERNAL_SERVER_MODULE_NAME = "server";
const char* INTERNAL_QUEUE_MODULE_NAME = "queue";
const char* SINK_MODULE_NAME = "classifier";

ExecutionManagerMod::ExecutionManagerMod()
        : serverBeingRemovedModuleId(-1), completeRemoveMsg(0) {
    serverBusySignalId = registerSignal("busy");
}


void ExecutionManagerMod::initialize() {
    ExecutionManagerModBase::initialize();
    getSimulation()->getSystemModule()->subscribe(serverBusySignalId, this);
}

void ExecutionManagerMod::handleMessage(cMessage *msg) {
    if (msg == completeRemoveMsg) {
        cModule* module = getSimulation()->getModule(serverBeingRemovedModuleId);
        MTServerType::ServerType serverType = getServerTypeFromName(module->getFullName());
        notifyRemoveServerCompleted(serverType);
        module->gate("out")->disconnect();
        module->deleteModule();
        serverBeingRemovedModuleId = -1;

        cancelAndDelete(msg);
        completeRemoveMsg = 0;
    } else {
        ExecutionManagerModBase::handleMessage(msg);
    }
}


void ExecutionManagerMod::doAddServerBootComplete(BootComplete* bootComplete) {
    cModule *server = getSimulation()->getModule(bootComplete->getModuleId());
    cModule* loadBalancer = getParentModule()->getSubmodule(
            LOAD_BALANCER_MODULE_NAME);
    cModule* sink = getParentModule()->getSubmodule(SINK_MODULE_NAME);
    // connect gates
    loadBalancer->getOrCreateFirstUnconnectedGate("out", 0, false, true)->connectTo(
            server->gate("in"));
    server->gate("out")->connectTo(
            sink->getOrCreateFirstUnconnectedGate("in", 0, false, true));
}


ExecutionManagerMod::~ExecutionManagerMod() {
}


BootComplete* ExecutionManagerMod::doAddServer(MTServerType::ServerType serverType, bool instantaneous) {
    // find factory object
    cModuleType *moduleType = cModuleType::get(getModuleStr(serverType).c_str());

    int serverCount = pModel->getConfiguration().getServers(serverType);
    stringstream name;
    name << getServerString(serverType);
    name << serverCount + 1;
    cModule *module = moduleType->create(name.str().c_str(), getParentModule());

    // setup parameters
    module->finalizeParameters();
    module->buildInside();

    // copy all params of the server inside the appserver module from the template
    cModule* pNewSubmodule = module->getSubmodule(INTERNAL_SERVER_MODULE_NAME);
    if (serverCount >= 1) {
        // copy from an existing server
        stringstream templateName;
        templateName << SERVER_MODULE_NAME;
        templateName << 1;
        cModule* pTemplateSubmodule = getParentModule()->getSubmodule(templateName.str().c_str())->getSubmodule(INTERNAL_SERVER_MODULE_NAME);
<<<<<<< HEAD
        for (int i = 0; i < pTemplateSubmodule->getNumParams(); i++) {
            pNewSubmodule->par(i) = pTemplateSubmodule->par(i);
        }
	int rdWeight = pModel->getServerWeight() + (rand() % 4);
	int rdConnection = 1 + (rand() % pModel->getActiveConnections());

=======

        for (int i = 0; i < pTemplateSubmodule->getNumParams(); i++) {
            pNewSubmodule->par(i) = pTemplateSubmodule->par(i);
        }
>>>>>>> fc568acc6e702a7b574ac602ba7dad7a5b6cf2db
    } else {
        // if it's the first server, we need to set the parameters common to all servers in the model
        pModel->setServerThreads(pNewSubmodule->par("threads"));
        double variance = 0.0;
        double mean = Utils::getMeanAndVarianceFromParameter(pNewSubmodule->par("serviceTime"), &variance);
        pModel->setServiceTime(mean, variance, serverType);
        mean = Utils::getMeanAndVarianceFromParameter(pNewSubmodule->par("lowFidelityServiceTime"), &variance);
        pModel->setLowFidelityServiceTime(mean, variance, serverType);
        pModel->setBrownoutFactor(pNewSubmodule->par("brownoutFactor"));
<<<<<<< HEAD
	int rdWeight = 1 + (rand() % 4);
	int rdConnection = 1 + (rand() % 9);
	pModel->setServerWeight(rdWeight);
	pModel->setActiveConnections(rdConnection);
=======
>>>>>>> fc568acc6e702a7b574ac602ba7dad7a5b6cf2db
    }

    // create activation message
    module->scheduleStart(simTime());
    module->callInitialize();

    BootComplete* bootComplete = new BootComplete;
    bootComplete->setModuleId(module->getId());
    return bootComplete;
}

string ExecutionManagerMod::getModuleStr(MTServerType::ServerType serverType) const {
    string module_str = "";

    switch (serverType) {
    case MTServerType::ServerType::POWERFUL:
        module_str = "plasa.modules.AppServerA";
        break;
    case MTServerType::ServerType::AVERAGE:
        module_str = "plasa.modules.AppServerB";
        break;
    case MTServerType::ServerType::WEAK:
        module_str = "plasa.modules.AppServerC";
        break;
    case MTServerType::ServerType::NONE:
        module_str = "";
    }

    return module_str;
}

string ExecutionManagerMod::getServerString(MTServerType::ServerType serverType, bool internal) const {
    string name_str = SERVER_MODULE_NAME;

    if (internal) {
        name_str = INTERNAL_SERVER_MODULE_NAME;
    }

    switch (serverType) {
    case MTServerType::ServerType::POWERFUL:
        name_str += string("_A_");
        break;
    case MTServerType::ServerType::AVERAGE:
        name_str += string("_B_");
        break;
    case MTServerType::ServerType::WEAK:
        name_str += string("_C_");
        break;
    case MTServerType::ServerType::NONE:
        name_str = "";
    }

    return name_str;
}

BootComplete*  ExecutionManagerMod::doRemoveServer(MTServerType::ServerType serverType) {
    int serverCount = pModel->getConfiguration().getServers(serverType);
    stringstream name;
    name << getServerString(serverType);
    name << serverCount;
    cModule *module = getParentModule()->getSubmodule(name.str().c_str());

    // disconnect module from load balancer
    cGate* pInGate = module->gate("in");
    if (pInGate->isConnected()) {
        cGate *otherEnd = pInGate->getPathStartGate();
        otherEnd->disconnect();
        ASSERT(otherEnd->getIndex() == otherEnd->getVectorSize() - 1);

        // reduce the size of the out gate in the queue module
        otherEnd->getOwnerModule()->setGateSize(otherEnd->getName(), otherEnd->getVectorSize() - 1);
        //TODO this is probably leaking memory because the gate may not be being deleted
    }

    serverBeingRemovedModuleId = module->getId();

    // check to see if we can delete the server immediately (or if it's busy)
    if (isServerBeingRemoveEmpty()) {
        completeServerRemoval();
    }

    BootComplete* bootComplete = new BootComplete;
    bootComplete->setModuleId(module->getId());
    return bootComplete;
}

void ExecutionManagerMod::doSetBrownout(MTServerType::ServerType serverType, double factor) {
    int serverCount = pModel->getConfiguration().getServers(serverType);
    for (int s = 1; s <= serverCount; s++) {
        stringstream name;
        name << getServerString(serverType);
        name << s;

        cModule* module = getParentModule()->getSubmodule(name.str().c_str());
        module->getSubmodule("server")->par("brownoutFactor").setDoubleValue(factor);
    }
}

void ExecutionManagerMod::doSetBrownout(double factor) {
    doSetBrownout(MTServerType::ServerType::POWERFUL, factor);
    doSetBrownout(MTServerType::ServerType::AVERAGE, factor);
    doSetBrownout(MTServerType::ServerType::WEAK, factor);
}


void ExecutionManagerMod::completeServerRemoval() {
    Enter_Method("sendMe()");

    // clear cache for server, so that the next time it is instantiated it is fresh
    cModule* module = getSimulation()->getModule(serverBeingRemovedModuleId);
    check_and_cast<MTBrownoutServer*>(module->getSubmodule(INTERNAL_SERVER_MODULE_NAME))->clearServerCache();

    completeRemoveMsg = new cMessage("completeRemove");
    cout << "scheduled complete remove at " << simTime() << endl;
    scheduleAt(simTime(), completeRemoveMsg);
}

bool ExecutionManagerMod::isServerBeingRemoveEmpty() {
    bool isEmpty = false;
    cModule* module = getSimulation()->getModule(serverBeingRemovedModuleId);
    MTServer* internalServer = check_and_cast<MTServer*> (module->getSubmodule(INTERNAL_SERVER_MODULE_NAME));
    if (internalServer->isEmpty()) {
        queueing::PassiveQueue* queue = check_and_cast<queueing::PassiveQueue*> (module->getSubmodule(INTERNAL_QUEUE_MODULE_NAME));
        if (queue->length() == 0) {
            isEmpty = true;
        }
    }
    return isEmpty;
}

void ExecutionManagerMod::receiveSignal(cComponent *source, simsignal_t signalID, bool value, cObject *details) {
    if (signalID == serverBusySignalId && source->getParentModule()->getId() == serverBeingRemovedModuleId) {
        if (value == false && isServerBeingRemoveEmpty()) {
            completeServerRemoval();
        }
    }
}
