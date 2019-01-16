#!/bin/bash

echo "Getting MC DSIDs"
for campaign in mc16a mc16d mc16e; do
    if [[ $campaign == "mc16a" ]]; then
        r_tag="r9364"
    elif [[ $sample == "mc16d" ]]; then
        r_tag="r10201"
    elif [[ $sample == "mc16e" ]]; then
        r_tag="r10724"
    fi
    search_pattern="mc16_13TeV:*.AOD.*${r_tag}*"
    ofile="all_${campaign}_DSIDs.txt"
    filter_pattern="[1-9][0-9]{5}\.[A-Za-z0-9_]*"
    cmd="rucio ls $search_pattern --filter type=CONTAINER --short"
    #echo ">> ${cmd}"
    $cmd | grep -Eo "$filter_pattern" | sort -u -t. -k1,1 > $ofile
    n_dsids=`wc -l $ofile | awk '{print $1}'`
    echo "INFO :: Created $ofile [${n_dsids} DSIDs found]"
done

echo
echo "Getting Data DSIDs"
for year in data15 data16 data17 data18; do
    search_pattern="${year}_13TeV:*physics_Main*.AOD.*"
    filter_pattern="[1-9][0-9]{5}"
    ofile="all_${year}_DSIDs.txt"
    cmd="rucio ls $search_pattern --filter type=DATASET --short"
    #echo ">> ${cmd}"
    $cmd | grep -Eo "$filter_pattern" | sort | uniq > $ofile
    n_dsids=`wc -l $ofile | awk '{print $1}'`
    echo "INFO :: Created $ofile [${n_dsids} DSIDs found]"
done
echo "Done!"
