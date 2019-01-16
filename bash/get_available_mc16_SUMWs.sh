#!/bin/bash

CHECK_DIR="/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/dev/PileupReweighting/share/"
#CHECK_DIR="/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/dev/PileupReweighting/mc16_13TeV/"

echo "Getting MC DSIDs"
echo "Looking for available samples in:"
echo "  $CHECK_DIR"
for campaign in mc16a mc16d mc16e; do
    search_pattern="*pileup_${campaign}*FS.root*"
    filter_pattern="[1-9][0-9]{5}"
    ofile="complete_${campaign}_SUMWs.txt"
    cmd="$CHECK_DIR -type f -name "${search_pattern}""
    #echo ">> ${cmd}"
    find $cmd | grep -oE "${filter_pattern}" | sort > $ofile 
    all_file="${campaign}/all_${campaign}_DSIDs.txt"
    if [ -f $all_file ]; then
        python ~/LexTools/ATLAS_sw/compare_sample_lists.py $all_file $ofile --keep_shared --plain -o tmp_${ofile}
        mv tmp_${ofile} ${ofile}
        mv ${ofile} ${campaign}/${ofile}
        ofile=${campaign}/${ofile}
    else
        echo "NAY"
    fi
    n_dsids=`wc -l $ofile | awk '{print $1}'`
    echo "INFO :: Created $ofile [${n_dsids} DSIDs found]"
done

echo "Done!"
