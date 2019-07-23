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
    IFFTruthClassifier
    Control/AthToolSupport/AsgTools
    PhysicsAnalysis/Interfaces/AsgAnalysisInterfaces
    PhysicsAnalysis/AnalysisCommon/PATInterfaces
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
    LINK_LIBRARIES SuperflowLib JigsawCalculator IFFTruthClassifierLib
    AsgAnalysisInterfaces AsgTools xAODEventInfo PATInterfaces
    ${ROOT_LIBRARIES}
)

# Build the executable(s) of the package
# define a function to call the add_executable macro on our executables
function(addExec filename)
    set(execname)
    get_filename_component(execname ${filename} NAME_WE)
    atlas_add_executable( ${execname} "util/${execname}.cxx"
        INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
        LINK_LIBRARIES ${ROOT_LIBRARIES} LexStop2LAnalysisLib
    )
endfunction(addExec)

file(GLOB files "util/*.cxx")
foreach(file ${files})
    addExec(${file})
endforeach()

atlas_install_data( data/* )
