LexStop2LAnalysis
=============

A repository for all the code specific to the [SUSY 3G Stop2L Analysis]
(https://twiki.cern.ch/twiki/bin/viewauth/AtlasProtected/TtbarMET2L), 
at least on the 3-body side that I contributed to.

# Contents

* [Packages](#packages)
* [Setup](#setup)
* [How to do what](#how-to-do-what)

It is assumed that the top level directory structure for the project contains a `source\`, `build\`, and `run\` directory as discussed in the [ATLAS Software Tutorial ](https://atlassoftwaredocs.web.cern.ch/ABtutorial/release_setup/).
From within this top level directory, do the following things

# Packages
- Add the n0308 tag of SusyNt packages to `source\` (see [UCINtSetup](https://gitlab.cern.ch/alarmstr/UCINtSetup))
- Add [LexTools](https://github.com/alexarmstrongvi/LexTools) to the home directory, making sure to setup atlasrootstyle as described in the README
- Add [PlotTools](https://github.com/alexarmstrongvi/PlotTools) to home directory or `source\`
- Add [Superflow](https://gitlab.cern.ch/susynt/superflow) to `source\`
- Add [jigsawcalculator](https://gitlab.cern.ch/alarmstr/jigsawcalculator) to `source\`, which will describe how to set up RestFrames in `source\` as well

# Setup

From the top level directory in a clean environment, create a link to the setup script 
```bash
ln -s source/LexStop2LAnalysis/bash/setup_env.sh setup_stop2l_env.sh
```
Now and any time later when running from a clean environment, setup with
```bash
source setup_stop2l_env.sh
```

# How to do what

TODO
