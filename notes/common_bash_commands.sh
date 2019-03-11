python submit_to_condor.py ../lists/file_lists_prefixed/*/*txt `cat ../../source/LexStop2LAnalysis/notes/split_samples_over_1M_events.txt`

# Sumw submit
python submit_to_condor.py ../lists/file_lists_prefixed/mc16*/*txt --exec grabSumw -o sumw_output/

# Building prefixed filelists from base names
for f in */*txt; do sed 's;/group/phys-susy;root://fax.mwt2.org:1094//pnfs/uchicago.edu/atlaslocalgroupdisk/rucio/group/phys-susy;g' $f > ../../file_lists_prefixed/$f; done

# Getting 
for f in mc16a mc16d mc16e; do python make_condor_lists.py -i ../container_lists/n0307_${f}SusyNt.txt -v -o n0307/tmp_${f}; done
for f in mc16a mc16d mc16e; do python ~/LexTools/ATLAS_sw/get_fax_link_samples.py -f ../container_lists/n0307_${f}SusyNt.txt -o n0307/ -s MWT2_UC_LOCALGROUPDISK MWT2_UC_SCRATCHDISK SLACXRD_LOCALGROUPDISK SLACXRD_SCRATCHDISK; done

SuperflowAnaStop2L -i ../../susynt-write/run/old_test_results/data17_13TeV.00326439.physics_Main.deriv.DAOD_SUSY2.r10250_p3399_p3637_susynt_old.root -c
SuperflowAnaStop2L -i ../../susynt-write/run/old_test_results/mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r10201_p3627_susynt_old.root -c
