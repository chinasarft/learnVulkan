vulkan的扩展和layer都分为instance层和device层
所以对应的功能的扩展要在instance和device层都启用
比如windows下的窗口程序在instance 级别需要如下两个扩展
        VK_KHR_surface
        VK_KHR_win32_surface // Linux 中称为VK_KHR_xlib_surface 或 VK_KHR_xcb_surface
在device级别需要下面这个扩展
#define VK_KHR_SWAPCHAIN_EXTENSION_NAME   "VK_KHR_swapchain
所以在程序里面需要分别对应instance和device检查对应的扩展是否支持，按照vulkan的设计理论如果直接传递给vulkan api即使不至此和应该也不会返回错误(没有尝试)，所以最好检查
