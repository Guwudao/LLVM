# LLVM

 ## 开发的插件效果如下
![效果图.png](https://upload-images.jianshu.io/upload_images/3350266-ced03a5f6e5818e7.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)



# 简介

#### 本开发是基于LLVM，那么我们先来简单了解一下LLVM：
+ LLVM项目是模块化、可重用的编译器以及工具链技术的集合
+ 美国计算机协会 (ACM) 将其2012 年软件系统奖项颁给了LLVM，之前曾经获得此奖项的软件和技术包括:Java、Apache、 Mosaic、the World Wide Web、Smalltalk、UNIX、Eclipse等等。
+ LLVM的创始人 Chris Lattner，也是swift之父（也就是下面的这位） 
 [LLVM官网链接](https://llvm.org)

![Chris Lattner.jpg](https://upload-images.jianshu.io/upload_images/3350266-4c3207bd5dfa2746.jpg?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
<br>

#### 而什么是Clang呢？
+ Clang是LLVM项目的一个子项目
+ 基于LLVM架构的C/C++/Objective-C编译器前端
[Clang传送门](http://clang.llvm.org/)

  简单上图看一眼二者之间的关系
![Clang与LLVM.png](https://upload-images.jianshu.io/upload_images/3350266-35568ab560601311.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
<br>

# 实战分析

### 基本文件
+ 新建文件夹llvm，下载LLVM（预计大小 648.2 M）
`$ git clone https://git.llvm.org/git/llvm.git/`
+ 下载clang（预计大小 240.6 M）
`$ cd llvm/tools`
`$ git clone https://git.llvm.org/git/clang.git/`
+ 注意 ：
clang的下载目录应在llvm/tools下（如图）
![clang下载目录.png](https://upload-images.jianshu.io/upload_images/3350266-e7e42a014a284518.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
<br>

### 编译工具

+ 这里推荐使用ninja和cmake（先安装brew，https://brew.sh/）
`$ brew install cmake`
`$ brew install ninja`
+ ninja如果安装失败，可以直接从github获取release版放入【/usr/local/bin】目录中
[ninja的GitHub传送门]( https://github.com/ninja-build/ninja/releases)

### 编译方式
##### 1、ninja编译
+ 在LLVM源码同级目录下新建一个【llvm_build】目录(最终会在【llvm_build】目录下生成【build.ninja】)
+ 同时在LLVM源码同级目录下新建一个【llvm_release】目录(最终编译文件会在llvm_release文件夹路径下)
`$ cd llvm_build`
`$ cmake -G Ninja ../llvm -DCMAKE_INSTALL_PREFIX=‘安装路径’（本机为/Users/xxx/Desktop/LLVM/llvm_release）`

+ 依次执行编译、安装指令
`$ ninja` 
  ###### 编译完毕后， 【llvm_build】目录大概 21.05 G(仅供参考)

  `$ ninja install`
  ###### 安装完毕后，安装目录大概 11.92 G(仅供参考)

+ 最终生成build.ninja以及llvm_release文件夹位置如下图
![build.ninja.jpg](https://upload-images.jianshu.io/upload_images/3350266-8df6aa029923fdee.jpg?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
![llvm目录.png](https://upload-images.jianshu.io/upload_images/3350266-1e4e1e45537e2d32.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

##### 2、Xcode编译
+ 作为iOS开发者，使用Xcode则是更加得心应手，但是Xcode编译的速度较慢，亲测在一个小时以上
+ 在llvm同级目录下新建一个【llvm_xcode】目录，然后开始编译
`$ cd llvm_xcode`
`$ cmake -G Xcode ../llvm`
+ 完成后我们将看到熟悉的打开方式![xcode编译.png](https://upload-images.jianshu.io/upload_images/3350266-458f749ccaa76512.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 打开project后我们选择Auto的方式来创建scheme![AutoScheme.png](https://upload-images.jianshu.io/upload_images/3350266-48ca5716ceff7961.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 然后选择ALL_BUILD进行编译，此处应有1+小时的休息时间，然后就可以开始插件的编写![ALL_BUILD.png](https://upload-images.jianshu.io/upload_images/3350266-ceedb81497a84a79.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

<br>
# 编写插件

#### 1.目录
+ 在【clang/tools】源码目录下新建一个插件目录，假设叫做【JJPlugin】(如下图"JJPlugin目录"红色箭头所示)
+ 在【clang/tools/CMakeLists.txt】(如下图"JJPlugin目录"绿色箭头所示) 最后添加内容: `add_clang_subdirectory(JJPlugin)`，小括号里是插件目录名
![CMakeLists.png](https://upload-images.jianshu.io/upload_images/3350266-7d43922d9dfde0bb.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 在【JJPlugin】目录下新建一个【CMakeLists.txt】 (如下图黄色箭头所示)，文件内容是: `add_llvm_loadable_module(JJPlugin JJPlugin.cpp)`
![JJPlugin-CMakeList.png](https://upload-images.jianshu.io/upload_images/3350266-1ae6b9060be33271.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 在【JJPlugin】目录下]新建一个【JJPlugin.cpp】`$ touch JJPlugin.cpp` (如下图黄色箭头所示)

![JJPlugin目录.png](https://upload-images.jianshu.io/upload_images/3350266-d8baa1050085a612.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

+ 目录文件创建完成后，需要利用cmake重新生成一下Xcode项目
`$ cmake -G Xcode ../llvm`
+ 插件源代码在【Sources/Loadable modules】目录下可以找到，这样就可以直接在Xcode里编写插件代码
![source file.png](https://upload-images.jianshu.io/upload_images/3350266-fe863c513579d96b.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

### 2.代码
+ 这里提供了类名中下划线的检测以及类首字母小写的警告
[github链接](https://github.com/Guwudao/LLVM)
![code.png](https://upload-images.jianshu.io/upload_images/3350266-a5688734533fb4ba.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

### 3.编译
+ 选择我们的插件进行编译
![编译插件.png](https://upload-images.jianshu.io/upload_images/3350266-039188995eb1a2c9.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 然后就能看到我们编译的lib
![show in finder.png](https://upload-images.jianshu.io/upload_images/3350266-fdbae2ffb96e551a.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
![lib路径.png](https://upload-images.jianshu.io/upload_images/3350266-ff2589de9dadccfc.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

### 4.加载
+ 在Xcode项目中指定加载插件动态库:BuildSettings > OTHER_CFLAGS
`-Xclang -load -Xclang 动态库路径 -Xclang -add-plugin -Xclang 插件名称`
![加载插件.png](https://upload-images.jianshu.io/upload_images/3350266-00324371cd12dd84.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

### 5.Hack Xcode
+ 首先我们要对Xcode进行Hack，才能修改默认的编译器
+ 找到自己编译好的clang的路径，也就是在我们前面定义的release的bin目录下。![clang路径.png](https://upload-images.jianshu.io/upload_images/3350266-3cc0457f6f7657cb.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 下载[XcodeHacking.zip](https://pan.baidu.com/s/1V6XQSoRwWzRimLZKXFYKEw)，解压，右键【HackedClang.xcplugin】点击"显示包内容"打开修改【HackedClang.xcplugin/Contents/Resources/HackedClang.xcspec】的内容
![HackedClang.xcspec.png](https://upload-images.jianshu.io/upload_images/3350266-1fb95e40becf9790.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 把这个路径修改为上面自己编译好的clang的路径
![ExecPath.png](https://upload-images.jianshu.io/upload_images/3350266-abb6e9d8796b687a.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 然后在XcodeHacking目录下进行命令行，将XcodeHacking的内容剪切到Xcode内部
>$ sudo mv HackedClang.xcplugin \`xcode-select -print-
path`/../PlugIns/Xcode3Core.ideplugin/Contents/SharedSupport/Developer/Library/Xcode/Plug-ins

>$ sudo mv HackedBuildSystem.xcspec \`xcode-select -print- path`/Platforms/iPhoneSimulator.platform/Developer/Library/Xcode/Specifications`

### 6.使用
+ 重启Xcode，修改Xcode的编译器，转而使用我们自己的编译器
![clang LLVM.png](https://upload-images.jianshu.io/upload_images/3350266-7f15688d6e926455.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
+ 编译后如果代码存在语法问题，便能看到本文开头的警告提示
![效果图.png](https://upload-images.jianshu.io/upload_images/3350266-3ea7eb7625b7defb.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)


<br>
# 总结

Clang 的开源给了我们更多的操作空间，我们可以利用clang的API针对语法树(AST)进行相应的分析和处理，进一步完善我们的需求，也能更好地提升我们代码的规范和质量。
附上关于语法树AST的资料：
https://clang.llvm.org/doxygen/namespaceclang.html
https://clang.llvm.org/doxygen/classclang_1_1Decl.html
https://clang.llvm.org/doxygen/classclang_1_1Stmt.html
