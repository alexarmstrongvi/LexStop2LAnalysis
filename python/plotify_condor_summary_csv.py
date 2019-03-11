import numpy as np
import matplotlib.pyplot as plt

import csv
import os
from collections import defaultdict

_ifile_name = 'condor_summary.csv'

assert os.path.exists(_ifile_name)
print("Loading in", _ifile_name)

labels = [
        ('disk_usage','Disk Usage','MB'),
        ('disk_allocated', 'Disk Allocated','MB'),
        ('memory_usage', 'Memory Usage','MB'),
        ('memory_allocated', 'Memory Allocated','MB'),
        #
        ('total', 'Total Job Time','hrs'),
	('Job submitted', 'Submit', 'hrs'),
	('Job executing', 'Execution', 'hrs'),
        ('','',''),
        #
	('Remote system call socket lost', 'Disconnect', 'hrs'),
	('Remote system call reconnect failure', 'Fail Reconnect', 'hrs'),
	('Job evicted from machine', 'Eviction', 'hrs'),
	('Shadow exception', 'Shadow Exception', 'hrs'),
        #
        ('rates', 'Looper Rate','events/sec'),
        ('times', 'Time in Looper','hrs'),
        ('events', 'Event Processed',''),
        ]

with open(_ifile_name, 'r') as ifile:
    my_dict = defaultdict(list)
    csv_reader = csv.DictReader(ifile)
    for row in csv_reader:
        for k, v in row.items():
            try:
                my_dict[k].append(float(v))
            except ValueError:
                pass

    plt.figure()
    plt.subplots_adjust(wspace=0.4,hspace=0.4)
    for ii, k in enumerate(labels,1):
        key, title, ylabel = k[0],k[1],k[2]
        if not key:
            print("Empty plot")
            continue
        else:
            print(ii,key)
        ax = plt.subplot(4,4,ii)
        ax.set_title(title)
        ax.set_ylabel(ylabel)
        ax.boxplot(my_dict[key], whis=[0.1,99.9])
    plt.show()

