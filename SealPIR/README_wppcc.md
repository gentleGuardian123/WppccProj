## 编译SealPIR

在`path/to/WppccProj/SealPIR`目录下：
```
mkdir build 
cd build 
cmake .. -DCMAKE_PREFIX_PATH=path/to/WppccProj/libs
make 
```
在`path/to/WppccProj/SealPIR`目录下会出现`bin/`文件夹，其中存放的即是编译后的可执行文件。
