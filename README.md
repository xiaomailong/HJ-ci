# CI FOR HJ
在运行系统前请注意下面几个问题：
0. buy HJ 联锁机
0. 平台软件的执行需要特殊硬件，需要执行请联系zhangys@hengjun365.com
0. linux下一定要使用eth0作为通信网卡，负责ip地址验证会报错

## 软件跟踪与调试
为方便软件调试与跟踪，有如下工具可用:
* 使用[.CiShark](https://github.com/lisency/CiShark)进行日志记录与分析
* 使用[.CiPerformanceAnalyser](https://github.com/lisency/CiPerformanceAnalyser)监控系统性能
* 使用[.CiMemSniffer](https://github.com/lisency/CiMemSniffer)查看FPGA内存空间

## 系统依赖软件
* linux下基本依赖软件：
>apt-get -y install cmake make gcc gdb g++ python vim less ssh lsof telnet strace ntpdate tree htop
* [.libexpat](http://expat.sourceforge.net)用来解析配置文件。
  debian下安装方法：
>apt-get -y install expat libexpat1-dev
* [.libcrypto](https://www.openssl.org/docs/crypto/crypto.html)用来生成文件的md5以便初始化检查各服务器软件版本是否一致。该库目前只在linux下使用，Windows无需安装。  
debian下安装方法：
>apt-get -y install libssl-dev

## 部署及运行条件
windows.windows下由于是使用模拟运行，所以相对简单  
1. git 克隆代码
1. 使用cmake-gui工具生成vs工程
1. 将settings/platform.xml当中的DeviceId改为Windows下的设备Id。默认为5，即Windows设备id。


linux.下面以将软件部署与/root目录为例，注：部署到其它目录需要在配置当中更改驱动模块的路径

```bash
# #login with user root
#cd ~
#git clone git@code.csdn.net:p569354158/ci.git
#cd ci
#mkdir build
#cd build
#cmake ..
#make
# #check if there are compile error,or unit test run fail,fix it and go on
# #lets clone and goto driver directory to make it one by one
#cd ~
#git clone git@code.csdn.net:p569354158/driver.git
#cd driver
#cd cpudpram && make
# #because driver of cpudpram use different device node number for master and slave cpu,so you need copy it respectively
#cd cpudpram && cp dpram_master.sh dpram.sh # do it for master cpu
#cd cpudpram && cp dpram_slave.sh dpram.sh # do it for slave cpu
#cd can && make
#cd fiber && make
#cd uart && make
#cd net && make
#cd watchdog && make
# #at here you need add system startup script to ~/.bashrc to start up ci when system boot up
# #IMPORTANT! different init script used for master and spare series,so you need copy it respectively
#cd ~/script/ && cp ci_profile_master.sh ci_profile.sh # do it for master series
#cd ~/script/ && cp ci_profile_spare.sh ci_profile.sh # do it for spare series
#cat > ~/.bashrc
#/root/ci/script/ci_profile.sh
#^D
# #ok, you have do it over,if you don't want load driver module manually,just reboot system,it will run our init script and load driver module
#reboot
```
## 平台软件编码规范
* 使用CIAPI_FUNC宏包装公有函数。
* 在整个系统的编码当中向其它模块暴露的函数一律使用以下命名规则。使用这种命名规范
  可以迅速的让其它开发人员知道某个函数是否可用，并且知道它在哪个模块当中。详见Python [pep-0007](https://www.python.org/dev/peps/pep-0007/)。
  
>CIModule_ConcreteName，总体分为两部分，由_分割，前面部分由前缀和模块名组成，后面为具体的名称，两部分的命名方法都为驼峰命名

* 公有变量暂时未做编码统一性要求。
* 私有函数及变量一律使用static修饰，以此避免链接时弱符号变量产生的不确定行为。
* 在心跳响应函数当中避免使用printf，因为printf使用了futex进行线程同步，会导致信号中断处理挂起。

## 发货前的准备
1. 检查光线通信道是否正常。
由于光线通信道做为双系之间的通信，其不像电子单元的通信和模块的通信可以提示在控显机界面上，当光线通信到断开时，要么发生双主系，要么另外一系宕机。

检查光线通信道时应按照如下格式检查

| from |to| CHANNAL_A| CHANNAL_B| 
|----|--|----------|----------| 
|200 |202| OK/FAIL | OK/FAIL | 
|200 |203| OK/FAIL | OK/FAIL | 
|201 |202| OK/FAIL | OK/FAIL |
|201 |203| OK/FAIL | OK/FAIL |
|202 |200| OK/FAIL | OK/FAIL |
|202 |200| OK/FAIL | OK/FAIL |
|203 |201| OK/FAIL | OK/FAIL |
|203 |201| OK/FAIL | OK/FAIL |

当所有检查完毕才可确保系统稳定正常的运行

## 其它需要注意的地方

### 解决ssh登录慢的问题

    echo UseDNS no | tee -a /etc/ssh/sshd_config

确保ssh登录无需使用DNS,因为我们一般是使用ip登录，所以不需要这个，而且该项可能造成登录缓慢。