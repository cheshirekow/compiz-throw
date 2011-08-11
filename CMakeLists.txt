cmake_minimum_required(VERSION 2.8)

# defines the project name
project (compiz-plugin-throw)

find_package (Compiz)

if( COMPIZ_FOUND )
    message( "Compiz found, configuring for throw plugin")
    include (CompizPlugin)
    compiz_plugin (throw PLUGINDEPS composite)
else( COMPIZ_FOUND )
    message( "Compiz not found, configuring for doxygen only")
endif( COMPIZ_FOUND )

# use Jan Woetzel's doxygen doc target
include("${CMAKE_MODULE_PATH}/TargetDoc.cmake" OPTIONAL)
