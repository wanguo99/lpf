################# Add include #################
# list(APPEND ADD_INCLUDE "include")
###############################################

############## Add source files ###############
list(APPEND ADD_SRCS "src/main.c")
###############################################

###### Add required/dependent components ######
# list(APPEND ADD_REQUIREMENTS component1)
###############################################

# 注册 main 组件
register_component()
