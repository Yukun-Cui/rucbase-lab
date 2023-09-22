# RucBase
```
 ____  _   _  ____ ____    _    ____  _____ 
|  _ \| | | |/ ___| __ )  / \  / ___|| ____|
| |_) | | | | |   |  _ \ / _ \ \___ \|  _|  
|  _ <| |_| | |___| |_) / ___ \ ___) | |___ 
|_| \_ \___/ \____|____/_/   \_\____/|_____|
```
[Rucbase](https://github.com/ruc-deke/rucbase-lab).是一个精简的RDBMS原型系统，用于《数据库系统实现》课程的实验教学。本实验框架源码参考和借鉴了CMU15-445课程的[BusTub](https://github.com/cmu-db/bustub) 和Standford CS346课程的[Redbase](https://web.stanford.edu/class/cs346/2015/redbase.html) 。

## 实验环境：
- 操作系统：Ubuntu 18.04 及以上(64位)
- 编译器：GCC
- 编程语言：C++17
- 管理工具：cmake
- 推荐编辑器：VScode

### 依赖环境库配置：
- gcc 7.1及以上版本（要求完全支持C++17）
- cmake 3.16及以上版本
- flex
- bison
- readline

欲查看有关依赖运行库和编译工具的更多信息，以及如何运行的说明，请查阅[Rucbase使用文档](docs/Rucbase使用文档.md)

欲了解如何在非Linux系统PC上部署实验环境的指导，请查阅[Rucbase环境配置文档](docs/Rucbase环境配置文档.md)

## 实验文档索引

> 这里给出目前公开的文档分类索引

### 开发规范文档

- [Rucbase开发文档](docs/Rucbase开发文档.md)

### 项目说明文档

- [Rucbase环境配置文档](docs/Rucbase环境配置文档.md)
- [Rucbase使用文档](docs/Rucbase使用文档.md)
- [Rucbase项目结构](docs/Rucbase项目结构.pdf)
- [框架图](docs/框架图.pdf)

### 学生实验文档(2022-10-26日更新)

> 请使用命令git pull来拉取最新的实验文档

- [Rucbase学生实验操作说明示例](docs/Rucbase学生实验操作说明示例.md)
- [Rucbase-Lab1存储管理实验文档](docs/Rucbase-Lab1[存储管理实验文档].md)
- [Rucbase-Lab2索引管理实验文档](docs/Rucbase-Lab2[索引管理实验文档].md)
- [Rucbase-Lab3查询执行实验文档](docs/Rucbase-Lab3[查询执行实验文档].md)
- [Rucbase-Lab3查询执行实验指导](docs/Rucbase-Lab3[查询执行实验指导].md)
- [Rucbase-Lab4并发控制实验文档](docs/Rucbase-Lab4[并发控制实验文档].md)

### 时间安排和工作量估计

| **实验**     | **发布时间**      | **截止时间**      | **工作量估计** | **难度系数** |
| ------------ | ----------------- | ----------------- | -------------- | ------------ |
| 实验环境配置 | 9.20（第三周） | 9.26（第四周） | 1~2h | 简单 |
| 存储管理实验 | 9.27（第四周）    | 10.17（第七周）   | 15h            | 简单         |
| 索引管理实验 | 10.11（第六周）   | 11.14（第十一周） | 35h            | 中等         |
| 查询执行实验 | 11.8（第十周） | 12.19（十六周） | 30-40h         | 困难         |
| 并发控制实验 | 11.22（第十二周） | 1.2（第十八周） | 25-30h         | 困难         |
