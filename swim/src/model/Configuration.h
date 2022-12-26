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
#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <pladapt/Configuration.h>
#include <modules/MTServerType.h>
#include <modules/LoadBalancer.h>
#include <ostream>

class Configuration : public pladapt::Configuration {
    int serversA; // number of active servers (there is one more powered up if bootRemain > 0
    int serversB;
    int serversC;
    int bootRemain; // how many periods until we have one more server. If 0, no server is booting
    // true if the cache of the last added server is cold
    MTServerType::ServerType bootServerType;
    int brownoutLevel;

    LoadBalancer::TrafficLoad trafficA;
    LoadBalancer::TrafficLoad trafficB;
    LoadBalancer::TrafficLoad trafficC;
public:
    Configuration();
    Configuration(int serverA, int serverB, int serverC, int bootRemain, MTServerType::ServerType serverType, 
    int brownoutLevel, 
    LoadBalancer::TrafficLoad trafficA = LoadBalancer::TrafficLoad::HUNDRED,
    LoadBalancer::TrafficLoad trafficB = LoadBalancer::TrafficLoad::ZERO,
    LoadBalancer::TrafficLoad trafficC = LoadBalancer::TrafficLoad::ZERO);

//    virtual bool operator==(const Configuration& other) const;
//    virtual void printOn(std::ostream& os) const;
//    virtual unsigned poweredServers() const;
//
//    friend std::ostream& operator<<(std::ostream& os, const Configuration& config);


    MTServerType::ServerType getBootType() const;
    int getBootRemain() const;
    void setBootRemain(int bootRemain, MTServerType::ServerType = MTServerType::ServerType::NONE);
    int getBrownOutLevel() const;
    void setBrownOutLevel(int brownoutLevel);
    int getActiveServers(MTServerType::ServerType) const;
    void setActiveServers(int servers, MTServerType::ServerType);
    int getServers(MTServerType::ServerType serverType) const;
    LoadBalancer::TrafficLoad getTraffic(MTServerType::ServerType serverType) const;
    void setTraffic(MTServerType::ServerType serverType, LoadBalancer::TrafficLoad trafficLoad);
    int getTotalActiveServers() const;
};

#endif /* CONFIGURATION_H_ */
