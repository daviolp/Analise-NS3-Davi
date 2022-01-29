#!/bin/bash

DIR="../../simulation/$pktSize/"

# Simulation Time
SIMULATION_TIME=30

# Packet Size
packetSize=(512 1024)
#packetSize=(1024)

# Nodes Wifi
nWifi=(11)

# Nodes CSMA
nCsma=(10 20 30 40 50 60 70 80 90 100)
#nCsma=(100)

for pktSize in "${packetSize[@]}"; do
    
    # Create Packet Size dir 
    if [ ! -d $DIR ];
    then
        mkdir $DIR ;
    fi

    for nodeWifi in "${nWifi[@]}"; do
        for nodeCsma in "${nCsma[@]}"; do
           #for execution in `seq $TRIALS`; do
                echo -e "packetSize: $pktSize, nWifi: $nodeWifi, nCsma: $nodeCsma \n" ;
                sudo ./waf --run "networkDavi --verbose=0 --packetSize=$pktSize --nWifi=$nodeWifi --nCsma=$nodeCsma --simulationTime=$SIMULATION_TIME"; >> $DIR/log-netWorkDavi-packetSize-$pktSize-nWifi-$nodeWifi-nCsma-$nodeCsma.txt ;
                echo -e "\n"
           #done
        done
    done    
done
