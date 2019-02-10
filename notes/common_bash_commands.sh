python submit_to_condor.py ../lists/file_lists_prefixed/*/*txt --split-dsids 410472 --split-dsids
python submit_to_condor.py ../lists/file_lists_prefixed/*/*txt --split-dsids 410472 --split-dsids 364161 --split-dsids 349268 --split-dsids 364108 --split-dsids 280673 data15 --split-dsids 280862 data15 --split-dsids 280950 data15 --split-dsids 281411 data15 --split-dsids 283429 data15 --split-dsids 283780 data15 --split-dsids 284213 data15 --split-dsids 284285 data15 --split-dsids 299584 data16 --split-dsids 300540 data16 --split-dsids 364122 mc16e --split-dsids 364253 mc16d --split-dsids 364254 mc16d

# Sumw submit
python submit_to_condor.py ../lists/file_lists_prefixed/mc16*/*txt --exec grabSumw -o sumw_output/

# Building prefixed filelists from base names
for f in */*txt; do sed 's;/group/phys-susy;root://fax.mwt2.org:1094//pnfs/uchicago.edu/atlaslocalgroupdisk/rucio/group/phys-susy;g' $f > ../../file_lists_prefixed/$f; done
