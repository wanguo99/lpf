################# Add include #################
# list(APPEND ADD_INCLUDE "include")
###############################################

############## Add source files ###############
file(GLOB_RECURSE ADD_SRCS "src/*.c")
###############################################

###### Add required/dependent components ######
# Demo 应用依赖 OSAL
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
# 应用程序特定的编译选项
###############################################

# 注册组件
register_component()
