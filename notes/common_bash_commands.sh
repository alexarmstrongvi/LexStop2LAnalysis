python submit_to_condor.py ../lists/file_lists_prefixed/*/*txt --selection zjets3l `cat ../../source/LexStop2LAnalysis/notes/samples_to_split.txt` -o output/SuperflowAnaStop2l_zjets3l --tar-file output/SuperflowAnaStop2l_zjets3l/area.tgz
python submit_to_condor.py ../lists/file_lists_prefixed/*/*txt --selection fake_zjets3l `cat ../../source/LexStop2LAnalysis/notes/samples_to_split.txt` -o output/SuperflowAnaStop2l_zjets3l_den --tar-file output/SuperflowAnaStop2l_zjets3l_den/area.tgz
python submit_to_condor.py ../lists/file_lists_prefixed/data*/*txt --selection fake_zjets3l -o output/tmp --tar-file output/tmp/area.tgz
python submit_to_condor.py ../lists/file_lists_prefixed/*/*txt --selection zjets2l_inc `cat ../../source/LexStop2LAnalysis/notes/samples_to_split.txt` -o output/SuperflowAnaStop2l_zjets2l_inc --tar-file output/SuperflowAnaStop2l_zjets2l_inc/area.tgz
# Sumw submit
python submit_to_condor.py ../lists/file_lists_prefixed/mc16*/*txt --exec grabSumw -o sumw_output/

# Building prefixed filelists from base names
for f in */*txt; do sed 's;/group/phys-susy;root://fax.mwt2.org:1094//pnfs/uchicago.edu/atlaslocalgroupdisk/rucio/group/phys-susy;g' $f > ../../file_lists_prefixed/$f; done

# Getting 
for f in mc16a mc16d mc16e; do python make_condor_lists.py -i ../container_lists/desired_n0307_${f}SusyNt.txt -v -o n0307/tmp_${f}; done
for f in mc16a mc16d mc16e; do python ~/LexTools/ATLAS_sw/get_fax_link_samples.py -f ../container_lists/desired_n0307_${f}SusyNt.txt -d n0307/ -s MWT2_UC_LOCALGROUPDISK MWT2_UC_SCRATCHDISK SLACXRD_LOCALGROUPDISK SLACXRD_SCRATCHDISK; done

SuperflowAnaStop2L -i ../../susynt-write/run/old_test_results/data17_13TeV.00326439.physics_Main.deriv.DAOD_SUSY2.r10250_p3399_p3637_susynt_old.root -c
SuperflowAnaStop2L -i ../../susynt-write/run/old_test_results/mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r10201_p3627_susynt_old.root -c
NtMaker --mctype mc16a --prw-auto -f ./test_daod_samples/mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r9364_p3652/ -i mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r9364_p3652 --outfilename mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r9364_p3652_susynt_new.root -n 1000

# Documenting missed samples

# 1)
for y in mc16a mc16d mc16e data15 data16 data17 data18; do echo $y; python compare_sample_lists.py ../container_lists/n0307_${y}SusyNt.txt ${y}/desired_${y}_DSIDs.txt -c overlap -o ${y}/all_${y}_SusyNts.txt; done
#2)
source get_available_AODs.sh # for AODs and ALL_EVENTS_AVAILABLE 
source get_available_AODs.sh # for DAODs and ALL_EVENTS_AVAILABLE 
source get_available_AODs.sh # for AODs and PARTIAL 
source get_available_AODs.sh # for DAODs and PARTIAL 
source get_available_PRWs.sh
source sort_files.sh
#3) Go into each year/campaign directory and run
for y in mc16a mc16d mc16e; do cd ${y}; source ../build_missing_lists.sh; cd ..; done
#4) Go into each year/campaign directory and run

for y in mc16a mc16d mc16e; do cd ${y}; source ../build_missing_PRW_lists.sh; cd ..; done






