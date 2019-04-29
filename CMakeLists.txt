################################################################################
# LexStop2LAnalysis Package
################################################################################

# Declare the package name
atlas_subdir( LexStop2LAnalysis )

# Declare which other packages this package depends on.
atlas_depends_on_subdirs (
    PUBLIC
    SusyNtuple 
    Superflow 
    jigsawcalculator
    FakeBkgTools
)

# Find the non-ATLAS externals that the build needs.
# >> ROOT Data Analysis Framework
#       Tree : TChain, TTree
#       Physics : TLorentzVector
find_package ( ROOT COMPONENTS Tree Physics)

atlas_add_library ( LexStop2LAnalysisLib
    LexStop2LAnalysis/*.h Root/*.cxx
    PUBLIC_HEADERS LexStop2LAnalysis
    INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
    LINK_LIBRARIES SuperflowLib JigsawCalculator FakeBkgToolsLib ${ROOT_LIBRARIES}
)

# Build the executable(s) of the package
atlas_add_executable ( SuperflowAnaStop2L util/SuperflowAnaStop2L.cxx
    INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
    LINK_LIBRARIES ${ROOT_LIBRARIES} LexStop2LAnalysisLib
)

atlas_add_executable ( SuperflowAnaStop2L_tmp util/SuperflowAnaStop2L_tmp.cxx
    INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
    LINK_LIBRARIES ${ROOT_LIBRARIES} LexStop2LAnalysisLib
)

atlas_install_data( data/* )
