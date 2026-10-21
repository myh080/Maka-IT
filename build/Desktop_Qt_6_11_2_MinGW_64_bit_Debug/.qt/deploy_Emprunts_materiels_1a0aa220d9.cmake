include("H:/Qt project/Maka_IT/build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Emprunts_materiels-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "H:/Qt project/Maka_IT/build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug/Emprunts_materiels.exe"
    GENERATE_QT_CONF
)
