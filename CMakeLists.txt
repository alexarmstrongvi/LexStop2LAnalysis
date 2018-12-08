################################################################################
# LexStop2LAnalysis Package
################################################################################

# Declare the package name
atlas_subdir( LexStop2LAnalysis )

# extra deps based on the build environment
# set( extra_deps )
# set( extra_libs )

# Declare which other packages this package depends on.
atlas_depends_on_subdirs (
    PUBLIC
    SusyNtuple 
    Superflow 
    #    ${extra_deps}
)

# Find the non-ATLAS externals that the build needs.
# >> ROOT Data Analysis Framework
#       Tree : TChain, TTree
#       Physics : TLorentzVector
find_package ( ROOT COMPONENTS Tree Physics )

atlas_add_library ( LexStop2LAnalysisLib
    LexStop2LAnalysis/*.h Root/*.cxx
    PUBLIC_HEADERS LexStop2LAnalysis
    PRIVATE_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
    LINK_LIBRARIES SuperflowLib
    PRIVATE_LINK_LIBRARIES ${ROOT_LIBRARIES}
)

# exectuables
#set( extra_libs )

# Build the executable(s) of the package
atlas_add_executable ( makeFlatNtuples util/makeFlatNtuples.cxx
    INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
    LINK_LIBRARIES ${ROOT_LIBRARIES} LexStop2LAnalysisLib
    #LINK_LIBRARIES ${ROOT_LIBRARIES} LexStop2LAnalysisLib ${extra_libs}
)

