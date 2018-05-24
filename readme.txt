新旧版本区别
源码目录：/Users/liuye/vulkan/MoltenV
之前的69版本编译后直接放在工程目录下面:
cmake -G Xcode ../learnVulkan/ -DMOLTENVK_LIB_PATH=/Users/liuye/vulkan/MoltenVK/Package/Release/MoltenVK/macOS/ -DMOLTENVK_INC_PATH=/Users/liuye/vulkan/MoltenVK/Package/Release/MoltenVK/include
更新到73版本后放在了~/Library下面
cmake -G Xcode ../learnVulkan/ -DMOLTENVK_LIB_PATH=/Users/liuye/Library/Developer/Xcode/DerivedData/MoltenVK-ahuotqeellbyfdawyrqibpvtfarz/Build/Products/Release -DMOLTENVK_INC_PATH=/Users/liuye/vulkan/MoltenVK/External/Vulkan-LoaderAndValidationLayers/include
73版本并且vulkan的头文件没有安装，所以不确定是否可以用External里面的，并且External里面的vulkan.h也是不一样的
所以这里还是考虑不要自己编译，直接下载lunarG的编译好的库直接用
