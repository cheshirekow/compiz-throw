find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (throw PLUGINDEPS composite)

# use Jan Woetzel's doxygen doc target
include("${CMAKE_MODULE_PATH}/TargetDoc.cmake" OPTIONAL)
